/* Geiger Board Firmware — Ring PCB C
 * STM32G0B1CCT6
 * Placeholder firmware for hardware bring-up
 *
 * Peripherals:
 *   - FDCAN1:  CAN-FD bus, 500kbps arbitration / 2Mbps data
 *              This board is END OF BUS — 120Ω termination resistor fitted
 *   - TIM1:    CH1 PA8 — HV boost PWM ~30kHz drives MPSA42
 *   - TIM2:    CH1 PA0 — Input capture, Geiger pulse timestamps
 *   - ADC1:    PA1 — HV rail monitor (10MΩ/50kΩ divider)
 *   - USART1:  Debug UART (PB6 TX, PB7 RX), 115200 baud
 *   - GPIO:    PA15 LED_STAT, PB5 CAN_STB
 *
 * HV circuit:
 *   MPSA42 base ← TIM1 CH1 PWM via 10kΩ base resistor
 *   MPSA42 collector → 1mH inductor → 5V
 *   Flyback: inductor/collector junction → UF4007 → C_HV (470pF 630V) → GND
 *   SBM-21 anode ← 400V rail via 10MΩ quench resistor
 *   SBM-21 cathode → GND
 *   Pulse: anode → 100pF 630V → comparator IN+ → TIM2 CH1 input capture
 *
 * Pulse counting:
 *   TIM2 input capture logs timestamp of every Geiger pulse
 *   Individual timestamps sent over CAN — MCU board integrates to CPM
 *   Count rate also computed locally and sent every second
 */

#include "main.h"
#include "can_ids.h"
#include "flight_state.h"
#include "hv.h"
#include <stdio.h>
#include <string.h>

/* Peripheral handles */
FDCAN_HandleTypeDef  hfdcan1;
TIM_HandleTypeDef    htim1;
TIM_HandleTypeDef    htim2;
ADC_HandleTypeDef    hadc1;
UART_HandleTypeDef   huart1;

/* CAN receive */
FDCAN_RxHeaderTypeDef rx_header;
uint8_t rx_data[64];

/* Flight state — synced from MCU board */
volatile FlightState_t flight_state = STATE_GROUND;

/* Pulse ring buffer — stores timestamps of recent pulses */
#define PULSE_BUF_SIZE  64
volatile uint32_t pulse_timestamps[PULSE_BUF_SIZE];
volatile uint8_t  pulse_head = 0;
volatile uint8_t  pulse_tail = 0;
volatile uint32_t total_pulse_count = 0;

/* Forward declarations */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_Init(void);
static void CAN_FilterConfig(void);
static void Process_CAN_Message(void);
static void Send_Pulse(uint32_t timestamp_ms);
static void Send_Count_Rate(void);
static void Send_HV_Status(void);
static uint32_t Compute_CPM(void);

