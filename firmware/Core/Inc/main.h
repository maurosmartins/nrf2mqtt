#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);


/* Private defines -----------------------------------------------------------*/
#define MPU_INT_Pin GPIO_PIN_15
#define MPU_INT_GPIO_Port GPIOC
#define NRF_IRQ_Pin GPIO_PIN_4
#define NRF_IRQ_GPIO_Port GPIOA
#define SPI1_CSN_Pin GPIO_PIN_11
#define SPI1_CSN_GPIO_Port GPIOA
#define NRF_CE_Pin GPIO_PIN_0
#define NRF_CE_GPIO_Port GPIOB
#define LedHigh_Pin GPIO_PIN_3
#define LedHigh_GPIO_Port GPIOB
#define LedLow_Pin GPIO_PIN_1
#define LedLow_GPIO_Port GPIOA


#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
