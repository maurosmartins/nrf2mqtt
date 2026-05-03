#ifndef INC_YS332_H_
#define INC_YS332_H_

#include "stm32g0xx_hal.h"

#define PIR_PORT GPIOA
#define PIR_PIN  GPIO_PIN_12

// Input mode
#define PIR_INPUT() \
    (PIR_PORT->MODER &= ~(3U << (12 * 2)))

// Output mode
#define PIR_OUTPUT() \
    do { \
        PIR_PORT->MODER &= ~(3U << (12 * 2)); \
        PIR_PORT->MODER |=  (1U << (12 * 2)); \
    } while (0)

// Drive low
#define PIR_LOW() \
    (PIR_PORT->BSRR = (1U << (12 + 16)))

// Drive high
#define PIR_HIGH() \
    (PIR_PORT->BSRR = (1U << 12))

// Read
#define PIR_READ() \
    ((PIR_PORT->IDR >> 12) & 1)


#define delay_5us() \
		__NOP(); \
		__NOP(); \
		__NOP(); \
		__NOP(); \
		__NOP(); \
		__NOP(); \
		__NOP(); \
		__NOP(); \
		__NOP(); \
		__NOP(); \


int16_t PIR_ReadRaw(void);
void vTimingTest_5us(void);



#endif /* INC_YS332_H_ */
