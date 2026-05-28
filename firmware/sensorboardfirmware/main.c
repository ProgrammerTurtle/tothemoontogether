/* Sensor Board Firmware — Ring PCB B
 * STM32G0B1CCT6
 * Placeholder firmware for hardware bring-up
 *
 * Peripherals:
 *   - FDCAN1:  CAN-FD bus, 500kbps arbitration / 2Mbps data
 *   - I2C1:    Sensor bus (PB6 SCL, PB7 SDA)
 *              MS5607 @ 0x76, BMP390 @ 0x77, SHT40 @ 0x44, LSM6DSO32 @ 0x6A
 *   - ADC1:    PA0 charge HI, PA1 charge LO (LMP7721), PA2 nosecone thermistor
 *   - TIM1:    CH1 PA8 — piezo swept sine PWM
 *   - USART1:  Debug UART (PB6 TX, PB7 RX) — NOTE: shared pins with I2C1
 *              Use USART1 only before I2C1 is started, or remap to different pins
 *   - GPIO:    PA3 ADXL_INT1, PA4 LSM_INT1, PA15 LED_STAT, PB5 CAN_STB
 *
 * NOTE on USART/I2C pin conflict:
 *   PB6/PB7 are shared between USART1 and I2C1 on G0B1.
 *   In this firmware USART1 is used for early boot debug only.
 *   I2C1 takes over after sensor init. Remap USART to PA9/PA10 in CubeMX
 *   if simultaneous debug + I2C is needed.
 */

#include "main.h"
#include "can_ids.h"
#include "flight_state.h"
#include "sensors.h"
#include "charge.h"
#include "piezo.h"
#include <stdio.h>
#include <string.h>

/* Peripheral handles */
FDCAN_HandleTypeDef  hfdcan1;
I2C_HandleTypeDef    hi2c1;
ADC_HandleTypeDef    hadc1;
TIM_HandleTypeDef    htim1;
UART_HandleTypeDef   huart1;

/* CAN receive */
FDCAN_RxHeaderTypeDef rx_header;
uint8_t rx_data[64];

/* Flight state — synced from MCU board via CAN */
volatile FlightState_t flight_state = STATE_GROUND;

