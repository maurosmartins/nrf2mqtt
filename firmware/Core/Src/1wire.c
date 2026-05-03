 /****** I N C L U D E S **********************************************************/

#include "1wire.h"
#include "nrf24l01p.h"
#include "common.h"

//****** V A R I A B L E S ********************************************************/
unsigned char macro_delay;

/**********************************************************************
* Function:        void drive_OW_low (void)
* PreCondition:    None
* Input:		   None	
* Output:		   None	
* Overview:		   Configure the OW_PIN as Output and drive the OW_PIN LOW.	
***********************************************************************/
void drive_OW_low(void){
	//vOW_PINOutput();
	vOW_PINLow();
}

//
//vOW_PINHi()
//
//

/**********************************************************************
* Function:        void drive_OW_high (void)
* PreCondition:    None
* Input:		   None	
* Output:		   None	
* Overview:		   Set the Open Drain pin High (no effect) and let the pull up drive the line HIGH.
***********************************************************************/
void drive_OW_high(void){
	//vOW_PINInput();
	vOW_PINHi();
}

/**********************************************************************
* Function:        unsigned char read_OW (void)
* PreCondition:    None
* Input:		   None	
* Output:		   Return the status of OW pin.	
* Overview:		   Configure as Input pin and Read the status of OW_PIN 	
***********************************************************************/
unsigned char read_OW(void){
	return  OW_READ_PIN();
}

/**********************************************************************
* Function:        unsigned char OW_reset_pulse(void)
* PreCondition:    None
* Input:		   None	
* Output:		   Return the Presense Pulse from the slave.	
* Overview:		   Initialization sequence start with reset pulse.
*				   This code generates reset sequence as per the protocol
***********************************************************************/
unsigned char OW_reset_pulse(void){
	unsigned char presence_detect;
	
  	drive_OW_low(); 				// Drive the bus low
 	
  	HAL_Delay_uS(DELAY480US);

 	drive_OW_high ();  				// Release the bus
	
 	HAL_Delay_uS(DELAY70US);				// delay 70 microsecond (us)
	
	//presence_detect = read_OW();	//Sample for presence pulse from slave
 	presence_detect = OW_READ_PIN();

	HAL_Delay_uS(DELAY410US);
	
	drive_OW_high ();		    	// Release the bus
	
	return presence_detect;
}	

/**********************************************************************
* Function:        void OW_write_bit (unsigned char write_data)
* PreCondition:    None
* Input:		   Write a bit to 1-wire slave device.
* Output:		   None
* Overview:		   This function used to transmit a single bit to slave device.
*				   
***********************************************************************/

void OW_write_bit(unsigned char write_bit){
	
	if (write_bit){
		//writing a bit '1'
		drive_OW_low(); 				// Drive the bus low
		HAL_Delay_uS(DELAY6US);				// delay 6 microsecond (us)
		drive_OW_high ();  				// Release the bus
		HAL_Delay_uS(DELAY64US);				// delay 64 microsecond (us)
	} else{
		//writing a bit '0'
		drive_OW_low(); 				// Drive the bus low
		HAL_Delay_uS(DELAY60US);				// delay 60 microsecond (us)
		drive_OW_high ();  				// Release the bus
		HAL_Delay_uS(DELAY10US);				// delay 10 microsecond for recovery (us)
	}
}	


/**********************************************************************
* Function:        unsigned char OW_read_bit (void)
* PreCondition:    None
* Input:		   None
* Output:		   Return the status of the OW PIN
* Overview:		   This function used to read a single bit from the slave device.
*				   
***********************************************************************/

unsigned char OW_read_bit(void){
	
	unsigned char read_data; 
	//reading a bit 
	drive_OW_low(); 						// Drive the bus low
	HAL_Delay_uS(DELAY6US);						// delay 6 microsecond (us)
	drive_OW_high ();  						// Release the bus
	HAL_Delay_uS(DELAY6US);						// delay 9 microsecond (us)

	//read_data = read_OW();					//Read the status of OW_PIN
	read_data = OW_READ_PIN();

	HAL_Delay_uS(DELAY55US);						// delay 55 microsecond (us)
	return read_data;
}

/**********************************************************************
* Function:        void OW_write_byte (unsigned char write_data)
* PreCondition:    None
* Input:		   Send byte to 1-wire slave device
* Output:		   None
* Overview:		   This function used to transmit a complete byte to slave device.
*				   
***********************************************************************/
void OW_write_byte(unsigned char write_data){
	unsigned char loop;
	
	for (loop = 0; loop < 8; loop++){
		OW_write_bit(write_data & 0x01); 	//Sending LS-bit first
		write_data >>= 1;					// shift the data byte for the next bit to send
	}	
}	

/**********************************************************************
* Function:        unsigned char OW_read_byte (void)
* PreCondition:    None
* Input:		   None
* Output:		   Return the read byte from slave device
* Overview:		   This function used to read a complete byte from the slave device.
*				   
***********************************************************************/

unsigned char OW_read_byte(void){
	unsigned char loop, result=0;
	
	for (loop = 0; loop < 8; loop++){
		
		result >>= 1; 				// shift the result to get it ready for the next bit to receive
		if (OW_read_bit())
		result |= 0x80;				// if result is one, then set MS-bit
	}
	return result;					
}

/**********************************************************************
* Function:        void OW_Skip (void)
* PreCondition:    Only valid if only one 1wire device exists on the bus
* Input:		   None
* Output:		   None
* Overview:		   This function used to select a 1wire device without having to set an address.
*				   
***********************************************************************/
void OW_Skip(void){
    OW_write_byte(SKIP_ROM);
}
/********************************************************************************************
                  E N D     O F     1 W I R E . C  
*********************************************************************************************/
