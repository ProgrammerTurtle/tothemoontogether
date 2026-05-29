/* main.h — Geiger Board */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g0xx_hal.h"

/* Pin definitions */
#define LED_STAT_Pin        GPIO_PIN_15
#define LED_STAT_GPIO_Port  GPIOA

#define CAN_STB_Pin         GPIO_PIN_5
#define CAN_STB_GPIO_Port   GPIOB

/* HV boost PWM — TIM1 CH1 PA8 */
#define HV_PWM_Pin          GPIO_PIN_8
#define HV_PWM_GPIO_Port    GPIOA

/* Geiger pulse input capture — TIM2 CH1 PA0 */
#define GEIGER_PULSE_Pin    GPIO_PIN_0
#define GEIGER_PULSE_Port   GPIOA

/* HV monitor ADC — PA1 */
#define HV_MON_Pin          GPIO_PIN_1
#define HV_MON_GPIO_Port    GPIOA

/* ADC channel */
#define HV_MON_CHANNEL      ADC_CHANNEL_1

/* HV divider scaling
 * 10MΩ top / 50kΩ bottom
 * V_adc = V_hv * 50k / (10M + 50k) = V_hv / 201
 * V_hv_mv = adc_mv * 201 */
#define HV_DIVIDER_SCALE    201

/* ADC reference */
#define ADC_VREF_MV         3300
#define ADC_RESOLUTION      4096

/* Peripheral handles */
extern FDCAN_HandleTypeDef  hfdcan1;
extern TIM_HandleTypeDef    htim1;
extern TIM_HandleTypeDef    htim2;
extern ADC_HandleTypeDef    hadc1;
extern UART_HandleTypeDef   huart1;

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
