#include "common.h"


void HAL_Delay_uS(uint16_t n_uS){
	TIM17->CNT = 0;						//cnt register = 0
	TIM17->ARR = n_uS;
	TIM17->SR &= ~TIM_FLAG_UPDATE;		//clear update flag
	TIM17->CR1 |= TIM_CR1_CEN;			//turn on timer
	while((TIM17->SR & TIM_SR_UIF) == 0);
	TIM17->CR1 &= ~TIM_CR1_CEN;			//turn oFF timer
}
