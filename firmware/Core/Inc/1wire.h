#ifndef _1wire_H
#define _1wire_H

/** I N C L U D E S **********************************************************/
#include "stm32g0xx_hal.h"

//ONE WIRE PORT PIN DEFINITION

#define OW_PORT					GPIOB

#define vOW_SetPINOpenDrain() 	((OW_PORT)->OTYPER |= GPIO_OTYPER_OT3)

#define vOW_PINLow()     		((OW_PORT)->BRR = GPIO_BRR_BR3)
#define vOW_PINHi() 	    	((OW_PORT)->BSRR = GPIO_BSRR_BS3)

#define vOW_PINOutput()     	(OW_PORT)->MODER &= ~GPIO_MODER_MODE3; \
								(OW_PORT)->MODER |= (0x01 << GPIO_MODER_MODE3_Pos)

#define vOW_PINInput() 	   		(OW_PORT)->MODER &= ~GPIO_MODER_MODE3

#define OW_READ_PIN()			((OW_PORT)->IDR & GPIO_IDR_ID3_Msk) ? 1 : 0


/******* G E N E R I C   D E F I N I T I O N S ************************************************/
#define	HIGH	1
#define	LOW		0

#define	OUTPUT	0
#define	INPUT 	1

#define	SET		1
#define	CLEAR	0


/** 1 - W I R E   C O M M A N D S ******************************************************/
#define SEARCH_ROM  0xF0
#define READ_ROM    0x33
#define MATCH_ROM   0x55
#define SKIP_ROM    0xCC


/** P R O T O T Y P E S ******************************************************/
void drive_one_wire_low(void);
void drive_one_wire_high(void);
unsigned char read__one_wire(void);
void OW_write_bit(unsigned char write_data);
unsigned char OW_read_bit(void);
unsigned char OW_reset_pulse(void);
void OW_write_byte(unsigned char write_data);
unsigned char OW_read_byte(void);
void OW_Skip(void);


/*****************************************************************************
   V A R I A B L E S
******************************************************************************/

#endif
