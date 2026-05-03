#include <string.h>
#include "nrfbee.h"
#include "nrf24l01p.h"
#include "main.h"
#include "common.h"

#ifdef DEBUG_PRINT
#include <stdio.h>
#endif

unsigned char u8NetworkID[5];					//Stores the Network ID of the local node
uint8_t u8NodeID;								//Stores the ID (ADDR) of the local node

/*********************************************
 * This function Initializes the NRFBee
 * Sets the Node and Network ID
 * Puts the node on the default state (RX)
 *
 ********************************************/
void vNRFBeeInit(void){

	vNRF24l01pInit();							//Configures the SPI Port and Control Pins
	
	vNRFDefaultRX();							//Default Settings
	vNRFEnableDynamicPayloadFeature(0b00111111);
	vNRFEnableAutoAcknowladge(0b00000111);		//Activate Pipe 0, 1 and 2
	vNRFWriteRegister(SETUP_RETR,0xFF);			//Configures the TimeOut and number of retransmits

    //u8NodeID=DEFAULT_NODE_ID;

	u8NetworkID[0]=DEFAULT_NETWORK_ID_1;
	u8NetworkID[1]=DEFAULT_NETWORK_ID_2;
	u8NetworkID[2]=DEFAULT_NETWORK_ID_3;
	u8NetworkID[3]=DEFAULT_NETWORK_ID_4;
	u8NetworkID[4]=u8NodeID;
	
	vNRFEnableRxPipeNum(RxPipe2);
	//vNRFEnableRxPipeNum(RxPipe3);
	//vNRFEnableRxPipeNum(RxPipe4);
	//vNRFEnableRxPipeNum(RxPipe5);
    
    
	
	vNRFSetRxADDR(RX_ADDR_P1,u8NetworkID,5);					//
	vNRFSet2to5LSBRxADD(RX_ADDR_P2,BROADCAST_ADDR);             //The Broadcast ADDR
    u8NetworkID[4] = BROADCAST_ADDR;
    
    vNRFDisableRxPipeNum(RxPipe0);	
    vNRFDisableRxPipeNum(RxPipe3);
    vNRFDisableRxPipeNum(RxPipe4);
    vNRFDisableRxPipeNum(RxPipe5);
    
	vNRFEnableSendPayloadNoAckCommand();
	
	vNRFPowerUP();
	HAL_Delay(30);

	vNRFBeeRXMode();											//Sets the Node to the default state (RX)
	
}	

/*********************************************
 * This function sends data into a specific node
 * or to the broadcast ADDR
 *
 * If data is sent to the broadcast ADDR no ack will be generated
 *
 ********************************************/
