#include <stdbool.h>
#include "stm32g0xx_hal.h"
#include "nrf24l01p.h"
#include "common.h"
#include "main.h"

#ifdef DEBUG_PRINT
#include <stdio.h>
#endif

/* Channel & Payload */ 
uint8_t NRFchannel;
uint8_t PayloadSize;

extern SPI_HandleTypeDef hspi1;


#ifdef DEBUG_PRINT
void vNRFPrintALLRegisters(void){
    printf("\r\r\n");
    printf("STATUS    :0x%.2X\n", u8NRFReadRegister(NRFSTATUS) );
    printf("CONFIG    :0x%.2X\n", u8NRFReadRegister(CONFIG)    );
    printf("EN_AA     :0x%.2X\n", u8NRFReadRegister(EN_AA)     );
    printf("EN_RXADDR :0x%.2X\n", u8NRFReadRegister(EN_RXADDR) );
    printf("SETUP_AW  :0x%.2X\n", u8NRFReadRegister(SETUP_AW)  );
    printf("SETUP_RETR:0x%.2X\n", u8NRFReadRegister(SETUP_RETR));
    printf("RF_CH     :0x%.2X\n", u8NRFReadRegister(RF_CH)     );
    printf("RF_SETUP  :0x%.2X\n", u8NRFReadRegister(RF_SETUP)  );
    printf("DYNPD     :0x%.2X\n", u8NRFReadRegister(DYNPD)     );
    printf("FEATURE   :0x%.2X\n", u8NRFReadRegister(FEATURE)   );
    printf("RxPipeAddr0: "); vNRFPrintRXPipeAddr( RX_ADDR_P0, 5);
    printf("RxPipeAddr1: "); vNRFPrintRXPipeAddr( RX_ADDR_P1, 5);
    printf("RxPipeAddr2: "); vNRFPrintRXPipeAddr( RX_ADDR_P2, 1);
    printf("RxPipeAddr3: "); vNRFPrintRXPipeAddr( RX_ADDR_P3, 1);
    printf("RxPipeAddr4: "); vNRFPrintRXPipeAddr( RX_ADDR_P4, 1);
    printf("RxPipeAddr5: "); vNRFPrintRXPipeAddr( RX_ADDR_P5, 1);

    printf("\nTxPipeAddr: "); vNRFPrintRXPipeAddr( TX_ADDR, 5);
}

void vNRFPrintRXPipeAddr(uint8_t reg, uint8_t qty){
	uint8_t buffer[5];
	uint8_t i;

  	vNRFCSNLow();                               // Pull down chip select
    HAL_Delay_uS(DELAY1US);
	
	//vNRFSPIWriteByte((uint8_t)(R_REGISTER|RX_ADDR_P0));            	// Send cmd to read rx payload
    vNRFSPIWriteByte((uint8_t)(R_REGISTER|reg));
	vNRFSpiReadArray(buffer, qty);	// Read payload
    
	HAL_Delay_uS(DELAY1US);
	vNRFCSNHi();
    for(i=0;i<qty; i++){
        printf("0x%.2X ", buffer[i]);
    }
    printf("\n");
}
#endif


//Add the following line to the nrf24l01p.c file
//This Function Enables the W_TX_PAYLOAD_NOACK command
//In other words this function enables the
//the functionallity that allows data to be sent without ACK 
void vNRFEnableSendPayloadNoAckCommand(void){
	uint8_t u8Feature;
	u8Feature=u8NRFReadRegister(FEATURE);
	vNRFWriteRegister(FEATURE,(u8Feature|EN_DYN_ACK)); 
}



//This Function Enables the RX pipe with u8PipeNumber
void vNRFEnableRxPipeNum(uint8_t u8PipeNumber){
	uint8_t u8EN_RXADDR;
	u8EN_RXADDR=u8NRFReadRegister(EN_RXADDR);
	u8EN_RXADDR=u8EN_RXADDR|(0b00000001<<u8PipeNumber);
	vNRFWriteRegister(EN_RXADDR,u8EN_RXADDR);
}	

