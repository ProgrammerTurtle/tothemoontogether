/* hv.c — High voltage boost driver implementation */

#include "hv.h"
#include <stdio.h>

void HV_Init(TIM_HandleTypeDef *htim) {
    /* Timer configured in MX_TIM1_Init — just make sure PWM is stopped */
    HAL_TIM_PWM_Stop(htim, TIM_CHANNEL_1);
    printf("[HV] Init complete\r\n");
}

void HV_Start(TIM_HandleTypeDef *htim) {
    HAL_TIM_PWM_Start(htim, TIM_CHANNEL_1);
    printf("[HV] PWM started ~30kHz\r\n");
}

void HV_Stop(TIM_HandleTypeDef *htim) {
    HAL_TIM_PWM_Stop(htim, TIM_CHANNEL_1);
    printf("[HV] PWM stopped\r\n");
}

uint32_t HV_ReadRailMV(ADC_HandleTypeDef *hadc) {
    /* Read PA1 ADC — HV monitor divider midpoint */
    ADC_ChannelConfTypeDef cfg = {0};
    cfg.Channel      = HV_MON_CHANNEL;
    cfg.Rank         = ADC_REGULAR_RANK_1;
    cfg.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    HAL_ADC_ConfigChannel(hadc, &cfg);

    HAL_ADC_Start(hadc);
    HAL_ADC_PollForConversion(hadc, 100);
    uint32_t raw = HAL_ADC_GetValue(hadc);
    HAL_ADC_Stop(hadc);

    /* Convert raw ADC to millivolts at divider midpoint */
    uint32_t adc_mv = (uint32_t)((uint64_t)raw * ADC_VREF_MV / ADC_RESOLUTION);

    /* Scale back through divider to get HV rail voltage */
    return adc_mv * HV_DIVIDER_SCALE;
}

uint8_t HV_IsNominal(ADC_HandleTypeDef *hadc) {
    uint32_t hv_mv = HV_ReadRailMV(hadc);
    return (hv_mv >= HV_MIN_MV && hv_mv <= HV_MAX_MV) ? 1 : 0;
}
