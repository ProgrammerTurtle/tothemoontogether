/* hv.h — High voltage boost driver
 * MPSA42 flyback topology
 * TIM1 CH1 PA8 drives MPSA42 base via 10kΩ resistor
 * Target output: ~400V for SBM-21 GM tube
 * Frequency: ~30kHz
 */

#ifndef __HV_H
#define __HV_H

#include "main.h"
#include <stdint.h>

/* Expected HV rail voltage range */
#define HV_TARGET_MV        400000   /* 400V target */
#define HV_MIN_MV           350000   /* Below this = fault */
#define HV_MAX_MV           450000   /* Above this = fault */

/* Public API */
void     HV_Init(TIM_HandleTypeDef *htim);
void     HV_Start(TIM_HandleTypeDef *htim);
void     HV_Stop(TIM_HandleTypeDef *htim);
uint32_t HV_ReadRailMV(ADC_HandleTypeDef *hadc);
uint8_t  HV_IsNominal(ADC_HandleTypeDef *hadc);

#endif /* __HV_H */
