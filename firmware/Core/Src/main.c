/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdbool.h>
#include <stdlib.h>
#include "nrf24l01p.h"
#include "nrfbee.h"
#include "DS18B20.h"
#include "common.h"
#include "m24cxx.h"
#include "ys332.h"
#include <string.h>
#include <stdio.h>

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM17_Init(void);
static void vHandleRemoteCommand(const char* cmd);

ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim17;
UART_HandleTypeDef huart2;

ST_NRFBee stNRFBee;							//structure used by nrfBee library
extern uint8_t u8NodeID;					//Stores the nodeID (ADDR) of the local node
extern volatile int16_t PIR;				//stores the output of the PIR sensor
uint8_t u8Payload[40];						//Stores the payload to send messages using the radio.
bool bKeepAwake = true;						//prevents the MCU to enter low power mode if = to true
uint8_t pipe;								//stores the pipe number that has received data
M24CXX_HandleTypeDef eeprom;				//stores the EEPROM handle
//volatile float temperature = 0;				//
volatile uint32_t u32pvCellVoltage, u32lipoCellVoltage;
volatile bool bLEDLow = false;
volatile bool bLEDHigh = false;
uint32_t u32EEPROM_ADDR = 0;
ADC_ChannelConfTypeDef sConfig = {0};

typedef enum {eRUN=0, eLOWBAT, ePOWERDOWN, eDURINGDAY} runState_t;
runState_t sysState = eRUN, lastSysState = ePOWERDOWN;


/**
 * @brief Redirects the standard output character to UART.
 *
 * This function is typically used to retarget the `printf` function
 * to a UART interface. It sends a single character over the UART
 * using the HAL library.
 *
 * @param ch The character to be transmitted.
 * @return The transmitted character.
 *
 * @note This implementation uses HAL_UART_Transmit with a blocking
 *       call and maximum timeout (0xFFFF).
 *
 * @warning This function is blocking and may impact real-time
 *          performance if used frequently.
 */
int __io_putchar(int ch){
 HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
 return ch;
}

/**
 * @brief Periodically checks the PV cell voltage.
 *
 * This function samples the PV cell voltage using the ADC every 100 ms.
 * It updates the global variable `u32pvCellVoltage` with the latest
 * conversion result and controls the `bLEDLow` flag based on defined
 * voltage thresholds.
 *
 * The function implements simple hysteresis:
 * - Sets `bLEDLow` to true when voltage drops below 300.
 * - Clears `bLEDLow` when voltage rises above 500.
 *
 * @note This function relies on HAL_GetTick() for timing and must be
 *       called periodically (e.g., inside a scheduler or main loop).
 *
 * @note ADC conversion is performed in blocking mode using
 *       HAL_ADC_PollForConversion().
 *
 * @warning The function is not reentrant due to the use of static
 *          variables and shared global resources.
 *
 * @warning Blocking ADC calls may affect system responsiveness if used
 *          in time-critical contexts.
 *
 * @retval None
 */
void vTask_CheckPVCell(void){
    static uint32_t last_run = 0;
    uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - last_run) >= 100){
        last_run = now;

		sConfig.Channel = PVCELLVOLTAGE;
		HAL_ADC_ConfigChannel(&hadc1, &sConfig);
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
		u32pvCellVoltage = HAL_ADC_GetValue(&hadc1);
		u32pvCellVoltage = u32pvCellVoltage * 2.1380;
    }
}

/**
 * @brief Periodically reads and updates the battery cell voltage.
 *
 * This function checks the elapsed time using HAL_GetTick() and performs
 * a battery voltage measurement every 1000 ms (1 second). It configures
 * the ADC to the battery voltage channel, starts a conversion, waits for
 * completion, and stores the result in a global variable after applying
 * a scaling factor.
 *
 * @note The function uses a static variable to keep track of the last
 * execution time and ensure periodic sampling.
 *
 * @details
 * - ADC channel is set to BATCELLVOLTAGE.
 * - Conversion is performed in blocking mode using HAL_ADC_PollForConversion().
 * - The raw ADC value is scaled by a factor of 2.1380 to obtain the
 *   actual battery voltage.
 *
 * @warning This function blocks execution during ADC conversion due to
 * HAL_MAX_DELAY timeout. Consider using interrupt or DMA mode if
 * non-blocking behavior is required.
 *
 * @global
 * - hadc1: ADC handle used for conversion.
 * - sConfig: ADC channel configuration structure.
 * - u32lipoCellVoltage: Stores the scaled battery voltage value.
 *
 * @retval None
 */
