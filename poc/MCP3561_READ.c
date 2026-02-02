#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_def.h"
#include "main.h"
#include "gpio.h"
#include "mcp3564.h"
#include "mcp3564_conf.h"
#include "spi.h"
#include <stdbool.h>
#include <stdint.h>
// #include "swv_debug.h"
#include "fdcan.h"
#include "stm32g4xx_hal_fdcan.h"
#include "stm32g4xx_hal_spi.h"
#include "stm32g4xx_hal_tim.h"
#include "tim.h"


void _MCP3561_write(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
                      uint8_t *pData, uint16_t size);
uint8_t _MCP3561_sread(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
                       uint8_t *cmd);

/* Public API matching Core/Inc/mcp3564.h */
void MCP3561_Channels(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
                      uint8_t ch_p, uint8_t ch_n);
void MCP3561_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
void MCP3561_PrintRegisters(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
void MCP3561_Reset(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
void MCP3561_ADC_Start_Restart(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
void MCP3561_ADC_Standby(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
void MCP3561_ADC_Shutdown(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
void MCP3561_ADC_Full_Shutdown(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
uint32_t MCP3561_ReadADCData(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
int32_t  MCP3561_ReadADCData_24Bit(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
int32_t  MCP3561_ReadADCData_32Bit(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
int32_t  MCP3561_ReadADCData_32Bit_Scan(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port,
                                        uint16_t cs_pin, uint8_t *ch_id, uint8_t *status);
uint32_t MCP3561_ReadADCData_IT(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
void send_sensors_over_can(uint16_t s1, uint16_t s2, uint16_t s3, uint16_t s4){
	FDCAN_TxHeaderTypeDef txh;
	uint8_t data[8];

	txh.Identifier = 0x123; // standard can ID
	txh.IdType = FDCAN_STANDARD_ID;
	txh.TxFrameType = FDCAN_DATA_FRAME;

	txh.DataLength = FDCAN_DLC_BYTES_8;
	txh.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	txh.BitRateSwitch = FDCAN_BRS_OFF;
	txh.FDFormat = FDCAN_CLASSIC_CAN;
	txh.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	txh.MessageMarker = 0;


	// pack big endian byte 
	data[0] = (uint8_t)(s1 >> 8);
    data[1] = (uint8_t)(s1 & 0xFF);

    data[2] = (uint8_t)(s2 >> 8);
    data[3] = (uint8_t)(s2 & 0xFF);

    data[4] = (uint8_t)(s3 >> 8);
    data[5] = (uint8_t)(s3 & 0xFF);

    data[6] = (uint8_t)(s4 >> 8);
    data[7] = (uint8_t)(s4 & 0xFF);
	if(HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txh, data) != HAL_OK){
		Error_Handler();
	}

}

// Variables externas
extern SPI_HandleTypeDef hspi1; 
extern TIM_HandleTypeDef htim5;

volatile uint32_t adc_val;
volatile bool setup_done = false;

uint32_t t0;

int main(void){
	// 
	setup_done = false;
	t0 = HAL_GetTick();

	
	HAL_Init();
	//GPIO system 
	MX_GPIO_Init();
	//SPI
	MX_SPI1_Init();
	MX_SPI3_Init();
	// Master Clocks
	MX_TIM1_Init();
	MX_TIM3_Init();
	MX_TIM4_Init();
	MX_TIM5_Init();
	// Protocolo FDCAN2

	MX_FDCAN2_Init();
	// configurar un filtro para FDCAN2 (aceptar todos los mensajes)

	

	FDCAN_FilterTypeDef filter_config2;
	filter_config2.IdType = FDCAN_STANDARD_ID;
	filter_config2.FilterIndex = 0;
	filter_config2.FilterType = FDCAN_FILTER_RANGE;
	filter_config2.FilterConfig = FDCAN_FILTER_RANGE;
	filter_config2.FilterID1 = 0x000; // desde 0x000 a 
	filter_config2.FilterID2 = 0x7FF; // 0x7FF (todos los IDs estandar)

	if(HAL_FDCAN_ConfigFilter(&hfdcan2, &filter_config2) != HAL_OK){
		// Error en la configuracion del filtro
		Error_Handler();
	}

	// Iniciar FDCAN2 
	if(HAL_FDCAN_Start(&hfdcan2) != HAL_OK){
		// Error en el inicio de FDCAN2
		Error_Handler();
	}

	

	// MCP1 channel setup (SPI1, CS_ADC_1)
	HAL_TIM_Base_Start(&htim5);
	HAL_TIM_OC_Start(&htim5, TIM_CHANNEL_1);
	HAL_Delay(10);
	MCP3561_Reset(&hspi1, CS_ADC_1_GPIO_Port, CS_ADC_1_Pin);
	HAL_Delay(10);
	MCP3561_PrintRegisters(&hspi1, CS_ADC_1_GPIO_Port, CS_ADC_1_Pin);
	MCP3561_Init(&hspi1, CS_ADC_1_GPIO_Port, CS_ADC_1_Pin);
	MCP3561_PrintRegisters(&hspi1, CS_ADC_1_GPIO_Port, CS_ADC_1_Pin);

	// MCP2 channel setup (SPI1, CS_ADC_2)
	HAL_TIM_Base_Start(&htim3);
	HAL_TIM_OC_Start(&htim3, TIM_CHANNEL_2);
	HAL_Delay(10);
	MCP3561_Reset(&hspi1, CS_ADC_2_GPIO_Port, CS_ADC_2_Pin);
	HAL_Delay(10);
	MCP3561_PrintRegisters(&hspi1, CS_ADC_2_GPIO_Port, CS_ADC_2_Pin);
	MCP3561_Init(&hspi1, CS_ADC_2_GPIO_Port, CS_ADC_2_Pin);
	MCP3561_PrintRegisters(&hspi1, CS_ADC_2_GPIO_Port, CS_ADC_2_Pin);
	
	// MCP3 channel setup (SPI3, CS_ADC_3)
	HAL_TIM_Base_Start(&htim1);
	HAL_TIM_OC_Start(&htim1, TIM_CHANNEL_1);
	HAL_Delay(10);
	MCP3561_Reset(&hspi3, CS_ADC_3_GPIO_Port, CS_ADC_3_Pin);
	HAL_Delay(10);
	MCP3561_PrintRegisters(&hspi3, CS_ADC_3_GPIO_Port, CS_ADC_3_Pin);
	MCP3561_Init(&hspi3, CS_ADC_3_GPIO_Port, CS_ADC_3_Pin);
	MCP3561_PrintRegisters(&hspi3, CS_ADC_3_GPIO_Port, CS_ADC_3_Pin);

	// MCP4 channel setup (SPI3, CS_ADC_4)
	HAL_TIM_Base_Start(&htim4);
	HAL_TIM_OC_Start(&htim4, TIM_CHANNEL_2);
	HAL_Delay(10);
	MCP3561_Reset(&hspi3, CS_ADC_4_GPIO_Port, CS_ADC_4_Pin);
	HAL_Delay(10);
	MCP3561_PrintRegisters(&hspi3, CS_ADC_4_GPIO_Port, CS_ADC_4_Pin);
	MCP3561_Init(&hspi3, CS_ADC_4_GPIO_Port, CS_ADC_4_Pin);
	MCP3561_PrintRegisters(&hspi3, CS_ADC_4_GPIO_Port, CS_ADC_4_Pin);

	setup_done = true;
	
	// Four differential thermocouple readings per ADC:
	//   idx 0 -> DIFF_A (CH0-CH1)
	//   idx 1 -> DIFF_B (CH2-CH3)
	//   idx 2 -> DIFF_C (CH4-CH5)
	//   idx 3 -> DIFF_D (CH6-CH7)
	int32_t adc1_tc[8] = {0,0,0,0,0,0,0,0};
	int32_t adc2_tc[8] = {0,0,0,0,0,0,0,0};
	int32_t adc3_tc[8] = {0,0,0,0,0,0,0,0};
	int32_t adc4_tc[8] = {0,0,0,0,0,0,0,0};

	// bitmask of which channels have been updated since last full scan
	uint8_t adc1_seen = 0;
	uint8_t adc2_seen = 0;
	uint8_t adc3_seen = 0;
	uint8_t adc4_seen = 0;

	while(1){
		// Iniciar comunicacion FDCAN para enviar los datos a la tarjeta RF
		// each MCP3561 module has an internal mux to select the input channels
		// in SCAN mode, channels are selected automatically according to SCAN config
		if (setup_done){
			uint8_t ch_id;
			uint8_t status;
			int32_t code;

			// --- ADC 1 (SPI1, CS_ADC_1) ---
			code = MCP3561_ReadADCData_32Bit_Scan(&hspi1, CS_ADC_1_GPIO_Port, CS_ADC_1_Pin,
			                                     &ch_id, &status);

			if (ch_id < 8) { 
				adc1_tc[ch_id] = code;
				adc1_seen |= (1u << ch_id);
			}

			// --- ADC 2 (SPI1, CS_ADC_2) ---
			code = MCP3561_ReadADCData_32Bit_Scan(&hspi1, CS_ADC_2_GPIO_Port, CS_ADC_2_Pin,
			                                     &ch_id, &status);
			if (ch_id < 8) {
				adc2_tc[ch_id] = code;
				adc2_seen |= (1u << ch_id);
			}

			// --- ADC 3 (SPI3, CS_ADC_3) ---
			code = MCP3561_ReadADCData_32Bit_Scan(&hspi3, CS_ADC_3_GPIO_Port, CS_ADC_3_Pin,
			                                     &ch_id, &status);
			if (ch_id < 8) {
				adc3_tc[ch_id] = code;
				adc3_seen |= (1u << ch_id);
			}

			// --- ADC 4 (SPI3, CS_ADC_4) ---
			code = MCP3561_ReadADCData_32Bit_Scan(&hspi3, CS_ADC_4_GPIO_Port, CS_ADC_4_Pin,
			                                     &ch_id, &status);
			if (ch_id < 8) {
				adc4_tc[ch_id] = code;
				adc4_seen |= (1u << ch_id);
			}

			

			// At this point, adcX_channels[0..7] hold the latest value per channel.
			// When the seen reaches 0xFF, you have a "full" snapshot for that ADC.
			if (adc1_seen == 0xFF) {
				
				uint16_t tp7 = (uint16_t)adc1_tc[0] - (uint16_t)adc1_tc[1];
				uint16_t tp3 = (uint16_t)adc1_tc[2] - (uint16_t)adc1_tc[3];
				uint16_t tp1 = (uint16_t)adc1_tc[4] - (uint16_t)adc1_tc[5];
				uint16_t rt1 = (uint16_t)adc1_tc[6] - (uint16_t)adc1_tc[7];
				adc1_seen = 0; // reset for next full 
				
			}

			if (adc2_seen == 0xFF) {
				// TODO: process full snapshot for ADC2 here
				uint16_t tp5 = (uint16_t)adc2_tc[0] - (uint16_t)adc2_tc[1];
				uint16_t tp4 = (uint16_t)adc2_tc[2] - (uint16_t)adc2_tc[3];
				uint16_t tp8 = (uint16_t)adc2_tc[4] - (uint16_t)adc2_tc[5];
				uint16_t tp9 = (uint16_t)adc2_tc[6] - (uint16_t)adc2_tc[7];
				adc2_seen = 0;
			}

			if (adc3_seen == 0xFF) {
				uint16_t tp6 = (uint16_t)adc3_tc[0] - (uint16_t)adc3_tc[1];
				uint16_t tp10 = (uint16_t)adc3_tc[2] - (uint16_t)adc3_tc[3];
				uint16_t rt2 = (uint16_t)adc3_tc[4] - (uint16_t)adc3_tc[5];
				uint16_t tp2 = (uint16_t)adc3_tc[6] - (uint16_t)adc3_tc[7];

				
				// TODO: process full snapshot for ADC3 here
				adc3_seen = 0;
			}

			if (adc4_seen == 0xFF) {
				// TODO: process full snapshot for ADC4 here
				uint16_t g2 = (uint16_t)adc4_tc[0] - (uint16_t)adc4_tc[1];
				uint16_t g1 = (uint16_t)adc4_tc[2] - (uint16_t)adc4_tc[3];
				uint16_t c = (uint16_t)adc4_tc[4] - (uint16_t)adc4_tc[5];
				uint16_t g3 = (uint16_t)adc4_tc[6] - (uint16_t)adc4_tc[7];

				adc4_seen = 0;
			}

			
			
			

			
		}
	}


	

    return 0;
}



// MCP3561 low level functions

void _MCP3561_write(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
                    uint8_t *pData, uint16_t size)
{
	// manually operate the !CS signal, because the STM32 hardware NSS signal is (sadly) useless 
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
	if (HAL_SPI_Transmit(hspi, pData, size, MCP3561_HAL_TIMEOUT) != HAL_OK) {
		HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
		Error_Handler();
	}
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
}

uint8_t _MCP3561_sread(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
                       uint8_t *cmd)
{
	uint8_t reg8[2];
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, reg8, 2, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
	return reg8[1];
}

void MCP3561_Channels(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
                      uint8_t ch_p, uint8_t ch_n)
{
	uint8_t cmd[4] = {0,0,0,0};
	cmd[0]  = MCP3561_MUX_WRITE;
	cmd[1]  = (ch_p << 4) | ch_n;   // [7..4] VIN+ / [3..0] VIN-
	_MCP3561_write(hspi, cs_port, cs_pin, cmd, 2);
}

void MCP3561_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin){
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
	_MCP3561_write(hspi, cs_port, cs_pin, cmd, 2);

	cmd[0]  = MCP3561_CONFIG1_WRITE;
	cmd[1]  = MCP3561_USERCONF_REG1;
	_MCP3561_write(hspi, cs_port, cs_pin, cmd, 2);

	cmd[0]  = MCP3561_CONFIG2_WRITE;
	cmd[1]  = MCP3561_USERCONF_REG2;
	cmd[1] += 3; // last two bits must always be '11'
	_MCP3561_write(hspi, cs_port, cs_pin, cmd, 2);

	cmd[0]  = MCP3561_CONFIG3_WRITE;
	cmd[1]  = MCP3561_USERCONF_REG3;
	_MCP3561_write(hspi, cs_port, cs_pin, cmd, 2);

	cmd[0]  = MCP3561_IRQ_WRITE;
	cmd[1]  = MCP3561_USERCONF_IRQ_REG;
	_MCP3561_write(hspi, cs_port, cs_pin, cmd, 2);

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
		_MCP3561_write(hspi, cs_port, cs_pin, cmd, 4);

		reg_val = MCP3561_USERCONF_TIMER_VAL;
		cmd[0] = MCP3561_TIMER_WRITE;
		cmd[1] = (uint8_t)((reg_val >> 16) & 0xff);
		cmd[2] = (uint8_t)((reg_val >>  8) & 0xff);
		cmd[3] = (uint8_t)((reg_val)       & 0xff);
		_MCP3561_write(hspi, cs_port, cs_pin, cmd, 4);
	#endif

}

void MCP3561_PrintRegisters(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin){
	uint8_t reg8 = 0;
	uint8_t cmd [5] = {0,0,0,0,0};
    // no existe PRINTF en este entorno, así que los comentarios de printf son solo indicativos

	cmd[0] = MCP3561_CONFIG0_SREAD;
	reg8 = _MCP3561_sread(hspi, cs_port, cs_pin, cmd);
	// printf("CONF0: %02x\n", reg8);

	cmd[0] = MCP3561_CONFIG1_SREAD;
	reg8 = _MCP3561_sread(hspi, cs_port, cs_pin, cmd);
	// printf("CONF1: %02x\n", reg8);

	cmd[0] = MCP3561_CONFIG2_SREAD;
	reg8 = _MCP3561_sread(hspi, cs_port, cs_pin, cmd);
	// printf("CONF2: %02x\n", reg8);

	cmd[0] = MCP3561_CONFIG3_SREAD;
	reg8 = _MCP3561_sread(hspi, cs_port, cs_pin, cmd);
	// printf("CONF3: %02x\n", reg8);

	cmd[0] = MCP3561_IRQ_SREAD;
	reg8 = _MCP3561_sread(hspi, cs_port, cs_pin, cmd);
	// printf("IRQ  : %02x\n", reg8);

	cmd[0] = MCP3561_MUX_SREAD;
	reg8 = _MCP3561_sread(hspi, cs_port, cs_pin, cmd);
	// printf("MUX  : %02x\\n", reg8);

	cmd[0] = MCP3561_SCAN_SREAD;
	uint8_t resp [5] = {0,0,0,0,0};

	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, resp, 4, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);

	// printf("SCAN : %02x %02x %02x\n", resp[1], resp[2], resp[3]);

	cmd[0] = MCP3561_TIMER_SREAD;

	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, resp, 4, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);

	// printf("TIMER: %02x %02x %02x\n", resp[1], resp[2], resp[3]);

	/* @todo all the remaining registers, registros externos tocará..*/ 
}

void MCP3561_Reset(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin){
	uint8_t cmd;
	cmd = DEVICE_RESET_COMMAND;
	_MCP3561_write(hspi, cs_port, cs_pin, &cmd, 1);
}

void MCP3561_ADC_Start_Restart(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
	uint8_t cmd;
	cmd = DEVICE_ADC_START_RESTART_COMMAND;
	_MCP3561_write(hspi, cs_port, cs_pin, &cmd, 1);
}

void MCP3561_ADC_Standby(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
	uint8_t cmd;
	cmd = DEVICE_ADC_STANDBY_COMMAND;
	_MCP3561_write(hspi, cs_port, cs_pin, &cmd, 1);
}

void MCP3561_ADC_Shutdown(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
	uint8_t cmd;
	cmd = DEVICE_ADC_SHUTDOWN_COMMAND;
	_MCP3561_write(hspi, cs_port, cs_pin, &cmd, 1);
}

void MCP3561_ADC_Full_Shutdown(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
	uint8_t cmd;
	cmd = DEVICE_ADC_FULL_SHUTDOWN_COMMAND;
	_MCP3561_write(hspi, cs_port, cs_pin, &cmd, 1);
}

uint32_t MCP3561_ReadADCData(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin){
	uint8_t val[5] = {0,0,0,0,0};
	uint8_t cmd[5] = {0,0,0,0,0};
	cmd[0] = MCP3561_SREAD_DATA_COMMAND;
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, val, 5, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
	uint32_t value = (val[1] << 16) | (val[2] << 8) | val[3];
	return value;
}

int32_t MCP3561_ReadADCData_24Bit(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin){
	uint8_t val[5] = {0,0,0,0,0};
	uint8_t cmd[5] = {0,0,0,0,0};
	cmd[0] = MCP3561_SREAD_DATA_COMMAND;
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, val, 5, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
	int32_t value = (((val[1] & 0x80) >> 7 ) << 31) | ( (val[1] & 0x7F) << 16) | (val[2] << 8) | val[3];
	return value;
}

int32_t MCP3561_ReadADCData_32Bit(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin){
	uint8_t val[5] = {0,0,0,0,0};
	uint8_t cmd[5] = {0,0,0,0,0};
	cmd[0] = MCP3561_SREAD_DATA_COMMAND;
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, val, 5, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
	int32_t value = ((val[1] & 0x01) << 31) | (val[2] << 16) | (val[3] << 8) | val[4];
	return value;
}

int32_t MCP3561_ReadADCData_32Bit_Scan(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port,
                                        uint16_t cs_pin, uint8_t *ch_id, uint8_t *status)
{
	uint8_t val[5] = {0,0,0,0,0};
	uint8_t cmd[5] = {0,0,0,0,0};
	cmd[0] = MCP3561_SREAD_DATA_COMMAND;
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, val, 5, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);

	if (status) {
		*status = val[0];
	}
	if (ch_id) {
		*ch_id = (uint8_t)((val[1] & 0xF0) >> 4);
	}

	int32_t value = ((int32_t)(val[1] & 0x01) << 31) |
	               ((int32_t)val[2] << 16) |
	               ((int32_t)val[3] << 8)  |
	               ((int32_t)val[4]);
	return value;
}

uint32_t MCP3561_ReadADCData_IT(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin){
	uint8_t val[5] = {0,0,0,0,0};
	uint8_t cmd[5] = {0,0,0,0,0};
	cmd[0] = MCP3561_SREAD_DATA_COMMAND;

	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, val, 5, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);

	uint32_t value = (val[1] << 16) | (val[2] << 8) | val[3];
	return value;
}
