/* piezo.h — Piezo actuator driver for structural resonance sweep
 * TIM1 CH1 on PA8 drives BSS138 MOSFET at 12V
 * Active sweep only during STATE_DESCENT
 * Passive accelerometer logging during STATE_BOOST / STATE_COAST
 */

#ifndef __PIEZO_H
#define __PIEZO_H

#include "main.h"
#include "can_ids.h"
#include <stdint.h>

/* Sweep parameters — tune after bench characterisation */
#define PIEZO_SWEEP_START_HZ    200     /* Start frequency Hz */
#define PIEZO_SWEEP_END_HZ      5000    /* End frequency Hz */
#define PIEZO_SWEEP_STEP_HZ     10      /* Step size Hz */
#define PIEZO_STEP_DWELL_MS     20      /* Time at each frequency ms */

/* TIM1 clock after prescaler — 64MHz / 64 = 1MHz */
#define PIEZO_TIM_CLK_HZ        1000000

/* Duty cycle — 50% */
#define PIEZO_DUTY_PCT          50

/* Public API */
void Piezo_Init(TIM_HandleTypeDef *htim);
void Piezo_Start(TIM_HandleTypeDef *htim);
void Piezo_Stop(TIM_HandleTypeDef *htim);
void Piezo_SetFreq(TIM_HandleTypeDef *htim, uint32_t freq_hz);
void Piezo_RunSweep(TIM_HandleTypeDef *htim,
                    FDCAN_HandleTypeDef *hfdcan);

#endif /* __PIEZO_H */