void vTask_CheckBat(void){
    static uint32_t last_run = 0;

    uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - last_run) >= 1000){
        last_run = now;

		sConfig.Channel = BATCELLVOLTAGE;
		HAL_ADC_ConfigChannel(&hadc1, &sConfig);
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
		u32lipoCellVoltage = HAL_ADC_GetValue(&hadc1);
		u32lipoCellVoltage = u32lipoCellVoltage * 2.1380;
    }
}

/**
 * @brief Periodically updates the LOW power LED state.
 *
 * This function controls the state of a GPIO pin connected to the LOW power
 * LED. Every 10 ms, it writes the value of the global
 * flag `bLEDLow` to the corresponding GPIO pin.
 *
 * @note Timing is based on HAL_GetTick() and requires the function
 *       to be called periodically (e.g., in a scheduler or main loop).
 *
 * @note The LED state directly reflects the value of `bLEDLow`,
 *       which is typically updated by another task (e.g., voltage monitoring).
 *
 * @warning This function is not reentrant due to the use of static
 *          variables and shared global resources.
 *
 * @retval None
 */
void vTask_LEDLow(void){
    static uint32_t last_run = 0;
    uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - last_run) >= 10){
        last_run = now;

        if(sysState == eRUN){
        	bLEDLow = true;
        }

        if( (sysState == eLOWBAT) || (sysState == eDURINGDAY) ){
        	bLEDLow = false;
        }

        HAL_GPIO_WritePin(LedLow_GPIO_Port, LedLow_Pin, bLEDLow);

    }//if ((uint32_t)(now - last_run) >= 10)
}//void vTask_LEDLow(void)


void vTask_info(void){
    static uint32_t last_run = 0;
    uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - last_run) >= 5000){
        last_run = now;

        printf("ID:%u,PV:%lu,BT:%lu,SS:%u\n",u8NodeID, u32pvCellVoltage, u32lipoCellVoltage, sysState);
    }
}


void vTask_PIR(void){
    static uint32_t last_run = 0;

    uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - last_run) >= 100){
        last_run = now;


		if( (sysState == eRUN) ){

			PIR = PIR_ReadRaw();
			if(PIR > 1500){

			}
		}//if( (sysState == eRUN) )


		HAL_GPIO_TogglePin(LedHigh_GPIO_Port, LedHigh_Pin);

    }//if ((uint32_t)(now - last_run) >= 100)

}//void vTask_PIR(void)



