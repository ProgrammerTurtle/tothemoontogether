/* MCU Board Firmware — Ring PCB A
 * STM32G0B1CCT6
 * Placeholder firmware for hardware bring-up
 *
 * Peripherals:
 *   - FDCAN1: CAN-FD bus, 500kbps arbitration / 2Mbps data
 *   - SPI1: W25Q128 NOR flash (PA5 SCK, PA6 MISO, PB5 MOSI, PA4 CS)
 *   - USART1: Debug UART via CH340C (PB6 TX, PB7 RX), 115200 baud
 *   - USB: Full speed device (PA11 DM, PA12 DP)
 *   - TIM2: 1Hz LED heartbeat
 *   - GPIO: PA15 LED_STAT
 */

#include "main.h"
#include "can_ids.h"
#include "flash.h"
#include "flight_state.h"
#include <stdio.h>
#include <string.h>

/* Peripheral handles */
FDCAN_HandleTypeDef hfdcan1;
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim2;

/* CAN receive FIFO */
FDCAN_RxHeaderTypeDef rx_header;
uint8_t rx_data[64];

/* Flight state */
volatile FlightState_t flight_state = STATE_GROUND;
volatile uint32_t state_entry_tick = 0;

/* Forward declarations */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_Init(void);
static void MX_TIM2_Init(void);
static void CAN_FilterConfig(void);
static void Process_CAN_Message(void);
static void Update_Flight_State(void);
static void Log_Housekeeping(void);

/* Printf redirect to UART */
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_Init();
    MX_SPI1_Init();
    MX_TIM2_Init();
    MX_FDCAN1_Init();
    CAN_FilterConfig();

    printf("\r\n=== MCU Board Firmware v0.1 ===\r\n");
    printf("STM32G0B1CCT6 | CAN-FD 500k/2M\r\n");

    /* Flash init */
    if (Flash_Init() != FLASH_OK) {
        printf("[ERROR] Flash init failed\r\n");
    } else {
        printf("[OK] W25Q128 flash init\r\n");
        uint32_t flash_id = Flash_ReadJEDECID();
        printf("[OK] Flash JEDEC ID: 0x%06lX\r\n", flash_id);
    }

    /* Start CAN */
    if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
        printf("[ERROR] CAN start failed\r\n");
        Error_Handler();
    }
    if (HAL_FDCAN_ActivateNotification(&hfdcan1,
        FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) {
        Error_Handler();
    }
    printf("[OK] CAN-FD started\r\n");

    /* Start heartbeat timer */
    HAL_TIM_Base_Start_IT(&htim2);

    printf("[OK] Init complete — entering main loop\r\n");
    printf("Flight state: GROUND\r\n");

    uint32_t last_hk_tick = 0;
    uint32_t last_state_print = 0;

    while (1) {
        /* Process incoming CAN messages */
        if (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0) {
            Process_CAN_Message();
        }

        /* Update flight state machine */
        Update_Flight_State();

        /* Log housekeeping to flash every 1 second */
        if (HAL_GetTick() - last_hk_tick >= 1000) {
            last_hk_tick = HAL_GetTick();
            Log_Housekeeping();
        }

        /* Print state periodically for debug */
        if (HAL_GetTick() - last_state_print >= 5000) {
            last_state_print = HAL_GetTick();
            printf("[STATE] %s | tick: %lu\r\n",
                Flight_State_Name(flight_state),
                HAL_GetTick());
        }
    }
}

static void Process_CAN_Message(void) {
    if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0,
        &rx_header, rx_data) != HAL_OK) {
        return;
    }

    uint32_t id = rx_header.Identifier;
    uint8_t dlc = rx_header.DataLength >> 16; /* Extract DLC */

    switch (id) {

        case CAN_ID_SENSOR_BARO:
            /* Barometric data from sensor board */
            /* TODO: parse and log pressure/altitude/temp */
            printf("[CAN] Baro data received (%d bytes)\r\n", dlc);
            Flash_WriteRecord(id, rx_data, dlc);
            break;

        case CAN_ID_SENSOR_IMU_HI:
            /* High-G accelerometer from sensor board */
            Flash_WriteRecord(id, rx_data, dlc);
            break;

        case CAN_ID_SENSOR_IMU_LO:
            /* Low-G accelerometer from sensor board */
            Flash_WriteRecord(id, rx_data, dlc);
            break;

        case CAN_ID_SENSOR_CHARGE:
            /* Charge measurement from sensor board */
            Flash_WriteRecord(id, rx_data, dlc);
            break;

        case CAN_ID_SENSOR_ENV:
            /* Humidity/temp from sensor board */
            Flash_WriteRecord(id, rx_data, dlc);
            break;

        case CAN_ID_SENSOR_THERM:
            /* Nosecone thermistor from sensor board */
            Flash_WriteRecord(id, rx_data, dlc);
            break;

        case CAN_ID_GEIGER_PULSE:
            /* Geiger pulse timestamp from Geiger board */
            Flash_WriteRecord(id, rx_data, dlc);
            break;

        case CAN_ID_GEIGER_STATUS:
            /* Geiger board HV status */
            printf("[CAN] Geiger status received\r\n");
            Flash_WriteRecord(id, rx_data, dlc);
            break;

        case CAN_ID_STATE_CHANGE:
            /* Another board reporting state change */
            printf("[CAN] State change received from board\r\n");
            break;

        default:
            printf("[CAN] Unknown ID: 0x%03lX\r\n", id);
            break;
    }
}