/* Forward declarations */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART1_Init(void);
static void CAN_FilterConfig(void);
static void Process_CAN_Message(void);
static void Send_Sensor_Data(void);
static void Send_Charge_Data(void);
static void Send_Thermistor_Data(void);

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

    printf("\r\n=== Sensor Board Firmware v0.1 ===\r\n");
    printf("STM32G0B1CCT6\r\n");

    MX_I2C1_Init();
    MX_ADC1_Init();
    MX_TIM1_Init();
    MX_FDCAN1_Init();
    CAN_FilterConfig();

    /* Init sensors */
    printf("[SENSOR] Initialising sensors...\r\n");

    if (MS5607_Init(&hi2c1) != SENSOR_OK)
        printf("[WARN] MS5607 init failed\r\n");
    else
        printf("[OK] MS5607\r\n");

    if (BMP390_Init(&hi2c1) != SENSOR_OK)
        printf("[WARN] BMP390 init failed\r\n");
    else
        printf("[OK] BMP390\r\n");

    if (SHT40_Init(&hi2c1) != SENSOR_OK)
        printf("[WARN] SHT40 init failed\r\n");
    else
        printf("[OK] SHT40\r\n");

    if (LSM6DSO32_Init(&hi2c1) != SENSOR_OK)
        printf("[WARN] LSM6DSO32 init failed\r\n");
    else
        printf("[OK] LSM6DSO32\r\n");

    /* Init charge frontend */
    Charge_Init(&hadc1);
    printf("[OK] Charge frontend\r\n");

    /* Start CAN */
    if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
        printf("[ERROR] CAN start failed\r\n");
        Error_Handler();
    }
    if (HAL_FDCAN_ActivateNotification(&hfdcan1,
        FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
        Error_Handler();
    printf("[OK] CAN-FD started\r\n");

    /* Piezo init — don't start sweep yet */
    Piezo_Init(&htim1);
    printf("[OK] Piezo driver ready\r\n");

    printf("[OK] Sensor board init complete\r\n");

    /* Timing */
    uint32_t last_baro_tick    = 0;
    uint32_t last_imu_tick     = 0;
    uint32_t last_charge_tick  = 0;
    uint32_t last_env_tick     = 0;
    uint32_t last_therm_tick   = 0;
    uint32_t last_status_tick  = 0;

    while (1) {
        uint32_t now = HAL_GetTick();

        /* Process incoming CAN — mainly state change from MCU board */
        if (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0)
            Process_CAN_Message();

        /* Barometric — 10Hz */
        if (now - last_baro_tick >= 100) {
            last_baro_tick = now;
            Send_Sensor_Data();
        }

        /* IMU — rate depends on flight state */
        uint32_t imu_rate = (flight_state == STATE_BOOST ||
                             flight_state == STATE_COAST) ? 1 : 10;
        if (now - last_imu_tick >= imu_rate) {
            last_imu_tick = now;
            Sensors_Send_IMU(&hfdcan1, &hi2c1, flight_state);
        }

        /* Charge measurement — 100Hz */
        if (now - last_charge_tick >= 10) {
            last_charge_tick = now;
            Send_Charge_Data();
        }

        /* Humidity + temp — 1Hz */
        if (now - last_env_tick >= 1000) {
            last_env_tick = now;
            Sensors_Send_Env(&hfdcan1, &hi2c1);
        }

        /* Nosecone thermistor — 2Hz */
        if (now - last_therm_tick >= 500) {
            last_therm_tick = now;
            Send_Thermistor_Data();
        }

        /* Status print — 5s */
        if (now - last_status_tick >= 5000) {
            last_status_tick = now;
            printf("[STATUS] State: %s | tick: %lu\r\n",
                Flight_State_Name(flight_state), now);
        }

        /* Piezo sweep — only during descent */
        if (flight_state == STATE_DESCENT)
            Piezo_RunSweep(&htim1, &hfdcan1);
    }
}

static void Process_CAN_Message(void) {
    if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0,
        &rx_header, rx_data) != HAL_OK)
        return;

    switch (rx_header.Identifier) {

        case CAN_ID_STATE_CHANGE:
            /* MCU board broadcasting new flight state */
            flight_state = (FlightState_t)rx_data[1];
            printf("[CAN] State -> %s\r\n",
                Flight_State_Name(flight_state));

            /* Start/stop piezo based on state */
            if (flight_state == STATE_DESCENT)
                Piezo_Start(&htim1);
            else
                Piezo_Stop(&htim1);
            break;

        default:
            break;
    }
}

static void Send_Sensor_Data(void) {
    typedef struct __attribute__((packed)) {
        int32_t  pressure_pa;     /* Pascal * 100 fixed point */
        int32_t  altitude_mm;     /* mm above sea level */
        int32_t  temperature_mc;  /* millidegrees C */
    } BaroPacket_t;

    BaroPacket_t pkt = {0};

    /* Read MS5607 */
    MS5607_Data_t ms_data;
    if (MS5607_Read(&hi2c1, &ms_data) == SENSOR_OK) {
        pkt.pressure_pa    = ms_data.pressure_pa;
        pkt.altitude_mm    = ms_data.altitude_mm;
        pkt.temperature_mc = ms_data.temperature_mc;
    }

    FDCAN_TxHeaderTypeDef tx = {0};
    tx.Identifier    = CAN_ID_SENSOR_BARO;
    tx.IdType        = FDCAN_STANDARD_ID;
    tx.TxFrameType   = FDCAN_DATA_FRAME;
    tx.DataLength    = FDCAN_DLC_BYTES_12;
    tx.BitRateSwitch = FDCAN_BRS_ON;
    tx.FDFormat      = FDCAN_FD_CAN;
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx, (uint8_t*)&pkt);
}