//This Function Disables the RX pipe with u8PipeNumber
void vNRFDisableRxPipeNum(uint8_t u8PipeNumber){
	uint8_t u8EN_RXADDR;
	u8EN_RXADDR=u8NRFReadRegister(EN_RXADDR);
	u8EN_RXADDR=u8EN_RXADDR & (~(0b00000001<<u8PipeNumber));
	vNRFWriteRegister(EN_RXADDR,u8EN_RXADDR);
}

//This Function Enables the Dynamic Payload Feature on argument Pipes. If no pipe is to be enabled call function with 0x00 on the argument
//In order to enable dynamic payload on individual pipes use NRFEnableDynamicPayloadOnPipe
void vNRFEnableDynamicPayloadFeature(uint8_t u8Pipes2Enable){
	uint8_t u8Feature;
	u8Feature=u8NRFReadRegister(FEATURE);
	vNRFWriteRegister(FEATURE,(u8Feature|EN_DPL));			//enables Dynamic Payload Feature
	vNRFWriteRegister(DYNPD,(u8Pipes2Enable&0b00111111));	//Enables Dynamic payload on specified pipes	
}

//This Function Enables the dynamic payload on a specific Pipe
//For this function to work we need to call NRFEnableDynamicPayloadLength first
void vNRFEnableDynamicPayloadOnPipe(uint8_t u8PipeNumber){
	uint8_t u8DYNPD;
	u8DYNPD=u8NRFReadRegister(DYNPD);
	vNRFWriteRegister(DYNPD,(u8DYNPD|(0b00000001<<u8PipeNumber)));
}

//Get the Length of the received Payload
uint8_t u8NRFGetPayloadLength(void){
	uint8_t u8PayloadLength;
	
	vNRFCSNLow();
	
    vNRFSPIWriteByte(R_RX_PL_WID);				//Read Payload Length Command
    
    u8PayloadLength=u8NRFSPIReadByte();
    
    vNRFCSNHi();
    
    
    
	return u8PayloadLength;
}

//Get Pipe number that has received data (0 to 5)
uint8_t u8NRFGetPayloadPipe(void){
	//The pipe number is in the 3:1 bits of Status Register
	return ((u8NRFGetStatusReg()&RX_P_NO)>>1);
}

//This functgion enables the AutoAck feature and can enable this option on argument pipes.
//If no pipe is to be enabled, call function with 0x00 in the argument
//for setting AutoAck on individual pipes use vNRFEnableAutoAcknowladgeOnPipe
void vNRFEnableAutoAcknowladge(uint8_t u8Pipes2Enable){
	//u8Feature=u8NRFReadRegister(FEATURE);
	//vNRFWriteRegister(FEATURE,(u8Feature|0b00000010));
//	uint8_t u8Feature;
//	u8Feature=u8NRFReadRegister(FEATURE);
//	vNRFWriteRegister(FEATURE,(u8Feature|EN_ACK_PAY));			//enables Payload With Ack

	vNRFWriteRegister(EN_AA,(u8Pipes2Enable&0b00111111));
}

//This function enables Auto Acknowladge on individual pipe specified by PipeNumber (0 to 5)
void vNRFEnableAutoAcknowladgeOnPipe(uint8_t u8PipeNumber){
	uint8_t u8EN_AA;
	u8EN_AA=u8NRFReadRegister(EN_AA);
	vNRFWriteRegister(EN_AA,(u8EN_AA|(0b00000001<<u8PipeNumber)));
}


//This Function Checks if the package was successfully sent and an Ack received
bool bNRFWasAckReceived(void){
	uint8_t u8Status=0;
	
	u8Status=u8NRFGetStatusReg();
	
	if((u8Status&MASK_TX_DS)==MASK_TX_DS){
		return true;
	}else{
		return false;
	}	
}


//This Function Checks if the maximum number of transmit retries wasn't reached
bool bNRFWasMaxRTReceived(void){
	uint8_t u8Status;
	u8Status=u8NRFGetStatusReg();
	if((u8Status&MASK_MAX_RT)!=0){
		return true;
	}else{
		return false;
	}	
}

