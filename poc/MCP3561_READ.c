#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_def.h"
#include "main.h"
#include "gpio.h"
#include "mcp3564.h"
#include "mcp3564_conf.h"
#include "spi.h"
#include <stdbool.h>
// #include "swv_debug.h"
#include "fdcan.h"
#include "stm32g4xx_hal_spi.h"
#include "stm32g4xx_hal_tim.h"
#include "tim.h"


void _MCP3561_write(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t size);
uint8_t _MCP3561_sread(SPI_HandleTypeDef *hspi, uint8_t *cmd);
void MCP3561_Channels(SPI_HandleTypeDef *hspi, uint8_t ch_p, uint8_t ch_n);
void MCP3561_Init(SPI_HandleTypeDef *hspi);
void MCP3561_PrintRegisters(SPI_HandleTypeDef *hspi);
void MCP3561_Reset(SPI_HandleTypeDef *hspi);
void MCP3561_ADC_Start_Restart(SPI_HandleTypeDef *hspi);
void MCP3561_ADC_Standby(SPI_HandleTypeDef *hspi);
void MCP3561_ADC_Shutdown(SPI_HandleTypeDef *hspi);
void MCP3561_ADC_Full_Shutdown(SPI_HandleTypeDef *hspi);
uint32_t MCP3561_ReadADCData(SPI_HandleTypeDef *hspi);
int32_t MCP3561_ReadADCData_24Bit(SPI_HandleTypeDef *hspi);
int32_t MCP3561_ReadADCData_32Bit(SPI_HandleTypeDef *hspi);
int32_t * MCP3561_ReadADCData_32Bit_Scan(SPI_HandleTypeDef *hspi);
uint32_t MCP3561_ReadADCData_IT(SPI_HandleTypeDef *hspi);




// Variables externas
extern SPI_HandleTypeDef hspi1; 
extern TIM_HandleTypeDef htim5;

volatile uint32_t adc_val;
volatile bool setup_done = false;


int main(void){
	// 
	setup_done = false;

  
	HAL_Init();
	MX_GPIO_Init();
	MX_SPI1_Init();
	MX_SPI3_Init();
	

	// MCP1 channel setup
	HAL_TIM_Base_Start(&htim5);
	HAL_TIM_OC_Start(&htim5, TIM_CHANNEL_1);
	HAL_Delay(10);
	MCP3561_Reset(&hspi1);
	HAL_Delay(10);
	MCP3561_PrintRegisters(&hspi1);
	MCP3561_Init(&hspi1);
	MCP3561_PrintRegisters(&hspi1);

	//MCP2 channel setup
	HAL_TIM_Base_Start(&htim3);
	HAL_TIM_OC_Start(&htim3, TIM_CHANNEL_2);
	HAL_Delay(10);
	MCP3561_Reset(&hspi1);
	HAL_Delay(10);
	MCP3561_PrintRegisters(&hspi1);
	MCP3561_Init(&hspi1);
	MCP3561_PrintRegisters(&hspi1);
	

	//MCP3 channel setup
	HAL_TIM_Base_Start(&htim1);
	HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_1);
	HAL_Delay(10);
	MCP3561_Reset(&hspi3);
	HAL_Delay(10);
	MCP3561_PrintRegisters(&hspi3);


	//MCP4 channel setup
	HAL_TIM_Base_Start(&htim4);
	HAL_TIM_OC_Start(&htim4, TIM_CHANNEL_2);
	HAL_Delay(10);
	MCP3561_Reset(&hspi3);
	HAL_Delay(10);
	MCP3561_PrintRegisters(&hspi3);



	
	setup_done = true;

	
	// master clock setup for this adc
	while(1){
		HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
		HAL_Delay(1000);
	}



	

    return 0;
}

// MCP3561 low level functions


void _MCP3561_write(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t size){
	// manually operate the !CS signal, because the STM32 hardware NSS signal is (sadly) useless 
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(hspi, pData, size, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, GPIO_PIN_SET);
    
    if (HAL_SPI_Transmit(hspi, pData, size, MCP3561_HAL_TIMEOUT) != HAL_OK) {
        // Handle error
        Error_Handler();
    }
    
}

uint8_t _MCP3561_sread(SPI_HandleTypeDef *hspi, uint8_t *cmd){
	uint8_t reg8[2];
	// manually operate the !CS signal, because the STM32 hardware NSS signal is (sadly) useless
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, reg8, 2, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, GPIO_PIN_SET);
	return reg8[1];
}

