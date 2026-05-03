/*
 * common.h
 *
 *  Created on: May 3, 2024
 *      Author: mauro
 */

#ifndef INC_COMMON_H_
#define INC_COMMON_H_

#include "stm32g0xx_hal.h"

#define DEBUG_PRINT
//#undef DEBUG_PRINT

//(values have been tailored to best fit the required delay sysclk & Timer clk = 32MHz)
#define DELAY480US 15360
#define DELAY410US 13144
#define DELAY70US 2263
#define DELAY64US 2077
#define DELAY60US 1922
#define DELAY55US 1760
#define DELAY15US 480
#define DELAY10US 329
#define DELAY9US 289
#define DELAY6US 186
#define DELAY1US 1

void HAL_Delay_uS(uint16_t n_uS);


#define PVCELLVOLTAGE ADC_CHANNEL_0
#define BATCELLVOLTAGE ADC_CHANNEL_4



#endif /* INC_COMMON_H_ */