//This Function cleans the flag TX_DS (indicates that the package was sent and an Ack was received)
void vNRFClearDataSentFlag(void){
	//uint8_t u8Status;
	//u8Status=u8NRFGetStatusReg();
	//u8Status=(u8Status|MASK_TX_DS);
	//vNRFWriteRegister(NRFSTATUS,u8Status);
	vNRFWriteRegister(NRFSTATUS,MASK_TX_DS);
	
}

//This Function cleans the flag MAX_RT (indicates that the maximum attempts to send
//the package was reached without an Ack being received)
void vNRFClearMaxRetriesFlag(void){
	uint8_t u8Status;
	u8Status=u8NRFGetStatusReg();
	u8Status=(u8Status|MASK_MAX_RT);
	vNRFWriteRegister(NRFSTATUS,u8Status);
}


//this function performs the operation from vNRFClearDataSentFlag and vNRFClearMaxRetriesFlag
void vNRFClearDataSentAndMaxRetriesFlag(void){
	uint8_t u8Status;
	u8Status=u8NRFGetStatusReg();
	u8Status=(u8Status|MASK_TX_DS|MASK_MAX_RT);
	vNRFWriteRegister(NRFSTATUS,u8Status);	
}	
	



void vNRFSPIWriteByte(uint8_t u8Data){
	HAL_SPI_Transmit(&hspi1, &u8Data, 1, HAL_MAX_DELAY);
}

uint8_t u8NRFSPIReadByte(void){
	uint8_t u8Data;
	HAL_SPI_Receive(&hspi1, &u8Data, 1, HAL_MAX_DELAY);
	return u8Data;
}
	
	 
// Initializes pins to communicate with the NRF module
// Should be called in the early initializing phase at startup.
void vNRF24l01pInit(void){
	
	//initialize control pins default state
	vNRFCELow();
	vNRFCSNHi();
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCWH p�gina 53
    
}


void vNRF24l01pInitV2(void){

   	//initialize control pins direction as output 
//	TrisCSN=0;
//	TrisCE=0;
	
	//initialize Interrupt pins as input
//	TrisIRQ=1;
	
	//initialize control pins default state
	vNRFCELow();
    vNRFCSNHi();
   HAL_Delay_uS(DELAY1US);                              //datasheet  50ns parameter TCWH page 53
   

   HAL_Delay_uS(DELAY10US);                              //datasheet  50ns parameter TCWH page 53
   vNRFWriteRegister(CONFIG,    0b01111001);   //MASK_RX_DR MASK_TX_DS MASK_MAX_RT EN_CRC CRCO PWR_UP PRIM_RX (0=TX, 1=RX)
   vNRFWriteRegister(EN_AA,     0b00000111);   //Reserved ENAA_P5 ENAA_P4 ENAA_P3 ENAA_P2 ENAA_P1 ENAA_P0  
   vNRFWriteRegister(EN_RXADDR, 0b00000111);   //Reserved ERX_P5 ERX_P4 ERX_P3 ERX_P2 ERX_P1 ERX_P0
   vNRFWriteRegister(SETUP_AW,  0b00000011);	//Reserved AW -> 5bytes (addr size)
   vNRFWriteRegister(SETUP_RETR,0b00101111);   //ARD ARC -> 750us delay between retries, 15 retries
   vNRFWriteRegister(RF_CH,     0b00000001);   //RF_CH -> 0x01
   vNRFWriteRegister(RF_SETUP,  0b00100110);   //CONT_WAVE Reserved RF_DR_LOW PLL_LOCK RF_DR_HIGH RF_PWR Obsolete -> 250kbps, 0dbm
   vNRFWriteRegister(DYNPD,     0b00000111);   //Reserved DPL_P5 DPL_P4 DPL_P3 DPL_P2 DPL_P1 DPL_P0
   vNRFWriteRegister(FEATURE,   0b00000111);   //Reserved EN_DPL EN_ACK_PAYd EN_DYN_ACK  
   vNRFFlushRx();
   vNRFFlushTx();
}



