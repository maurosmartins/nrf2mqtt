/*
 * ys332.c
 *
 *  Created on: Apr 6, 2026
 *      Author: mauro
 */

#include "ys332.h"

volatile int16_t PIR=0;

int16_t PIR_ReadRaw(void){
    uint32_t value = 0;
    int16_t pirRaw = 0;


    PIR_HIGH();
    PIR_OUTPUT();
    for (volatile int d = 0; d < 18; d++); //~110us


    for (uint8_t i = 0; i < 19; i++)
    {
        //Pull LOW
    	PIR_OUTPUT();
        PIR_LOW();

        delay_5us();

        //Rising edge
        PIR_HIGH();
        delay_5us();

        //wait for DOCI to switch state
		PIR_INPUT();
        delay_5us();

        //Read bit
        value <<= 1;
        if (PIR_READ() == GPIO_PIN_SET){
            value |= 1;
        }

        PIR_OUTPUT();
        PIR_LOW();

    }

    //pull low
    PIR_LOW();
    PIR_OUTPUT();


    value = (value >> 1);
    value &= 0b001111111111111111;

    pirRaw = value;

    return pirRaw;
}

void vTimingTest_5us(void){
	PIR_OUTPUT();
	while(1){
		PIR_LOW();
		delay_5us();
		PIR_HIGH();
		delay_5us();
	}
}

