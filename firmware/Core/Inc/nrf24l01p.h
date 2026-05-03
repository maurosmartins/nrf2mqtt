#ifndef NRF24L01P_H
#define NRF24L01P_H

#include "stm32g0xx_hal.h"
#include <stdbool.h>

#define NRF_ADDR_LEN 5

#define CE_PORT GPIOB
#define CE_PIN  0

#define CSN_PORT GPIOA
#define CSN_PIN  11

#define NRFIRQ

//#define vNRFCELow() 	GPIOx->BSRR = pin
#define vNRFCELow()     ((CE_PORT)->BRR = GPIO_BRR_BR0)
#define vNRFCEHi() 	    ((CE_PORT)->BSRR = GPIO_BSRR_BS0)

#define vNRFCSNLow() 	((CSN_PORT)->BRR = GPIO_BRR_BR11)
#define vNRFCSNHi()		((CSN_PORT)->BSRR = GPIO_BSRR_BS11)


/* Memory Map */
#define CONFIG      0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define NRFSTATUS   0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0Au
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
#define FIFO_STATUS 0x17				
#define DYNPD		0x1C
#define FEATURE		0x1D

/* Bit Mnemonics */
#define MASK_RX_DR  0b01000000
#define MASK_TX_DS  0b00100000
#define MASK_MAX_RT 0b00010000
#define EN_CRC      0b00001000
#define CRCO        0b00000100
#define PWR_UP      0b00000010
#define PWR_DOWN	0b11111101
#define PRIM_RX     0b00000001
#define PRIM_TX		0b11111110
#define EN_DYN_ACK	0b00000001
#define EN_ACK_PAY  0b00000010
#define ENAA_P5     5
#define ENAA_P4     4
#define ENAA_P3     3
#define ENAA_P2     2
#define ENAA_P1     1
#define ENAA_P0     0
#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0
#define AW          0
#define ARD         4
#define ARC         0
#define PLL_LOCK    4
#define RF_DR       3
#define RF_PWR      1
#define LNA_HCURR   0        
#define RX_DR       0b01000000
#define TX_DS       5
#define MAX_RT      4
#define RX_P_NO     0b00001110
#define TX_FULL     0
#define PLOS_CNT    4
#define ARC_CNT     0
#define TX_REUSE    6
#define FIFO_FULL   5
#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0
#define EN_DPL		0b00000100
#define DPL_P0		0
#define DPL_P1		1
#define DPL_P2		2
#define DPL_P3		3
#define DPL_P4		4
#define DPL_P5		5

/* Instruction Mnemonics */
#define R_REGISTER    		0x00u
#define W_REGISTER    		0x20
#define REGISTER_MASK 		0x1F
#define R_RX_PAYLOAD  		0x61
#define W_TX_PAYLOAD  		0xA0
#define FLUSH_TX      		0xE1
#define FLUSH_RX      		0xE2
#define REUSE_TX_PL   		0xE3
#define NRFNOP           	0xFF
#define R_RX_PL_WID			0x60
#define W_TX_PAYLOAD_NO_ACK 0xB0

#define RxPipe0			0
#define RxPipe1			1
#define RxPipe2			2
#define RxPipe3			3
#define RxPipe4			4
#define RxPipe5			5


// Prototypes 
void vNRFPrintALLRegisters(void);
void vNRFPrintRXPipeAddr(uint8_t reg, uint8_t qty);
void vNRFRead(uint8_t u8RegAddr,uint8_t * u8Data,uint8_t u8Length);
uint8_t msmstatus(void);
void vNRF24l01pInit(void);
void vNRF24l01pInitV2(void);
void vNRFDefaultRX(void);
void vNRFDefaultTX(void);
void vNRFSendPayloadNoAck(uint8_t u8Payload[],uint8_t u8PayloadSize);
void vNRFSendPayload(uint8_t u8Payload[],uint8_t u8PayloadSize);
bool bNRFIsDataReady(void);
uint8_t u8NRFGetPayloadPipe(void);
uint8_t u8NRFGetPayloadLength(void);
void vNRFGetPayload(uint8_t * u8Payload,uint8_t u8PayloadSize);
bool bNRFWasAckReceived(void);
void vNRFClearDataSentFlag(void);
void vNRFClearMaxRetriesFlag(void);
void vNRFClearDataSentAndMaxRetriesFlag(void);
void vNRFEnableSendPayloadNoAckCommand(void);
uint8_t u8NRFReadRegister(uint8_t  u8RegAddr);
void vNRFWriteRegister(uint8_t u8RegADDR, uint8_t u8RegValue);
void vNRFFlushRx(void);
void vNRFFlushTx(void);
void vNRFSetTxADDR(uint8_t u8TxAddr[], uint8_t u8TxAddrLength);
void vNRFPowerUP(void);
void vNRFPowerDown(void);
void vNRFSetAsRx(void);
void vNRFSetAsTx(void);
uint8_t u8NRFGetStatusReg(void);
void vNRFSetRxADDR(uint8_t u8PipeReg,uint8_t  u8RxAddr[],uint8_t u8RxAddrLength);
void vNRFSet2to5LSBRxADD(uint8_t u8PipeReg,uint8_t u8RxLSBAddr);
void vNRFEnableRxPipeNum(uint8_t u8PipeNumber);
void vNRFDisableRxPipeNum(uint8_t u8PipeNumber);
void vNRFEnableAutoAcknowladgeOnPipe(uint8_t u8PipeNumber);
void vNRFEnableAutoAcknowladge(uint8_t u8Pipes2Enable);
void vNRFEnableDynamicPayloadFeature(uint8_t u8Pipes2Enable);
void vNRFEnableDynamicPayloadOnPipe(uint8_t u8PipeNumber);
void vNRFSpiWriteArray(uint8_t *u8Payload,uint8_t u8PayloadSize);
void vNRFSpiReadArray(uint8_t *u8Payload,uint8_t u8PayloadSize);
void vNRFSPIWriteByte(uint8_t u8Data);
uint8_t u8NRFSPIReadByte(void);
bool bNRFWasMaxRTReceived(void);

#endif