/* Printf to UART */
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_Init();

    printf("\r\n=== Geiger Board Firmware v0.1 ===\r\n");
    printf("STM32G0B1CCT6 | End-of-bus CAN termination fitted\r\n");

    MX_ADC1_Init();
    MX_TIM1_Init();
    MX_TIM2_Init();
    MX_FDCAN1_Init();
    CAN_FilterConfig();

    /* Init HV boost */
    HV_Init(&htim1);
    printf("[HV] Boost converter init\r\n");

    /* Start CAN */
    if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
        printf("[ERROR] CAN start failed\r\n");
        Error_Handler();
    }
    if (HAL_FDCAN_ActivateNotification(&hfdcan1,
        FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
        Error_Handler();
    printf("[OK] CAN-FD started\r\n");

    /* Start HV boost */
    HV_Start(&htim1);
    printf("[HV] Boost started — waiting for rail to stabilise\r\n");
    HAL_Delay(500); /* Allow HV rail to charge */

    /* Check HV rail */
    uint32_t hv_mv = HV_ReadRailMV(&hadc1);
    printf("[HV] Rail voltage: %lumV (target ~400000mV)\r\n",
           (unsigned long)hv_mv);
    if (hv_mv < 300000 || hv_mv > 500000)
        printf("[WARN] HV rail out of expected range\r\n");
    else
        printf("[OK] HV rail nominal\r\n");

    /* Start pulse input capture */
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    printf("[OK] Pulse capture started\r\n");

    printf("[OK] Geiger board init complete\r\n");

    uint32_t last_count_tick  = 0;
    uint32_t last_hv_tick     = 0;
    uint32_t last_status_tick = 0;
    uint32_t last_led_tick    = 0;
    uint8_t  led_state        = 0;

    while (1) {
        uint32_t now = HAL_GetTick();

        /* Process CAN — state sync from MCU board */
        if (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0)
            Process_CAN_Message();

        /* Drain pulse buffer — send each pulse over CAN */
        while (pulse_head != pulse_tail) {
            uint32_t ts = pulse_timestamps[pulse_tail];
            pulse_tail = (pulse_tail + 1) % PULSE_BUF_SIZE;
            Send_Pulse(ts);
        }

        /* Send count rate every 1 second */
        if (now - last_count_tick >= 1000) {
            last_count_tick = now;
            Send_Count_Rate();
        }

        /* Send HV status every 5 seconds */
        if (now - last_hv_tick >= 5000) {
            last_hv_tick = now;
            Send_HV_Status();
        }

        /* Debug status every 10 seconds */
        if (now - last_status_tick >= 10000) {
            last_status_tick = now;
            uint32_t cpm = Compute_CPM();
            uint32_t hv  = HV_ReadRailMV(&hadc1);
            printf("[STATUS] State: %s | CPM: %lu | HV: %lumV | Total: %lu\r\n",
                Flight_State_Name(flight_state),
                (unsigned long)cpm,
                (unsigned long)hv,
                (unsigned long)total_pulse_count);
        }

        /* LED blink rate varies with count rate
         * Ground: 1Hz slow blink
         * In flight: faster blink proportional to count rate */
        uint32_t blink_interval = (flight_state == STATE_GROUND) ? 1000 : 200;
        if (now - last_led_tick >= blink_interval) {
            last_led_tick = now;
            led_state = !led_state;
            HAL_GPIO_WritePin(LED_STAT_GPIO_Port, LED_STAT_Pin,
                led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
    }
}

/* ── TIM2 input capture interrupt — fires on every Geiger pulse ── */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance != TIM2) return;

    uint32_t ts = HAL_GetTick();

    /* Store in ring buffer */
    uint8_t next_head = (pulse_head + 1) % PULSE_BUF_SIZE;
    if (next_head != pulse_tail) { /* Buffer not full */
        pulse_timestamps[pulse_head] = ts;
        pulse_head = next_head;
    }
    total_pulse_count++;

    /* Quick LED flash on pulse — toggle briefly */
    HAL_GPIO_TogglePin(LED_STAT_GPIO_Port, LED_STAT_Pin);
}

/* ── CAN helpers ─────────────────────────────────────────────── */

static void Process_CAN_Message(void) {
    if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0,
        &rx_header, rx_data) != HAL_OK)
        return;

    switch (rx_header.Identifier) {
        case CAN_ID_STATE_CHANGE:
            flight_state = (FlightState_t)rx_data[1];
            printf("[CAN] State -> %s\r\n",
                Flight_State_Name(flight_state));
            break;
        default:
            break;
    }
}

static void Send_Pulse(uint32_t timestamp_ms) {
    FDCAN_TxHeaderTypeDef tx = {0};
    tx.Identifier    = CAN_ID_GEIGER_PULSE;
    tx.IdType        = FDCAN_STANDARD_ID;
    tx.TxFrameType   = FDCAN_DATA_FRAME;
    tx.DataLength    = FDCAN_DLC_BYTES_4;
    tx.BitRateSwitch = FDCAN_BRS_ON;
    tx.FDFormat      = FDCAN_FD_CAN;

    uint8_t buf[4];
    buf[0] = (timestamp_ms >> 24) & 0xFF;
    buf[1] = (timestamp_ms >> 16) & 0xFF;
    buf[2] = (timestamp_ms >> 8)  & 0xFF;
    buf[3] = (timestamp_ms)       & 0xFF;

    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx, buf);
}