static void Update_Flight_State(void) {
    /* TODO: implement full state machine
     * For now just a placeholder that stays in GROUND
     * Real implementation will use:
     *   - Baro data from CAN for pressure derivative apogee detect
     *   - IMU data for launch detect (accel > 3G) and landed detect
     *   - Broadcast state changes over CAN so all boards sync
     */

    static FlightState_t last_state = STATE_GROUND;

    if (flight_state != last_state) {
        printf("[STATE] Transition: %s -> %s\r\n",
            Flight_State_Name(last_state),
            Flight_State_Name(flight_state));

        /* Broadcast state change to all boards */
        FDCAN_TxHeaderTypeDef tx_header = {0};
        uint8_t tx_data[2];
        tx_header.Identifier = CAN_ID_STATE_CHANGE;
        tx_header.IdType = FDCAN_STANDARD_ID;
        tx_header.TxFrameType = FDCAN_DATA_FRAME;
        tx_header.DataLength = FDCAN_DLC_BYTES_2;
        tx_header.BitRateSwitch = FDCAN_BRS_ON;
        tx_header.FDFormat = FDCAN_FD_CAN;
        tx_data[0] = (uint8_t)last_state;
        tx_data[1] = (uint8_t)flight_state;
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, tx_data);

        last_state = flight_state;
    }
}

static void Log_Housekeeping(void) {
    /* Log MCU board housekeeping record to flash */
    typedef struct __attribute__((packed)) {
        uint32_t timestamp_ms;
        uint8_t  flight_state;
        uint8_t  reserved[3];
    } HK_Record_t;

    HK_Record_t hk = {
        .timestamp_ms = HAL_GetTick(),
        .flight_state = (uint8_t)flight_state,
        .reserved = {0}
    };

    Flash_WriteRecord(CAN_ID_MCU_HK, (uint8_t*)&hk, sizeof(hk));
}

/* TIM2 interrupt — 1Hz heartbeat */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        HAL_GPIO_TogglePin(LED_STAT_GPIO_Port, LED_STAT_Pin);
    }
}

/* ── Peripheral init ──────────────────────────────────────────────────────── */

static void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Use HSE (external ceramic resonator 8MHz) */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN = 8;  /* 8MHz * 8 = 64MHz */
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

static void MX_FDCAN1_Init(void) {
    hfdcan1.Instance = FDCAN1;
    hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    hfdcan1.Init.FrameFormat = FDCAN_FRAME_FD_BRS;
    hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan1.Init.AutoRetransmission = ENABLE;
    hfdcan1.Init.TransmitPause = DISABLE;
    hfdcan1.Init.ProtocolException = ENABLE;

    /* Arbitration phase: 500kbps @ 64MHz
     * Prescaler=8, TimeSeg1=11, TimeSeg2=4, SJW=4
     * Bit time = (1+11+4) * (8/64MHz) = 2us = 500kbps */
    hfdcan1.Init.NominalPrescaler = 8;
    hfdcan1.Init.NominalSyncJumpWidth = 4;
    hfdcan1.Init.NominalTimeSeg1 = 11;
    hfdcan1.Init.NominalTimeSeg2 = 4;

    /* Data phase: 2Mbps @ 64MHz
     * Prescaler=2, TimeSeg1=11, TimeSeg2=4, SJW=4
     * Bit time = (1+11+4) * (2/64MHz) = 500ns = 2Mbps */
    hfdcan1.Init.DataPrescaler = 2;
    hfdcan1.Init.DataSyncJumpWidth = 4;
    hfdcan1.Init.DataTimeSeg1 = 11;
    hfdcan1.Init.DataTimeSeg2 = 4;

    hfdcan1.Init.StdFiltersNbr = 8;
    hfdcan1.Init.ExtFiltersNbr = 0;
    hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK) Error_Handler();
}

static void CAN_FilterConfig(void) {
    FDCAN_FilterTypeDef filter = {0};

    /* Accept all standard frames into FIFO0 for now
     * TODO: set specific filters per message ID */
    filter.IdType = FDCAN_STANDARD_ID;
    filter.FilterIndex = 0;
    filter.FilterType = FDCAN_FILTER_RANGE;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1 = 0x000;
    filter.FilterID2 = 0x7FF;
    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &filter) != HAL_OK) Error_Handler();

    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1,
        FDCAN_REJECT, FDCAN_REJECT,
        FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
}

static void MX_SPI1_Init(void) {
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4; /* 16MHz */
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) Error_Handler();
}

static void MX_USART1_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

static void MX_TIM2_Init(void) {
    TIM_ClockConfigTypeDef clk = {0};
    TIM_MasterConfigTypeDef master = {0};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 63999;      /* 64MHz / 64000 = 1kHz */
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 999;           /* 1kHz / 1000 = 1Hz */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) Error_Handler();

    clk.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &clk) != HAL_OK) Error_Handler();

    master.MasterOutputTrigger = TIM_TRGO_RESET;
    master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &master) != HAL_OK)
        Error_Handler();
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* LED_STAT — PA15 */
    HAL_GPIO_WritePin(LED_STAT_GPIO_Port, LED_STAT_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = LED_STAT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_STAT_GPIO_Port, &GPIO_InitStruct);

    /* FLASH_CS — PA4, default high */
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = FLASH_CS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(FLASH_CS_GPIO_Port, &GPIO_InitStruct);

    /* CAN_STB — PB5, drive low for normal operation */
    HAL_GPIO_WritePin(CAN_STB_GPIO_Port, CAN_STB_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = CAN_STB_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CAN_STB_GPIO_Port, &GPIO_InitStruct);
}

void Error_Handler(void) {
    __disable_irq();
    /* Fast blink on error */
    while (1) {
        HAL_GPIO_TogglePin(LED_STAT_GPIO_Port, LED_STAT_Pin);
        HAL_Delay(100);
    }
}