//Sets the important registers in the NRF module and powers the module in receiving mode.
void vNRFDefaultRX(void){
	uint8_t u8RxAddr[5]={0xE7,0xE7,0xE7,0xE7,0xE7};
	
	vNRFWriteRegister(CONFIG,    0b01111011);   //MASK_RX_DR MASK_TX_DS MASK_MAX_RT EN_CRC CRCO PWR_UP PRIM_RX (0=TX, 1=RX)
    vNRFWriteRegister(EN_AA,     0b00000111);   //Reserved ENAA_P5 ENAA_P4 ENAA_P3 ENAA_P2 ENAA_P1 ENAA_P0  
    vNRFWriteRegister(EN_RXADDR, 0b00000111);   //Reserved ERX_P5 ERX_P4 ERX_P3 ERX_P2 ERX_P1 ERX_P0
    vNRFWriteRegister(SETUP_AW,  0b00000011);	//Reserved AW -> 5bytes
    vNRFWriteRegister(SETUP_RETR,0b00101111);   //ARD ARC -> 750us delay between retries, 15 retries
    vNRFWriteRegister(RF_CH,     0b00000001);   //RF_CH -> 0x01
    vNRFWriteRegister(RF_SETUP,  0b00100110);   //CONT_WAVE Reserved RF_DR_LOW PLL_LOCK RF_DR_HIGH RF_PWR Obsolete -> 250kbps, 0dbm
    vNRFWriteRegister(DYNPD,     0b00000111);   //Reserved DPL_P5 DPL_P4 DPL_P3 DPL_P2 DPL_P1 DPL_P0
    vNRFWriteRegister(FEATURE,   0b00000111);   //Reserved EN_DPL EN_ACK_PAYd EN_DYN_ACK
    	
    //vNRFSetRxADDR(0, u8RxAddr, 5);
    vNRFSetRxADDR(RX_ADDR_P0, u8RxAddr, 5);
	
	vNRFFlushRx();
	vNRFSetAsRx();
}

//Sets the important registers in the NRF module and powers the module in Transmitting mode.
void vNRFDefaultTX(void){
	uint8_t u8TxAddr[5]={0xE7,0xE7,0xE7,0xE7,0xE7};
    //uint8_t u8TxAddr[5]={0x78, 0x78, 0x78, 0x78, 0x78};
    
    
	vNRFWriteRegister(CONFIG,    0b01111010);//0x7A   //MASK_RX_DR MASK_TX_DS MASK_MAX_RT EN_CRC CRCO PWR_UP PRIM_RX (0=TX, 1=RX)
    vNRFWriteRegister(EN_AA,     0b00000111);//0x07   //Reserved ENAA_P5 ENAA_P4 ENAA_P3 ENAA_P2 ENAA_P1 ENAA_P0  
    vNRFWriteRegister(EN_RXADDR, 0b00000111);//0x07   //Reserved ERX_P5 ERX_P4 ERX_P3 ERX_P2 ERX_P1 ERX_P0
    vNRFWriteRegister(SETUP_AW,  0b00000011);//0x03	  //Reserved AW -> 5bytes
    vNRFWriteRegister(SETUP_RETR,0b00101111);//0x2F   //ARD ARC -> 750us delay between retries, 15 retries
    vNRFWriteRegister(RF_CH,     0b00000001);//0x01   //RF_CH -> 0x01
    vNRFWriteRegister(RF_SETUP,  0b00100110);//0x26   //CONT_WAVE Reserved RF_DR_LOW PLL_LOCK RF_DR_HIGH RF_PWR Obsolete -> 250kbps, 0dbm
    vNRFWriteRegister(DYNPD,     0b00000111);//0x07   //Reserved DPL_P5 DPL_P4 DPL_P3 DPL_P2 DPL_P1 DPL_P0
    vNRFWriteRegister(FEATURE,   0b00000111);//0x07   //Reserved EN_DPL EN_ACK_PAYd EN_DYN_ACK
    	
	//vNRFSetTxADDR(u8TxAddr,5);	
    //vNRFSetRxADDR(0, u8TxAddr, 5);
    
    
    vNRFSetTxADDR(u8TxAddr,5);	
    vNRFSetRxADDR(RX_ADDR_P0, u8TxAddr, 5);
    
	
	vNRFFlushTx();
	vNRFSetAsTx();
}	