void MCP3561_Channels(SPI_HandleTypeDef *hspi, uint8_t ch_p, uint8_t ch_n){
	uint8_t cmd[4] = {0,0,0,0};
	cmd[0]  = MCP3561_MUX_WRITE;
	cmd[1]  = (ch_p << 4) | ch_n;   // [7..4] VIN+ / [3..0] VIN-
	//cmd[1]  = (MCP3561_MUX_CH_IntTemp_P << 4) | MCP3561_MUX_CH_IntTemp_M;   // [7..4] VIN+ / [3..0] VIN-
	_MCP3561_write(hspi, cmd, 2);
}

void MCP3561_Init(SPI_HandleTypeDef *hspi){
	uint8_t cmd[4] = {0,0,0,0};
	
	// Following the MCP3561 Errata Sheet recommendation (section 3) when internal oscillator is selected
	uint32_t reg_val = 0x900F00;
	cmd[0]  = MCP3561_RSVD_WRITE;
	cmd[1] = (uint8_t)((reg_val >> 16) & 0xff);
	cmd[2] = (uint8_t)((reg_val >>  8) & 0xff);
	cmd[3] = (uint8_t)((reg_val)       & 0xff);

	// 8-bit CONFIG registers
	cmd[0]  = MCP3561_CONFIG0_WRITE;
	cmd[1]  = MCP3561_USERCONF_REG0;
	_MCP3561_write(hspi, cmd, 2);

	cmd[0]  = MCP3561_CONFIG1_WRITE;
	cmd[1]  = MCP3561_USERCONF_REG1;
	_MCP3561_write(hspi, cmd, 2);

	cmd[0]  = MCP3561_CONFIG2_WRITE;
	cmd[1]  = MCP3561_USERCONF_REG2;
	cmd[1] += 3; // last two bits must always be '11'
	_MCP3561_write(hspi, cmd, 2);

	cmd[0]  = MCP3561_CONFIG3_WRITE;
	cmd[1]  = MCP3561_USERCONF_REG3;
	_MCP3561_write(hspi, cmd, 2);

	cmd[0]  = MCP3561_IRQ_WRITE;
	cmd[1]  = MCP3561_USERCONF_IRQ_REG;
	_MCP3561_write(hspi, cmd, 2);

	// 24-bit CONFIG registers

	// configure SCAN mode to automatically cycle through channels
	// only available for MCP3562 and MCP3564, and only for certain input combinations
	// @see Datasheet Table 5-14 on p. 54
	#ifdef MCP3561_USERCONF_SCAN_ENABLE
		reg_val = MCP3561_USERCONF_SCAN_REG;
		cmd[0] = MCP3561_SCAN_WRITE;
		cmd[1] = (uint8_t)((reg_val >> 16) & 0xff);
		cmd[2] = (uint8_t)((reg_val >>  8) & 0xff);
		cmd[3] = (uint8_t)((reg_val)       & 0xff);
		_MCP3561_write(hspi, cmd, 4);

		reg_val = MCP3561_USERCONF_TIMER_VAL;
		cmd[0] = MCP3561_TIMER_WRITE;
		cmd[1] = (uint8_t)((reg_val >> 16) & 0xff);
		cmd[2] = (uint8_t)((reg_val >>  8) & 0xff);
		cmd[3] = (uint8_t)((reg_val)       & 0xff);
		_MCP3561_write(hspi, cmd, 4);
	#endif

}

void MCP3561_PrintRegisters(SPI_HandleTypeDef *hspi){
	uint8_t reg8 = 0;
	uint8_t cmd [5] = {0,0,0,0,0};
    // no existe PRINTF en este entorno, así que los comentarios de printf son solo indicativos

	cmd[0] = MCP3561_CONFIG0_SREAD;
	reg8 = _MCP3561_sread(hspi, cmd);
	// printf("CONF0: %02x\n", reg8);

	cmd[0] = MCP3561_CONFIG1_SREAD;
	reg8 = _MCP3561_sread(hspi, cmd);
	// printf("CONF1: %02x\n", reg8);

	cmd[0] = MCP3561_CONFIG2_SREAD;
	reg8 = _MCP3561_sread(hspi, cmd);
	// printf("CONF2: %02x\n", reg8);

	cmd[0] = MCP3561_CONFIG3_SREAD;
	reg8 = _MCP3561_sread(hspi, cmd);
	// printf("CONF3: %02x\n", reg8);

	cmd[0] = MCP3561_IRQ_SREAD;
	reg8 = _MCP3561_sread(hspi, cmd);
	// printf("IRQ  : %02x\n", reg8);

	cmd[0] = MCP3561_MUX_SREAD;
	reg8 = _MCP3561_sread(hspi, cmd);
	// printf("MUX  : %02x\n", reg8);

	cmd[0] = MCP3561_SCAN_SREAD;
	uint8_t resp [5] = {0,0,0,0,0};

	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, resp, 4, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, GPIO_PIN_SET);

	// printf("SCAN : %02x %02x %02x\n", resp[1], resp[2], resp[3]);

	cmd[0] = MCP3561_TIMER_SREAD;

	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, resp, 4, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, GPIO_PIN_SET);

	// printf("TIMER: %02x %02x %02x\n", resp[1], resp[2], resp[3]);

	/* @todo all the remaining registers, registros externos tocará..*/ 
}

