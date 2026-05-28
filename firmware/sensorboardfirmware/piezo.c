/* piezo.c — Piezo sweep driver implementation */

#include "piezo.h"
#include <stdio.h>

static uint8_t sweep_active = 0;
static uint32_t current_freq_hz = 0;

void Piezo_Init(TIM_HandleTypeDef *htim) {
    sweep_active = 0;
    current_freq_hz = PIEZO_SWEEP_START_HZ;
    /* Timer already configured in MX_TIM1_Init — just confirm it's stopped */
    HAL_TIM_PWM_Stop(htim, TIM_CHANNEL_1);
}

void Piezo_Start(TIM_HandleTypeDef *htim) {
    if (sweep_active) return;
    sweep_active = 1;
    current_freq_hz = PIEZO_SWEEP_START_HZ;
    Piezo_SetFreq(htim, current_freq_hz);
    HAL_TIM_PWM_Start(htim, TIM_CHANNEL_1);
    printf("[PIEZO] Sweep started %luHz -> %luHz\r\n",
        (unsigned long)PIEZO_SWEEP_START_HZ,
        (unsigned long)PIEZO_SWEEP_END_HZ);
}

void Piezo_Stop(TIM_HandleTypeDef *htim) {
    sweep_active = 0;
    HAL_TIM_PWM_Stop(htim, TIM_CHANNEL_1);
    printf("[PIEZO] Sweep stopped\r\n");
}

void Piezo_SetFreq(TIM_HandleTypeDef *htim, uint32_t freq_hz) {
    if (freq_hz == 0) return;

    /* Period = TIM_CLK / freq - 1 */
    uint32_t period = (PIEZO_TIM_CLK_HZ / freq_hz) - 1;
    uint32_t pulse  = period * PIEZO_DUTY_PCT / 100;

    __HAL_TIM_SET_AUTORELOAD(htim, period);
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, pulse);
    current_freq_hz = freq_hz;
}

void Piezo_RunSweep(TIM_HandleTypeDef *htim,
                    FDCAN_HandleTypeDef *hfdcan) {
    if (!sweep_active) return;

    static uint32_t last_step_tick = 0;
    static uint32_t sweep_freq = PIEZO_SWEEP_START_HZ;
    static uint32_t sweep_start_tick = 0;

    uint32_t now = HAL_GetTick();

    /* Dwell at current frequency */
    if (now - last_step_tick < PIEZO_STEP_DWELL_MS) return;
    last_step_tick = now;

    /* Advance frequency */
    sweep_freq += PIEZO_SWEEP_STEP_HZ;

    /* End of sweep — wrap back to start and send sweep metadata over CAN */
    if (sweep_freq > PIEZO_SWEEP_END_HZ) {

        /* Send sweep metadata — MCU board logs this with altitude */
        typedef struct __attribute__((packed)) {
            uint32_t start_hz;
            uint32_t end_hz;
            uint32_t duration_ms;
        } SweepMeta_t;

        SweepMeta_t meta = {
            .start_hz    = PIEZO_SWEEP_START_HZ,
            .end_hz      = PIEZO_SWEEP_END_HZ,
            .duration_ms = now - sweep_start_tick,
        };

        FDCAN_TxHeaderTypeDef tx = {0};
        tx.Identifier    = CAN_ID_SENSOR_SWEEP;
        tx.IdType        = FDCAN_STANDARD_ID;
        tx.TxFrameType   = FDCAN_DATA_FRAME;
        tx.DataLength    = FDCAN_DLC_BYTES_12;
        tx.BitRateSwitch = FDCAN_BRS_ON;
        tx.FDFormat      = FDCAN_FD_CAN;
        HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &tx, (uint8_t*)&meta);

        sweep_freq = PIEZO_SWEEP_START_HZ;
        sweep_start_tick = now;
    }

    Piezo_SetFreq(htim, sweep_freq);
}
