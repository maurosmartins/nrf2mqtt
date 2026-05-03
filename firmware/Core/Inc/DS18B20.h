#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>
#include <stdbool.h>

// OneWire commands
#define STARTCONVO      0x44
#define READSCRATCH     0xBE
#define WRITESCRATCH    0x4E

// Device resolution
#define TEMP_9_BIT  0x1F //  9 bit
#define TEMP_10_BIT 0x3F // 10 bit
#define TEMP_11_BIT 0x5F // 11 bit
#define TEMP_12_BIT 0x7F // 12 bit

// Error Code
#define DEVICE_DISCONNECTED -127

void vDS18B20Init(void);
void readScratchPad(uint8_t *scratchPad, uint8_t fields);
bool isConversionComplete(void);
void requestTemperature(void);
float getTempC(void);
void setResolution(uint8_t newResolution);

#endif

//  END OF FILE