void MCP3561_Reset(SPI_HandleTypeDef *hspi){
	uint8_t cmd;
	cmd = DEVICE_RESET_COMMAND;
	_MCP3561_write(hspi, &cmd, 1);
}

void MCP3561_ADC_Start_Restart(SPI_HandleTypeDef *hspi)
{
	uint8_t cmd;
	cmd = DEVICE_ADC_START_RESTART_COMMAND;
	_MCP3561_write(hspi, &cmd, 1);
}

void MCP3561_ADC_Standby(SPI_HandleTypeDef *hspi)
{
	uint8_t cmd;
	cmd = DEVICE_ADC_STANDBY_COMMAND;
	_MCP3561_write(hspi, &cmd, 1);
}

void MCP3561_ADC_Shutdown(SPI_HandleTypeDef *hspi)
{
	uint8_t cmd;
	cmd = DEVICE_ADC_SHUTDOWN_COMMAND;
	_MCP3561_write(hspi, &cmd, 1);
}

void MCP3561_ADC_Full_Shutdown(SPI_HandleTypeDef *hspi)
{
	uint8_t cmd;
	cmd = DEVICE_ADC_FULL_SHUTDOWN_COMMAND;
	_MCP3561_write(hspi, &cmd, 1);
}

uint32_t MCP3561_ReadADCData(SPI_HandleTypeDef *hspi){
	uint8_t val[5] = {0,0,0,0,0};
	uint8_t cmd[5] = {0,0,0,0,0};
	cmd[0] = MCP3561_SREAD_DATA_COMMAND;
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, 0);
	HAL_SPI_TransmitReceive(hspi, cmd, val, 5, 10);
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, 1);
	uint32_t value = (val[1] << 16) | (val[2] << 8) | val[3];
	return value;
}

int32_t MCP3561_ReadADCData_24Bit(SPI_HandleTypeDef *hspi){
	uint8_t val[5] = {0,0,0,0,0};
	uint8_t cmd[5] = {0,0,0,0,0};
	cmd[0] = MCP3561_SREAD_DATA_COMMAND;
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, 0);
	HAL_SPI_TransmitReceive(hspi, cmd, val, 5, 10);
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, 1);
	int32_t value = (((val[1] & 0x80) >> 7 ) << 31) | ( (val[1] & 0x7F) << 16) | (val[2] << 8) | val[3];
	return value;
}

int32_t MCP3561_ReadADCData_32Bit(SPI_HandleTypeDef *hspi){
	uint8_t val[5] = {0,0,0,0,0};
	uint8_t cmd[5] = {0,0,0,0,0};
	cmd[0] = MCP3561_SREAD_DATA_COMMAND;
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, 0);
	HAL_SPI_TransmitReceive(hspi, cmd, val, 5, 10);
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, 1);
	int32_t value = ((val[1] & 0x01) << 31) | (val[2] << 16) | (val[3] << 8) | val[4];
	return value;
}

int32_t * MCP3561_ReadADCData_32Bit_Scan(SPI_HandleTypeDef *hspi){
	uint8_t val[5] = {0,0,0,0,0};
	uint8_t cmd[5] = {0,0,0,0,0};
	cmd[0] = MCP3561_SREAD_DATA_COMMAND;
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, 0);
	HAL_SPI_TransmitReceive(hspi, cmd, val, 5, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, 1);
	int32_t value[3] = { val[0] ,
			( (val[1] & 0xF0) >> 4 ) ,
			( (val[1] & 0x01) << 31) | (val[2] << 16) | (val[3] << 8) | val[4]};
	int32_t * ptr = &value[0];
	return ptr;
}

uint32_t MCP3561_ReadADCData_IT(SPI_HandleTypeDef *hspi){
	uint8_t val[5] = {0,0,0,0,0};
	uint8_t cmd[5] = {0,0,0,0,0};
	cmd[0] = MCP3561_SREAD_DATA_COMMAND;

	// manually operate the !CS signal, because the STM32 hardware NSS signal is (sadly) useless
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, val, 5, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(MCP3561_CHIP_SELECT_GPIO_Port, MCP3561_CHIP_SELECT_GPIO_Pin, GPIO_PIN_SET);

	uint32_t value = (val[1] << 16) | (val[2] << 8) | val[3];
	return value;
}