int main(void){

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config(); //-----> Sysclk = 2MHz

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_ADC1_Init();
	MX_I2C1_Init();
	MX_SPI1_Init();
	MX_USART2_UART_Init();
	MX_TIM17_Init();

	//volatile uint32_t clockfreq = 0;
	//clockfreq = HAL_RCC_GetSysClockFreq();

	m24cxx_init(&eeprom, &hi2c1, 0b1010000);

	#ifdef DEBUG_PRINT
	if(m24cxx_isconnected(&eeprom) != M24CXX_Ok){
		printf("[EEPROM Error]\n");
	}
	#endif

	m24cxx_read(&eeprom, u32EEPROM_ADDR, &u8NodeID, 1);

	if(u8NodeID == 255){
		#ifdef DEBUG_PRINT
		printf("Format Flash\n");
		#endif
		m24cxx_erase_all(&eeprom);
		#ifdef DEBUG_PRINT
		printf("Write Default Node ID\n");
		#endif
		u8NodeID  = 0x69; //105 dec
		m24cxx_write(&eeprom, u32EEPROM_ADDR, &u8NodeID, 1);
	}else{
		m24cxx_read(&eeprom, u32EEPROM_ADDR, &u8NodeID, 1);
	}

    vNRFBeeInit();
	#ifdef DEBUG_PRINT
    vNRFPrintALLRegisters();
	#endif

	stNRFBee.u8TXAddr = 0x00;
	stNRFBee.u8OriginAddr = u8NodeID;
	stNRFBee.u8MsgType = 0;
	stNRFBee.u8Payload=u8Payload;				//Sets the pointer of the Structure to the addr of local buffer

	//sprintf((char *)u8Payload, "ID%u\n",u8NodeID);
	//stNRFBee.u8PayloadLength = strlen((const char *)u8Payload);
    //u8NRFBeeSendPayload(&stNRFBee);

    for(uint32_t i=0; i<5; i++){
		HAL_GPIO_WritePin(LedHigh_GPIO_Port, LedHigh_Pin, GPIO_PIN_SET);
		HAL_Delay(200);
		HAL_GPIO_WritePin(LedHigh_GPIO_Port, LedHigh_Pin, GPIO_PIN_RESET);
		HAL_Delay(200);
    }


    while (1){

		vTask_CheckPVCell();
		vTask_CheckBat();

		//turn off if battery gets to low
		if( (u32lipoCellVoltage < 2800) && (sysState == eRUN)){
			sysState = eLOWBAT;
		}

		//turn off during the day
		if( (u32pvCellVoltage > 500) ){
			sysState = eDURINGDAY;
		}

		//turn everything on during the night
		if( (u32pvCellVoltage < 300) && (u32lipoCellVoltage > 2850) ){
			sysState = eRUN;
		}


		if(sysState != lastSysState){

			if(sysState == eRUN){
				vNRFPowerUP();
				vNRFBeeRXMode();

				#ifdef DEBUG_PRINT
					puts((char *)"[DEBUG] sysState = eRUN");
				#endif
			}

			if(sysState == eLOWBAT){
				vNRFPowerDown();
				HAL_GPIO_WritePin(LedHigh_GPIO_Port, LedHigh_Pin, GPIO_PIN_RESET);

				#ifdef DEBUG_PRINT
					puts((char *)"[DEBUG] sysState = eLOWBAT");
				#endif
			}

			if(sysState == eDURINGDAY){
				vNRFPowerDown();
				HAL_GPIO_WritePin(LedHigh_GPIO_Port, LedHigh_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(LedLow_GPIO_Port, LedLow_Pin, GPIO_PIN_RESET);

				#ifdef DEBUG_PRINT
					puts((char *)"[DEBUG] sysState = eDURINGDAY");
				#endif
			}

			lastSysState = sysState;
		}//if(sysState != lastSysState)

		vTask_LEDLow();
		vTask_info();

		if(sysState == eRUN){


			if(bNRFIsDataReady()){

				pipe = u8NRFGetPayloadPipe();

				vNRFBeeGetPayload(&stNRFBee);

				u8Payload[stNRFBee.u8PayloadLength]='\0';

				if( (pipe == BROADCAST_PIPE) ){
					#ifdef DEBUG_PRINT
					printf("[Broadcast: Ori %u Pipe %u Data-> ",stNRFBee.u8OriginAddr,pipe);
					puts((char *)u8Payload);
					#endif
				}else{
					#ifdef DEBUG_PRINT
					printf("[Direct: Ori %u Des %u Pipe %u Data-> ",stNRFBee.u8OriginAddr,stNRFBee.u8TXAddr,pipe);
					puts((char *)u8Payload);
					#endif
				}

				u8Payload[stNRFBee.u8PayloadLength]='\0';

				vHandleRemoteCommand((const char*)u8Payload);

				vNRFFlushRx();
				vNRFWriteRegister(NRFSTATUS,RX_DR);			//Cleans RX Flag writing a 1 to NRFSTATUS RX_DR bit

			}//if(bNRFBeeIsDataReady())

		}//if(sysState == eRUN)

    }//while(1)

}//main


static void vHandleRemoteCommand(const char* cmd) {

    // Copy input to a local buffer to safely use strtok
    char buffer[64];
    strncpy(buffer, cmd, strlen(cmd));


    // Split string into parts
    char* command = strtok(buffer, ":");
    char* channel_str = strtok(NULL, ":");
    char* parameter = strtok(NULL, "");

    if (!command || !channel_str || !parameter) {
        // Invalid format
        return;
    }

    int channel = atoi(channel_str);
    if (channel < 0 || channel > 99) {
        return;
    }



    if (strcmp(command, "power") == 0) {

    	GPIO_PinState PinState;

    	if (parameter[1] == 'N') { 			//ON state
			PinState = GPIO_PIN_SET;
		} else if (parameter[1] == 'F') { 	//OFF state
			PinState = GPIO_PIN_RESET;
		} else {
			return; 						// Invalid power state
		}


        switch (channel) {
            case 1:
                HAL_GPIO_WritePin(LedHigh_GPIO_Port, LedHigh_Pin, PinState);
                break;
            case 2:
            	HAL_GPIO_WritePin(LedLow_GPIO_Port, LedLow_Pin, PinState);
                break;
            // Add more cases as needed
            default:
                return;
        }


    }

    if (strcmp(command, "eeprom") == 0) {
        // Handle EEPROM write logic here
        // Example: save `value` to EEPROM at index `channel`
    	u8NodeID = (uint8_t)atoi(parameter);
    	m24cxx_write(&eeprom, u32EEPROM_ADDR, &u8NodeID, 1);
		#ifdef DEBUG_PRINT
		printf("[Saved ID: %u]\n", u8NodeID);
		#endif
		vNRFBeeRXMode();
    }

    if (strcmp(command, "pdown") == 0) {

    	vNRFPowerDown();

    } else {
        // Unknown command
        return;
    }
}




void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV8;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}