static void Send_Count_Rate(void) {
    uint32_t cpm = Compute_CPM();

    typedef struct __attribute__((packed)) {
        uint32_t timestamp_ms;
        uint32_t cpm;
    } CountPacket_t;

    CountPacket_t pkt = {
        .timestamp_ms = HAL_GetTick(),
        .cpm          = cpm,
    };

    FDCAN_TxHeaderTypeDef tx = {0};
    tx.Identifier    = CAN_ID_GEIGER_COUNT;
    tx.IdType        = FDCAN_STANDARD_ID;
    tx.TxFrameType   = FDCAN_DATA_FRAME;
    tx.DataLength    = FDCAN_DLC_BYTES_8;
    tx.BitRateSwitch = FDCAN_BRS_ON;
    tx.FDFormat      = FDCAN_FD_CAN;
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx, (uint8_t*)&pkt);
}

static void Send_HV_Status(void) {
    uint32_t hv_mv = HV_ReadRailMV(&hadc1);

    FDCAN_TxHeaderTypeDef tx = {0};
    tx.Identifier    = CAN_ID_GEIGER_HV;
    tx.IdType        = FDCAN_STANDARD_ID;
    tx.TxFrameType   = FDCAN_DATA_FRAME;
    tx.DataLength    = FDCAN_DLC_BYTES_4;
    tx.BitRateSwitch = FDCAN_BRS_ON;
    tx.FDFormat      = FDCAN_FD_CAN;

    uint8_t buf[4];
    buf[0] = (hv_mv >> 24) & 0xFF;
    buf[1] = (hv_mv >> 16) & 0xFF;
    buf[2] = (hv_mv >> 8)  & 0xFF;
    buf[3] = (hv_mv)       & 0xFF;

    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx, buf);
}

static uint32_t Compute_CPM(void) {
    /* Count pulses in the last 60 seconds from ring buffer timestamps
     * Simple approach: count entries within last 60000ms */
    uint32_t now = HAL_GetTick();
    uint32_t count = 0;
    uint8_t i = pulse_tail;
    while (i != pulse_head) {
        if ((now - pulse_timestamps[i]) <= 60000)
            count++;
        i = (i + 1) % PULSE_BUF_SIZE;
    }
    /* Scale to CPM if less than 60 seconds have elapsed */
    if (now < 60000 && now > 0)
        count = count * 60000 / now;
    return count;
}

/* ── Peripheral init ─────────────────────────────────────────── */

static void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN       = 8;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR       = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

static void MX_FDCAN1_Init(void) {
    hfdcan1.Instance                  = FDCAN1;
    hfdcan1.Init.ClockDivider         = FDCAN_CLOCK_DIV1;
    hfdcan1.Init.FrameFormat          = FDCAN_FRAME_FD_BRS;
    hfdcan1.Init.Mode                 = FDCAN_MODE_NORMAL;
    hfdcan1.Init.AutoRetransmission   = ENABLE;
    hfdcan1.Init.TransmitPause        = DISABLE;
    hfdcan1.Init.ProtocolException    = ENABLE;
    hfdcan1.Init.NominalPrescaler     = 8;
    hfdcan1.Init.NominalSyncJumpWidth = 4;
    hfdcan1.Init.NominalTimeSeg1      = 11;
    hfdcan1.Init.NominalTimeSeg2      = 4;
    hfdcan1.Init.DataPrescaler        = 2;
    hfdcan1.Init.DataSyncJumpWidth    = 4;
    hfdcan1.Init.DataTimeSeg1         = 11;
    hfdcan1.Init.DataTimeSeg2         = 4;
    hfdcan1.Init.StdFiltersNbr        = 8;
    hfdcan1.Init.ExtFiltersNbr        = 0;
    hfdcan1.Init.TxFifoQueueMode      = FDCAN_TX_FIFO_OPERATION;
    if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK) Error_Handler();
}

static void CAN_FilterConfig(void) {
    FDCAN_FilterTypeDef filter = {0};
    filter.IdType       = FDCAN_STANDARD_ID;
    filter.FilterIndex  = 0;
    filter.FilterType   = FDCAN_FILTER_RANGE;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1    = 0x000;
    filter.FilterID2    = 0x7FF;
    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &filter) != HAL_OK) Error_Handler();
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1,
        FDCAN_REJECT, FDCAN_REJECT,
        FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
}