static void Send_Charge_Data(void) {
    typedef struct __attribute__((packed)) {
        int32_t hi_range_mv;   /* High range divider output, millivolts */
        int32_t lo_range_uv;   /* LMP7721 output, microvolts */
    } ChargePacket_t;

    ChargePacket_t pkt = {0};
    Charge_Read(&hadc1, &pkt.hi_range_mv, &pkt.lo_range_uv);

    FDCAN_TxHeaderTypeDef tx = {0};
    tx.Identifier    = CAN_ID_SENSOR_CHARGE;
    tx.IdType        = FDCAN_STANDARD_ID;
    tx.TxFrameType   = FDCAN_DATA_FRAME;
    tx.DataLength    = FDCAN_DLC_BYTES_8;
    tx.BitRateSwitch = FDCAN_BRS_ON;
    tx.FDFormat      = FDCAN_FD_CAN;
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx, (uint8_t*)&pkt);
}

static void Send_Thermistor_Data(void) {
    /* Read raw ADC on PA2, convert to temperature via lookup table */
    typedef struct __attribute__((packed)) {
        int32_t tip_temp_mc;  /* millidegrees C */
    } ThermPacket_t;

    ThermPacket_t pkt = {0};
    pkt.tip_temp_mc = Charge_ReadThermistor(&hadc1);

    FDCAN_TxHeaderTypeDef tx = {0};
    tx.Identifier    = CAN_ID_SENSOR_THERM;
    tx.IdType        = FDCAN_STANDARD_ID;
    tx.TxFrameType   = FDCAN_DATA_FRAME;
    tx.DataLength    = FDCAN_DLC_BYTES_4;
    tx.BitRateSwitch = FDCAN_BRS_ON;
    tx.FDFormat      = FDCAN_FD_CAN;
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx, (uint8_t*)&pkt);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM1) {
        HAL_GPIO_TogglePin(LED_STAT_GPIO_Port, LED_STAT_Pin);
    }
}

/* ── Peripheral init ─────────────────────────────────────────────────────── */

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

static void MX_I2C1_Init(void) {
    hi2c1.Instance              = I2C1;
    hi2c1.Init.Timing           = 0x10707DBC; /* 100kHz @ 64MHz — verify with CubeMX */
    hi2c1.Init.OwnAddress1      = 0;
    hi2c1.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
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
    /* 16x hardware oversampling — reduces noise on sensitive inputs */
    hadc1.Init.Oversampling.Ratio                 = ADC_OVERSAMPLING_RATIO_16;
    hadc1.Init.Oversampling.RightBitShift         = ADC_RIGHTBITSHIFT_4;
    hadc1.Init.Oversampling.TriggeredMode         = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
    hadc1.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) Error_Handler();

    /* Default channel — PA0 charge HI */
    chan.Channel      = ADC_CHANNEL_0;
    chan.Rank         = ADC_REGULAR_RANK_1;
    chan.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    if (HAL_ADC_ConfigChannel(&hadc1, &chan) != HAL_OK) Error_Handler();

    HAL_ADCEx_Calibration_Start(&hadc1);
}

static void MX_TIM1_Init(void) {
    TIM_OC_InitTypeDef oc = {0};
    TIM_MasterConfigTypeDef master = {0};

    /* TIM1 CH1 on PA8 — piezo PWM
     * Base frequency: 64MHz / (prescaler+1) / (period+1)
     * Set for 10kHz start — piezo sweep firmware adjusts period dynamically */
    htim1.Instance               = TIM1;
    htim1.Init.Prescaler         = 63;    /* 64MHz / 64 = 1MHz timer clock */
    htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim1.Init.Period            = 99;    /* 1MHz / 100 = 10kHz */
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
    oc.Pulse        = 50;  /* 50% duty cycle */
    oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode   = TIM_OCFAST_DISABLE;
    oc.OCIdleState  = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_1) != HAL_OK)
        Error_Handler();
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

    /* CAN_STB — PB5 */
    HAL_GPIO_WritePin(CAN_STB_GPIO_Port, CAN_STB_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = CAN_STB_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CAN_STB_GPIO_Port, &GPIO_InitStruct);

    /* LSM_INT1 — PA4, input */
    GPIO_InitStruct.Pin  = LSM_INT1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LSM_INT1_GPIO_Port, &GPIO_InitStruct);
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(LED_STAT_GPIO_Port, LED_STAT_Pin);
        HAL_Delay(100);
    }
}
