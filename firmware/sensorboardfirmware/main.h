/* main.h — Sensor Board */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g0xx_hal.h"

/* Pin definitions */
#define LED_STAT_Pin         GPIO_PIN_15
#define LED_STAT_GPIO_Port   GPIOA

#define CAN_STB_Pin          GPIO_PIN_5
#define CAN_STB_GPIO_Port    GPIOB

#define LSM_INT1_Pin         GPIO_PIN_4
#define LSM_INT1_GPIO_Port   GPIOA

/* ADC channel pins */
#define CHARGE_HI_PIN        GPIO_PIN_0   /* PA0 — ADC_CHANNEL_0 */
#define CHARGE_LO_PIN        GPIO_PIN_1   /* PA1 — ADC_CHANNEL_1 */
#define THERM_PIN            GPIO_PIN_2   /* PA2 — ADC_CHANNEL_2 */

/* I2C sensor addresses */
#define MS5607_ADDR          (0x76 << 1)  /* CSB = V3V3 */
#define BMP390_ADDR          (0x77 << 1)  /* SDO = V3V3 */
#define SHT40_ADDR           (0x44 << 1)  /* Fixed */
#define LSM6DSO32_ADDR       (0x6A << 1)  /* SA0 = GND */

/* Peripheral handles */
extern FDCAN_HandleTypeDef  hfdcan1;
extern I2C_HandleTypeDef    hi2c1;
extern ADC_HandleTypeDef    hadc1;
extern TIM_HandleTypeDef    htim1;
extern UART_HandleTypeDef   huart1;

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