//#define DOCI_PIN 3 // Connect YS332 DOCI to digital pin 3
//
//void setup() {
//    pinMode(DOCI_PIN, INPUT_PULLDOWN); // Set DOCI as input with pull-down
//    Serial.begin(9600); // Start serial communication
//}
//
//uint16_t readYS332() {
//    uint16_t data = 0;
//
//    // Wait for DOCI to go HIGH (interrupt signal from YS332)
//    while (digitalRead(DOCI_PIN) == LOW);
//    delayMicroseconds(62); // Wait ~0.0625ms (2 system clock cycles)
//
//    // Pull DOCI LOW for at least 200ns
//    pinMode(DOCI_PIN, OUTPUT);
//    digitalWrite(DOCI_PIN, LOW);
//    delayMicroseconds(1);
//
//    for (int i = 0; i < 16; i++) {
//        // Generate rising edge and hold for ≥ 200ns
//        digitalWrite(DOCI_PIN, HIGH);
//        delayMicroseconds(1);
//
//        // Read the bit (MSB first)
//        pinMode(DOCI_PIN, INPUT);
//        data = (data << 1) | digitalRead(DOCI_PIN);
//        delayMicroseconds(1);
//
//        // Prepare for next bit
//        pinMode(DOCI_PIN, OUTPUT);
//        digitalWrite(DOCI_PIN, LOW);
//        delayMicroseconds(1);
//    }
//
//    // Force DOCI low, then release
//    digitalWrite(DOCI_PIN, LOW);
//    delayMicroseconds(1);
//    pinMode(DOCI_PIN, INPUT);
//
//    return data;
//}
//
//void loop() {
//    uint16_t pirData = readYS332();
//    Serial.print("YS332 Data: ");
//    Serial.println(pirData, HEX); // Print data in hex
//    delay(16); // Wait for next cycle (16ms)
//}




/**
  * @brief System Clock Configuration
  * @retval None
  */