//Sets the receiving address Pipe 0 or 1
//For setting pipe 2 to 5 use vNRFSet2to5LSBRxADD())
//one of :
//#define RX_ADDR_P0  0x0A
//#define RX_ADDR_P1  0x0B
//#define RX_ADDR_P2  0x0C
//#define RX_ADDR_P3  0x0D
//#define RX_ADDR_P4  0x0E
//#define RX_ADDR_P5  0x0F
void vNRFSetRxADDR(uint8_t u8PipeReg,uint8_t  u8RxAddr[],uint8_t u8RxAddrLength){
	uint8_t u8RxAddrInv[6];
	uint8_t i,j;

	j=0;
	for(i=u8RxAddrLength;i>0;i--){
		u8RxAddrInv[j++] = u8RxAddr[i-1];
	}

	vNRFCSNLow();
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 2ns parametro TCC p�gina 53
    
	vNRFSPIWriteByte(W_REGISTER|u8PipeReg);
	HAL_SPI_Transmit(&hspi1, u8RxAddrInv, u8RxAddrLength, HAL_MAX_DELAY);
		
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCCh p�gina 53
    vNRFCSNHi();
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCWH p�gina 53
}

//Sets the Least significant byte of pipe 2 to 5
//the remaining 4 (MSB) are shared with pipe 1
void vNRFSet2to5LSBRxADD(uint8_t u8PipeReg,uint8_t u8RxLSBAddr){
	vNRFWriteRegister(u8PipeReg,u8RxLSBAddr);
}	

// Sets the transmitting address, u8TxAddrLength
void vNRFSetTxADDR(uint8_t u8TxAddr[], uint8_t u8TxAddrLength){
	uint8_t u8TxAddrInv[6];
	uint8_t i,j;

	j=0;
	for(i=u8TxAddrLength;i>0;i--){
		u8TxAddrInv[j++] = u8TxAddr[i-1];
	}

	vNRFCSNLow();
    HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 2ns parametro TCC p�gina 53
	
	vNRFSPIWriteByte(W_REGISTER|TX_ADDR);
	HAL_SPI_Transmit(&hspi1, u8TxAddrInv, u8TxAddrLength, HAL_MAX_DELAY);
	
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCCh p�gina 53
    vNRFCSNHi();
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCWH p�gina 53
	
	
}

// Checks if data is available for reading
bool bNRFIsDataReady(void){
	uint8_t u8NRFSTATUS;
    
	//u8NRFSTATUS=u8NRFReadRegister(NRFSTATUS);

	u8NRFSTATUS =  u8NRFGetStatusReg();
	
	if((u8NRFSTATUS&RX_DR)!=0){
		vNRFWriteRegister(NRFSTATUS, NRFSTATUS | RX_DR);
		return true;
	} else{
		return false;
	}
}

bool bRxFifoEmpty(void){
	//uint8_t  fifoStatus;
	
	//u8NRFReadRegister(FIFO_STATUS,&fifoStatus,sizeof(fifoStatus));
	//return (fifoStatus & 0b00000001);
    return false;
}


//Reads an array of data 
void vNRFSpiReadArray(uint8_t *u8Payload,uint8_t u8PayloadSize){
	HAL_SPI_Receive(&hspi1, u8Payload, u8PayloadSize, HAL_MAX_DELAY);
}

//Writes an array of data 
void vNRFSpiWriteArray(uint8_t *u8Payload,uint8_t u8PayloadSize){
	HAL_SPI_Transmit(&hspi1, u8Payload, u8PayloadSize, HAL_MAX_DELAY);
}
	

// Reads payload bytes into data array
void vNRFGetPayload(uint8_t * u8Payload,uint8_t u8PayloadSize){
	
	vNRFCSNLow();                               // Pull down chip select
    HAL_Delay_uS(DELAY1US);
	
	vNRFSPIWriteByte(R_RX_PAYLOAD);            	// Send cmd to read rx payload
	//Delay10KTCYx(1);
	vNRFSpiReadArray(u8Payload,u8PayloadSize);	// Read payload
		HAL_Delay_uS(DELAY1US);
	vNRFCSNHi();
	
	
	
	//Delay10KTCYx(1);
		
	vNRFCSNLow();
	//Delay10KTCYx(1);
	
	vNRFSPIWriteByte(FLUSH_RX);					//Clean Rx Buffer
	//Delay10KTCYx(1);
	
	vNRFCSNHi();
	
	
	//Delay10KTCYx(1);
	
	vNRFWriteRegister(NRFSTATUS,RX_DR);			//Cleans RX Flag writing a 1 to NRFSTATUS RX_DR bit	
}