unsigned char u8NRFBeeSendPayload(ST_NRFBee * stNRFbee){
	unsigned char u8AirPacket[32];
	bool ack;
	bool max;
	
	
	//this function should be altered and if the stNRFbee->u8TXAddr == to BroadCast Addr
	//it would automatically send in broadcast mode ignoring the ACK.
	vNRFSetAsTx();

	u8AirPacket[0]=stNRFbee->u8TXAddr;
	u8AirPacket[1]=stNRFbee->u8OriginAddr;
	u8AirPacket[2]=stNRFbee->u8MsgType;
	u8AirPacket[3]=stNRFbee->u8PayloadLength;
	
	memcpy(&u8AirPacket[4],stNRFbee->u8Payload,stNRFbee->u8PayloadLength);
	
	vNRFClearDataSentAndMaxRetriesFlag();

	if(stNRFbee->u8TXAddr!=BROADCAST_ADDR){						//The payload must be sent to a specific Node
																//An Ack will be sent by the RX Node
																//Or the maximum number of retries will be reached
																
		u8NetworkID[4]=stNRFbee->u8TXAddr;

		vNRFSetTxADDR(u8NetworkID,5);							//Set the ADDR to send data
		#ifdef DEBUG_PRINT
		printf("Tx PipeAddr: ");
		vNRFPrintRXPipeAddr( TX_ADDR, 5);						//print Tx addr despite the name
		#endif

        vNRFEnableRxPipeNum(RxPipe0);
		vNRFSetRxADDR(RX_ADDR_P0,u8NetworkID,5);				//Pipe 0 must have the same ADDR in order to receive the ack

		vNRFFlushTx();											//Flush TX buffer


		//vNRFWriteRegister(NRFSTATUS,MASK_TX_DS);					//Updates the register	
		
		vNRFClearDataSentFlag();
		vNRFClearMaxRetriesFlag();

//		#ifdef DEBUG_PRINT
//		vNRFPrintALLRegisters();
//		#endif

		vNRFSendPayload(u8AirPacket,(stNRFbee->u8PayloadLength+4));
		
		//while(!bNRFWasAckReceived()&&!bNRFWasMaxRTReceived());		//para velocidade altas do micro isto nao parece funcionar bem
		
		//maneira melhor, verificar o sinal de interrupt e so dps verificar qual � que a desencadeou
		//while(NRFIRQ==1);						//ainda nao consegui meter isto a funcionar

		HAL_Delay(1);
		
		ack=0;
		ack=bNRFWasAckReceived();
		
		max=0;
		max=bNRFWasMaxRTReceived();
		
		
		if(ack==1){
			vNRFClearDataSentFlag();
            vNRFDisableRxPipeNum(RxPipe0);
			return ExitOK;
		}

		if(max==1){
			vNRFClearMaxRetriesFlag();
            vNRFDisableRxPipeNum(RxPipe0);
			return ExitError;
		}
	
	} else{														//The payload shoud be sent to the broadcast ADDR

		//TODO: Complete this ELSE with:
		//send payload with no ack
		u8NetworkID[4]=BROADCAST_ADDR;																
		
		vNRFSetTxADDR(u8NetworkID,5);							//Set the ADDR to send data
		//vNRFSetRxADDR(RX_ADDR_P0,u8NetworkID,5);				//Pipe 0 must have the same ADDR in order to receive the ack

		vNRFFlushTx();											//Flush TX buffer
		vNRFFlushRx();											//Flush RX buffer	

		vNRFSendPayloadNoAck(u8AirPacket,(stNRFbee->u8PayloadLength+4));
		vNRFDisableRxPipeNum(RxPipe0);
		return ExitOK;
	}
	
    vNRFDisableRxPipeNum(RxPipe0);
	return ExitError;
}

/*********************************************
 * This function gets the payload and transmition
 * details if a package has been received
 *
 ********************************************/
void vNRFBeeGetPayload(ST_NRFBee * stNRFbee){
	unsigned char u8PayloadTemp[36];
	unsigned char u8PayloadSize;

	u8PayloadSize=u8NRFGetPayloadLength();
	
	//Delay10KTCYx(10);									//msm
	
	//Nop();

	vNRFGetPayload(u8PayloadTemp,u8PayloadSize);
	
	//Nop();
	
	//Delay10KTCYx(10);									//msm

	stNRFbee->u8TXAddr=u8PayloadTemp[0];
	stNRFbee->u8OriginAddr=u8PayloadTemp[1];
	stNRFbee->u8MsgType=u8PayloadTemp[2];
	stNRFbee->u8PayloadLength=u8PayloadTemp[3];

	memcpy(stNRFbee->u8Payload,&u8PayloadTemp[4],stNRFbee->u8PayloadLength);
}


/*********************************************
 * This function Puts the node on the default state (RX)
 *
 ********************************************/
void vNRFBeeRXMode(void){
	
	vNRFFlushRx();
	vNRFSetAsRx();
	
	u8NetworkID[4]=u8NodeID;
	vNRFSetRxADDR(RX_ADDR_P1,u8NetworkID,5);
	u8NetworkID[4]=DEFAULT_NETWORK_ID_4;
	vNRFSetRxADDR(RX_ADDR_P0,u8NetworkID,5);
	vNRFSet2to5LSBRxADD(RX_ADDR_P2,BROADCAST_ADDR);		//The Broadcast ADDR
	//vNRFDisableRxPipeNum(RxPipe0);
//
//
//	vNRFDefaultRX();							//Default Settings
//	vNRFEnableDynamicPayloadFeature(0b00111111);
//	vNRFEnableAutoAcknowladge(0b00000111);		//Activate Pipe 0, 1 and 2
//	vNRFWriteRegister(SETUP_RETR,0xFF);			//Configures the TimeOut and number of retransmits

	vNRFEnableDynamicPayloadFeature(0b00000111);	//Dynamic Payload on Pipe 0, 1 and 2
	vNRFEnableAutoAcknowladge(0b00000111);			//Auto Ack on Pipe 0, 1 and 2

}


