#include "stm32g0xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "DS18B20.h"
#include "1wire.h"
#include "nrf24l01p.h"

void vDS18B20Init(void){
	vOW_PINOutput();
	vOW_SetPINOpenDrain();
	vOW_PINHi();
    OW_reset_pulse();

}

void readScratchPad(uint8_t *scratchPad, uint8_t fields){
  OW_reset_pulse();
  //_wire->select(deviceAddress);
  OW_Skip(); //only valid when only one 1Wire device exist on BUS
  OW_write_byte(READSCRATCH);

  for(uint8_t i=0; i < fields; i++){
    scratchPad[i] = OW_read_byte();
  }
  OW_reset_pulse();
}

bool isConversionComplete(void){
  return (OW_read_bit() == 1);
}	

void requestTemperature(void){
    OW_reset_pulse();
    OW_Skip();                    //only valid when only one 1Wire device exist on BUS
    OW_write_byte(STARTCONVO);
}

float getTempC(void){
    //ScratchPad scratchPad;
    uint8_t scratchPad[8];
    int16_t rawTemperature;
    float temp = 0;
    
    readScratchPad(scratchPad, 2);

    rawTemperature = (((int16_t)scratchPad[1]) << 8) | scratchPad[0];

    temp = 0.0625 * rawTemperature;
    
    if(temp < -55){
        return DEVICE_DISCONNECTED;
    }
    return temp;
}

void setResolution(uint8_t newResolution){
  OW_reset_pulse();
  //_wire->select(deviceAddress);
  OW_Skip();                    //only valid when only one 1Wire device exist on BUS
  OW_write_byte(WRITESCRATCH);
  // two dummy values for LOW & HIGH ALARM
  OW_write_byte(0);
  OW_write_byte(100);
  switch (newResolution)
  {
  case 12:
    OW_write_byte(TEMP_12_BIT);
    break;
  case 11:
    OW_write_byte(TEMP_11_BIT);
    break;
  case 10:
    OW_write_byte(TEMP_10_BIT);
    break;
  case 9:
  default:
    OW_write_byte(TEMP_9_BIT);
    break;
  }
  OW_reset_pulse();
}

//  END OF FILE
