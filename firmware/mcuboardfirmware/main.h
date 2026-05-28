/* main.h — MCU Board */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g0xx_hal.h"

/* Pin definitions */
#define LED_STAT_Pin        GPIO_PIN_15
#define LED_STAT_GPIO_Port  GPIOA

#define FLASH_CS_Pin        GPIO_PIN_4
#define FLASH_CS_GPIO_Port  GPIOA

#define CAN_STB_Pin         GPIO_PIN_5
#define CAN_STB_GPIO_Port   GPIOB

/* Peripheral handles — extern for use in other files */
extern FDCAN_HandleTypeDef  hfdcan1;
extern SPI_HandleTypeDef    hspi1;
extern UART_HandleTypeDef   huart1;
extern TIM_HandleTypeDef    htim2;

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
