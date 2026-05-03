#ifndef NRFBEE_H
#define NRFBEE_H

//NRFbee uses Pipe 1 for Node to Node communication and Pipe 2 for broadcast communication
//The Pipe 2 ADDR is 0xFF (the broadcast ADDR)
//Pipe 0 is reserved to send the ACK
//NRFbee uses 5bytes length Addresses
//The first four address bytes are the Network ID
//The fifth byte is the Node Address
//If the node addr is equal to 0xFF the package will be sent to the broadcast ADDR
//The Package sent in air has the following Message Structure
//u8TXAddr,u8OriginAddr,u8MsgType,u8PayloadLength, DATA0, DATA1 ... DATA27
//This means 4 bytes are used for control information and 28 bytes can be used for User defined communication

//#define USE_EEPROM_SETTINGS					//If this define is active the network and node ID will be restored from PIC EEPROM
												//else they are defined below on the DEFAULT_NETWORK_ID_x and DEFAULT_NODE_ID.

#define DEFAULT_NETWORK_ID_1	0xE7			//if USE_EEPROM_SETTINGS is commented the
#define DEFAULT_NETWORK_ID_2	0xE7			//Network ID is defined in this fields
#define DEFAULT_NETWORK_ID_3	0xE7
#define DEFAULT_NETWORK_ID_4	0xE7

#define DEFAULT_NODE_ID			1				//if USE_EEPROM_SETTINGS is commented the Node ID is defined here

#define ExitOK                  0x01
#define ExitError               0x00

#define BROADCAST_ADDR          0xFF

#define bNRFBeeIsDataReady() bNRFIsDataReady()

#define BROADCAST_PIPE          2

//This structure stores information about the Node that is transmitting, the ...
typedef struct{
	unsigned char u8TXAddr;				//Address to where we want to transmmit
	unsigned char u8OriginAddr;			//Address of node from where we are transmitting
	unsigned char u8MsgType;			//Type of message	
	unsigned char u8PayloadLength;		//Size of payload
	unsigned char u8HopCount;			//For future use???
	unsigned char * u8Payload;			//Pointer to Payload
} ST_NRFBee;	


void vNRFBeeInit(void);
unsigned char u8NRFBeeSendPayload(ST_NRFBee * stNRFbee);
void vNRFBeeGetPayload(ST_NRFBee * stNRFbee);
void vNRFBeeRXMode(void);


#endif