//Clocks only one byte into the given NRF register
void vNRFWriteRegister(uint8_t u8RegADDR, uint8_t u8RegValue){
	uint8_t data[2];
    vNRFCSNLow();
    HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 2ns parametro TCC p�gina 53

	data[0] = (W_REGISTER | u8RegADDR);
	data[1] = u8RegValue;
    HAL_SPI_Transmit(&hspi1, data, 2, HAL_MAX_DELAY); //Sends

	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCCh p�gina 53
    vNRFCSNHi();
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCWH p�gina 53
}

//Reads a Given Register
uint8_t u8NRFReadRegister(uint8_t  u8RegAddr){
	uint8_t u8RegisterValue;

    vNRFCSNLow();
	HAL_Delay_uS(DELAY1US);                          //no datasheet fala em 2ns parametro TCC p�gina 53

	u8RegisterValue = R_REGISTER|u8RegAddr;


	HAL_SPI_Transmit(&hspi1, &u8RegisterValue, 1, HAL_MAX_DELAY); //Sends Read Command and Register Address
	HAL_SPI_Receive(&hspi1, &u8RegisterValue, 1, HAL_MAX_DELAY);  //Dummy Byte is sent in order to clock out the register value

    HAL_Delay_uS(DELAY1US);                          //no datasheet fala em 50ns parametro TCCh p�gina 53
    vNRFCSNHi();
    HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCWH p�gina 53

    return u8RegisterValue;
}


/**
 * isSending.
 *
 * Test if chip is still sending.
 * When sending has finished return chip to listening.
 *
 */

bool bIsSending(void){
//	uint8_t  NRFstatus;
//	uint8_t PTX=1;
//	if(PTX){
//		NRFstatus = getStatus();
//	    	
//		/*
//		 *  if sending successful (TX_DS) or max retries exceded (MAX_RT).
//		 */
//
//		if((NRFstatus & ((1 << TX_DS)  | (1 << MAX_RT)))){
//			vPowerUpRx();
//			return false; 
//		}
//
//		return true;
//	}
    return false;
}


////Reads the Status register
//uint8_t u8NRFGetStatusReg(void){
//	return u8NRFReadRegister(NRFSTATUS);			//Sends the Nop Operation to Clock out the Status Register
//}


//Reads the Status register
uint8_t u8NRFGetStatusReg(void){
	uint8_t u8StatusReg;
//	return u8NRFReadRegister(NRFSTATUS);			//Sends the Nop Operation to Clock out the Status Register

	vNRFCSNLow();
	HAL_Delay_uS(DELAY1US);                          //no datasheet fala em 2ns parametro TCC p�gina 53


	HAL_SPI_Receive(&hspi1, &u8StatusReg, 1, HAL_MAX_DELAY);  //Dummy Byte is sent in order to clock out the register value

	HAL_Delay_uS(DELAY1US);                          //no datasheet fala em 50ns parametro TCCh p�gina 53
	vNRFCSNHi();
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCWH p�gina 53

	return u8StatusReg;
}








//Sets the NRF as RX
void vNRFSetAsRx(void){
	uint8_t u8NRFConfig;
	
	u8NRFConfig=u8NRFReadRegister(CONFIG);			//Reads Current Contents of Config Register	
	u8NRFConfig=(u8NRFConfig|PRIM_RX);				//Ores it with Primary RX 
	vNRFWriteRegister(CONFIG,u8NRFConfig);			//Updates the register
    
    vNRFCEHi();                                     //tem de estar a 1 para estar no modo RX
    
}

//Sets the NRF as TX
void vNRFSetAsTx(void){
	uint8_t u8NRFConfig;
    
    vNRFCELow();                                     //tem de estar a 0 para estar no modo TX
	
	u8NRFConfig=u8NRFReadRegister(CONFIG);			//Reads Current Contents of Config Register	
	u8NRFConfig=(u8NRFConfig&PRIM_TX);				//ANDs it with Primary TX 
	vNRFWriteRegister(CONFIG,u8NRFConfig);			//Updates the register
}