static void MX_TIM1_Init(void) {
    /* TIM1 CH1 PA8 — HV boost PWM
     * 64MHz / 64 = 1MHz timer clock
     * Period = 1MHz / 30kHz = 33 → ~30.3kHz */
    TIM_OC_InitTypeDef oc = {0};
    TIM_MasterConfigTypeDef master = {0};
    TIM_BreakDeadTimeConfigTypeDef bdt = {0};

    htim1.Instance               = TIM1;
    htim1.Init.Prescaler         = 63;
    htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim1.Init.Period            = 32;   /* 1MHz / 33 ≈ 30.3kHz */
    htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) Error_Handler();

    master.MasterOutputTrigger  = TIM_TRGO_RESET;
    master.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    master.MasterSlaveMode      = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &master) != HAL_OK)
        Error_Handler();

    oc.OCMode       = TIM_OCMODE_PWM1;
    oc.Pulse        = 16;  /* 50% duty cycle */
    oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode   = TIM_OCFAST_DISABLE;
    oc.OCIdleState  = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_1) != HAL_OK)
        Error_Handler();

    bdt.OffStateRunMode  = TIM_OSSR_DISABLE;
    bdt.OffStateIDLEMode = TIM_OSSI_DISABLE;
    bdt.LockLevel        = TIM_LOCKLEVEL_OFF;
    bdt.DeadTime         = 0;
    bdt.BreakState       = TIM_BREAK_DISABLE;
    bdt.BreakPolarity    = TIM_BREAKPOLARITY_HIGH;
    bdt.AutomaticOutput  = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &bdt) != HAL_OK)
        Error_Handler();
}

static void MX_TIM2_Init(void) {
    /* TIM2 CH1 PA0 — input capture for Geiger pulses
     * Free-running at 1MHz for microsecond resolution timestamps */
    TIM_IC_InitTypeDef ic = {0};
    TIM_MasterConfigTypeDef master = {0};

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 63;       /* 64MHz / 64 = 1MHz */
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 0xFFFFFFFF; /* 32-bit free running */
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_IC_Init(&htim2) != HAL_OK) Error_Handler();

    master.MasterOutputTrigger = TIM_TRGO_RESET;
    master.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &master) != HAL_OK)
        Error_Handler();

    ic.ICPolarity  = TIM_ICPOLARITY_RISING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter    = 0x08; /* Some input filtering to debounce */
    if (HAL_TIM_IC_ConfigChannel(&htim2, &ic, TIM_CHANNEL_1) != HAL_OK)
        Error_Handler();
}

static void MX_ADC1_Init(void) {
    ADC_ChannelConfTypeDef chan = {0};

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait      = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.OversamplingMode      = ENABLE;
    hadc1.Init.Oversampling.Ratio                 = ADC_OVERSAMPLING_RATIO_16;
    hadc1.Init.Oversampling.RightBitShift         = ADC_RIGHTBITSHIFT_4;
    hadc1.Init.Oversampling.TriggeredMode         = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
    hadc1.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) Error_Handler();

    chan.Channel      = ADC_CHANNEL_1; /* PA1 — HV monitor */
    chan.Rank         = ADC_REGULAR_RANK_1;
    chan.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    if (HAL_ADC_ConfigChannel(&hadc1, &chan) != HAL_OK) Error_Handler();

    HAL_ADCEx_Calibration_Start(&hadc1);
}

static void MX_USART1_Init(void) {
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* LED_STAT — PA15 */
    HAL_GPIO_WritePin(LED_STAT_GPIO_Port, LED_STAT_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = LED_STAT_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_STAT_GPIO_Port, &GPIO_InitStruct);

    /* CAN_STB — PB5, drive low */
    HAL_GPIO_WritePin(CAN_STB_GPIO_Port, CAN_STB_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = CAN_STB_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CAN_STB_GPIO_Port, &GPIO_InitStruct);
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(LED_STAT_GPIO_Port, LED_STAT_Pin);
        HAL_Delay(100);
    }
}