/*void SystemClock_Config(void){
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  * Configure the main internal regulator output voltage

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  * Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  * Initializes the CPU, AHB and APB buses clocks

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}*/


//		if(u8NRFBeeSendPayload(&stNRFBee)==ExitOK){
//			#ifdef DEBUG_PRINT
//			printf("Ack OK\n");
//			#endif
//		} else{
//			#ifdef DEBUG_PRINT
//			printf("Ack ERROR\n");
//			#endif
//		}
//			#ifdef DEBUG_PRINT
//			printf("App Awake and in Listening Mode\n");
//			vNRFPrintALLRegisters();
//			#endif
//    while(1){
//        sConfig.Channel = ADC_CHANNEL_0;
//        sConfig.Rank = ADC_REGULAR_RANK_1;
//        sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
//
//        HAL_ADC_ConfigChannel(&hadc1, &sConfig);
//        HAL_ADC_Start(&hadc1);
//        HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
//        adc_pa0 = HAL_ADC_GetValue(&hadc1);
//
//
//        sConfig.Channel = ADC_CHANNEL_4;
//        HAL_ADC_ConfigChannel(&hadc1, &sConfig);
//		HAL_ADC_Start(&hadc1);
//		HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
//		adc_pa4 = HAL_ADC_GetValue(&hadc1);
//
//
//		printf("PV%lu BAT%lu\n", adc_pa0, adc_pa4);
//		HAL_Delay(1000);
//    }



    //vDS18B20Init();
//		#ifdef DEBUG_PRINT
//		printf("[POWER] Entering Low Power Mode\n");
//		#endif
//		for(itCounter=0; itCounter< 1; itCounter++){//4 cycles should last more or less 10  minutes
//			CLRWDT();
//			WDTCONbits.SWDTEN = 1;
//			SLEEP();
//		}
//
//		printf("[POWER] Leaving Low Power Mode\r");
//requestTemperature();
//HAL_Delay(100);
//temperature = getTempC();

//			#ifdef DEBUG_PRINT
//	    	sprintf((char *)u8Payload, "T %.2f\n", temperature);
////	    	u8Payload[0] = 'T';
////	    	u8Payload[1] = ' ';
////	    	ftoa(temperature, &u8Payload[2], 1);
//			printf("%s",u8Payload);
//			#endif

//			#define PVCELLVOLTAGE ADC_CHANNEL_0
//			#define BATCELLVOLTAGE ADC_CHANNEL_4



/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void){
  ADC_ChannelConfTypeDef sConfig = {0};

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.LowPowerAutoPowerOff = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_1CYCLE_5;
  hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_1CYCLE_5;
  hadc1.Init.OversamplingMode = DISABLE;
  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void){

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00707CBB;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void){

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM17 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM17_Init(void){

  htim17.Instance = TIM17;
  htim17.Init.Prescaler = 0;
  htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim17.Init.Period = 31;
  htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim17.Init.RepetitionCounter = 0;
  htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim17) != HAL_OK){
    Error_Handler();
  }

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_CSN_GPIO_Port, SPI1_CSN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(NRF_CE_GPIO_Port, NRF_CE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : MPU_INT_Pin */
  GPIO_InitStruct.Pin = MPU_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MPU_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = LedLow_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LedLow_GPIO_Port, &GPIO_InitStruct);

//  GPIO_InitStruct.Pin = GPIO_PIN_1;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : NRF_IRQ_Pin */
  GPIO_InitStruct.Pin = NRF_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(NRF_IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_CSN_Pin */
  GPIO_InitStruct.Pin = SPI1_CSN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_CSN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : NRF_CE_Pin */
  GPIO_InitStruct.Pin = NRF_CE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(NRF_CE_GPIO_Port, &GPIO_InitStruct);


  /*Configure GPIO pin : LedHigh_Pin */
  GPIO_InitStruct.Pin = LedHigh_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LedHigh_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_4;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);



//  /*Configure GPIO pin : OneWire_Pin */
//  GPIO_InitStruct.Pin = OneWire_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(OneWire_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