//Clears the Rx Buffer
void vNRFFlushRx(void){
    vNRFCSNLow();
    HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 2ns parametro TCC p�gina 53
    
    vNRFSPIWriteByte(FLUSH_RX);
    
    HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCCh p�gina 53
    vNRFCSNHi();
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCWH p�gina 53
}

//Clears the Tx Buffer
void vNRFFlushTx(void){
    vNRFCSNLow();
    HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 2ns parametro TCC p�gina 53
    
    vNRFSPIWriteByte(FLUSH_TX);

    HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCCh p�gina 53
    vNRFCSNHi();
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCWH p�gina 53
}


void vNRFPowerDown(void){
	uint8_t u8NRFConfig;
	
	u8NRFConfig=u8NRFReadRegister(CONFIG);		//Reads Current Contents of Config Register	
	u8NRFConfig=(u8NRFConfig&PWR_DOWN);			//Ands it with Power DOWN
	vNRFWriteRegister(CONFIG, u8NRFConfig);		//Updates the register
}

void vNRFPowerUP(void){
	uint8_t u8NRFConfig;
	
	u8NRFConfig=u8NRFReadRegister(CONFIG);		//Reads Current Contents of Config Register	
	u8NRFConfig=(u8NRFConfig|PWR_UP);			//Ores it with PowerUP
	vNRFWriteRegister(CONFIG,u8NRFConfig);		//Updates the register
	HAL_Delay(30);                              //parametro TPD2STBY p�gina 24
}

void vNRFSendPayload(uint8_t u8Payload[],uint8_t u8PayloadSize){

	//Todo: Ver se nao devia ser Clean TX Ints
	vNRFWriteRegister(NRFSTATUS,0b01111110);    //Clean Int. Flags 
      
    //Pressup�e que ja esta ligado antes, estou a fazer isso no default tx e rx
    //vNRFWriteRegister(CONFIG,0x3A);             //PWR_UP = 1
	vNRFFlushTx();
    
    vNRFCSNLow();
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 2ns parametro TCC p�gina 53
	
    vNRFSPIWriteByte(W_TX_PAYLOAD);
    vNRFSpiWriteArray(u8Payload,u8PayloadSize);
    
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCCh p�gina 53
    vNRFCSNHi();
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCWH p�gina 53
    
    
	    
    //Pulse CE to start transmission
	vNRFCEHi();
	HAL_Delay_uS(DELAY1US);
	HAL_Delay_uS(12);
	HAL_Delay_uS(12);
	HAL_Delay_uS(12);

	vNRFCELow();
}

void vNRFSendPayloadNoAck(uint8_t u8Payload[],uint8_t u8PayloadSize){

	//Todo: Ver se nao devia ser Clean TX Ints
	vNRFWriteRegister(NRFSTATUS,0b01111110);    //Clean Int. Flags 

	vNRFFlushTx();
										
    
	vNRFCSNLow();
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 2ns parametro TCC p�gina 53
    
    vNRFSPIWriteByte(W_TX_PAYLOAD_NO_ACK);
    vNRFSpiWriteArray(u8Payload,u8PayloadSize);	
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCCh p�gina 53
    vNRFCSNHi();
	HAL_Delay_uS(DELAY1US);                              //no datasheet fala em 50ns parametro TCWH p�gina 53
    
    	    
    //Pulse CE to start transmission
	vNRFCEHi();
    HAL_Delay_uS(DELAY15US);                             //no datasheet fala em pelo menos 10us
	vNRFCELow();

	HAL_Delay_uS(DELAY10US);
}

void vNRFRead(uint8_t u8RegAddr,uint8_t * u8Data,uint8_t u8Length){
	uint8_t u8i;
	
	vNRFCSNLow();
	
	vNRFSPIWriteByte(R_REGISTER|u8RegAddr);	//Sends Read Command and Register Address
	 
	for(u8i=0;u8i<u8Length;u8i++){
		u8Data[u8i]=u8NRFSPIReadByte();
	}
		
  vNRFCSNHi();
	
}

