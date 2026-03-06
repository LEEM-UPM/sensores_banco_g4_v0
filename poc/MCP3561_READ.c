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
#include "stm32g4xx_hal_gpio.h"
#include "stm32g4xx_hal_spi.h"
#include "stm32g4xx_hal_tim.h"
#include "stm32g4xx_hal_adc.h"
#include "stm32g4xx_hal_adc_ex.h"
#include "tim.h"

#if defined(__GNUC__)
#define UNUSED_FUNCTION __attribute__((unused))
#else
#define UNUSED_FUNCTION
#endif


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

static void mcp3564_spi_write(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
		const uint8_t *data, uint16_t size);
static void mcp3564_write_reg8(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
		uint8_t reg_cmd, uint8_t value);
static void mcp3564_write_reg24(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
		uint8_t reg_cmd, uint32_t value);
static void mcp3564_send_cmd(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
		uint8_t cmd);
static void MCP3564_InitRaw(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin) UNUSED_FUNCTION;
static bool MCP3564_ReadOneSample(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
	GPIO_TypeDef *irq_port, uint16_t irq_pin, uint8_t *ch_id, int32_t *code, uint32_t timeout_ms) UNUSED_FUNCTION;

typedef struct {
	uint8_t config0;
	uint8_t config1;
	uint8_t config2;
	uint8_t config3;
	uint8_t irq;
	uint8_t mux;
	uint32_t scan;
	uint32_t timer;
} MCP3561_RegSnapshot;

static MCP3561_RegSnapshot MCP3561_ReadRegistersSnapshot(SPI_HandleTypeDef *hspi,
	GPIO_TypeDef *cs_port, uint16_t cs_pin) UNUSED_FUNCTION;

static volatile MCP3561_RegSnapshot adc1_regs;
static volatile MCP3561_RegSnapshot adc2_regs;
static volatile MCP3561_RegSnapshot adc3_regs;
static volatile MCP3561_RegSnapshot adc4_regs;

static void CAN2_SendCounter(uint32_t can_id, uint8_t *data) UNUSED_FUNCTION;

void MCP3561_InitSimple(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);

// Variables externas
extern SPI_HandleTypeDef hspi1; 
extern TIM_HandleTypeDef htim5;

static ADC_HandleTypeDef hadc1;

volatile uint32_t adc_val;
volatile float adc_voltage;
volatile bool setup_done = false;

volatile int32_t adc_diff_a1 = 0;
volatile int32_t adc_diff_b1 = 0;
volatile int32_t adc_diff_c1 = 0;
volatile int32_t adc_diff_d1 = 0;

volatile int32_t adc_diff_a2 = 0;
volatile int32_t adc_diff_b2 = 0;
volatile int32_t adc_diff_c2 = 0;
volatile int32_t adc_diff_d2 = 0;

volatile int32_t adc_diff_a3 = 0;
volatile int32_t adc_diff_b3 = 0;
volatile int32_t adc_diff_c3 = 0;
volatile int32_t adc_diff_d3 = 0;

volatile int32_t adc_diff_a4 = 0;
volatile int32_t adc_diff_b4 = 0;
volatile int32_t adc_diff_c4 = 0;
volatile int32_t adc_diff_d4 = 0;

volatile uint8_t adc_meta_byte = 0;
volatile uint8_t adc_ch_id_dbg = 0;

uint32_t t0;

static bool MCP3561_ReadManualAveraged(SPI_HandleTypeDef *hspi,
                                       GPIO_TypeDef *cs_port,
                                       uint16_t cs_pin,
                                       uint8_t samples,
                                       int32_t *avg_code)
{
	if ((samples == 0U) || (avg_code == NULL)) {
		return false;
	}

	int64_t acc = 0;
	uint8_t collected = 0;
	uint32_t t_start = HAL_GetTick();

	while (collected < samples) {
		uint8_t status = 0;
		int32_t code = MCP3561_ReadADCData_32Bit_Scan(hspi, cs_port, cs_pin, NULL, &status);

		if ((status & MCP3561_DATA_READY_SMASK) != 0U) {
			acc += code;
			collected++;
		}

		if ((HAL_GetTick() - t_start) > ((uint32_t)samples * 150U + 50U)) {
			return false;
		}
	}

	*avg_code = (int32_t)(acc / (int64_t)samples);
	return true;
}

void MCP3561_InitSimple(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
	MCP3561_Reset(hspi, cs_port, cs_pin);
	HAL_Delay(5);
	MCP3561_Init(hspi, cs_port, cs_pin);
	MCP3561_ADC_Start_Restart(hspi, cs_port, cs_pin);
}


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

    MCP3561_InitSimple(&hspi1, CS_ADC_1_GPIO_Port, CS_ADC_1_Pin);
    MCP3561_InitSimple(&hspi1, CS_ADC_2_GPIO_Port, CS_ADC_2_Pin);

    MCP3561_InitSimple(&hspi3, CS_ADC_3_GPIO_Port, CS_ADC_3_Pin);
    MCP3561_InitSimple(&hspi3, CS_ADC_4_GPIO_Port, CS_ADC_4_Pin);

	// En SCAN no se fija un MUX manual: el ADC recorre automaticamente los canales configurados.
	HAL_Delay(20);

    uint8_t cmd[2] = {0};
	cmd[0] = MCP3561_CONFIG0_SREAD;

    uint8_t conf0 = _MCP3561_sread(&hspi1, CS_ADC_2_GPIO_Port, CS_ADC_2_Pin, cmd);
    conf0 = _MCP3561_sread(&hspi1, CS_ADC_1_GPIO_Port, CS_ADC_1_Pin, cmd);

    conf0 = _MCP3561_sread(&hspi3, CS_ADC_3_GPIO_Port, CS_ADC_3_Pin, cmd);
    conf0 = _MCP3561_sread(&hspi3, CS_ADC_4_GPIO_Port, CS_ADC_4_Pin, cmd);

	while(1){
		uint8_t ch_id_1 = 0;
        uint8_t status_1 = 0;

        uint8_t ch_id_2 = 0;
        uint8_t status_2 = 0;

        uint8_t ch_id_3 = 0;
        uint8_t status_3 = 0;

        uint8_t ch_id_4 = 0;
        uint8_t status_4 = 0;
                
		int32_t code_1 = MCP3561_ReadADCData_32Bit_Scan(&hspi1, CS_ADC_1_GPIO_Port,
				CS_ADC_1_Pin, &ch_id_1, &status_1);		
                
		int32_t code_2 = MCP3561_ReadADCData_32Bit_Scan(&hspi1, CS_ADC_2_GPIO_Port,
				CS_ADC_2_Pin, &ch_id_2, &status_2);

        int32_t code_3 = MCP3561_ReadADCData_32Bit_Scan(&hspi3, CS_ADC_3_GPIO_Port,
				CS_ADC_3_Pin, &ch_id_3, &status_3);

        int32_t code_4 = MCP3561_ReadADCData_32Bit_Scan(&hspi3, CS_ADC_4_GPIO_Port,
				CS_ADC_4_Pin, &ch_id_4, &status_4);
		

		// DR is active-low in STATUS: 0 means a new conversion result is ready.
		if ((status_1 & MCP3561_DATA_READY_SMASK) == 0U) {
			switch (ch_id_1) {
				case 8: // DIFF_A = CH0-CH1
					adc_diff_a1 = code_1;
					break;
				case 9: // DIFF_B = CH2-CH3
					adc_diff_b1 = code_1;
					break;
				case 10: // DIFF_C = CH4-CH5
					adc_diff_c1 = code_1;
					break;
				case 11: // DIFF_D = CH6-CH7
					adc_diff_d1 = code_1;
					break;
				default:
					break;
			}

		}
		if ((status_2 & MCP3561_DATA_READY_SMASK) == 0U) {
			switch (ch_id_2) {
				case 8: // DIFF_A = CH0-CH1
					adc_diff_a2 = code_2;
					break;
				case 9: // DIFF_B = CH2-CH3
					adc_diff_b2 = code_2;
					break;
				case 10: // DIFF_C = CH4-CH5
					adc_diff_c2 = code_2;
					break;
				case 11: // DIFF_D = CH6-CH7
					adc_diff_d2 = code_2;
					break;
				default:
					break;
			}
		}

		if ((status_3 & MCP3561_DATA_READY_SMASK) == 0U) {
			switch (ch_id_3) {
				case 8: // DIFF_A = CH0-CH1
					adc_diff_a3 = code_3;
					break;
				case 9: // DIFF_B = CH2-CH3
					adc_diff_b3 = code_3;
					break;
				case 10: // DIFF_C = CH4-CH5
					adc_diff_c3 = code_3;
					break;
				case 11: // DIFF_D = CH6-CH7
					adc_diff_d3 = code_3;
					break;
				default:
					break;
			}
        }
		if ((status_4 & MCP3561_DATA_READY_SMASK) == 0U) {
			switch (ch_id_4) {
				case 8: // DIFF_A = CH0-CH1
					adc_diff_a4 = code_4;
					break;
				case 9: // DIFF_B = CH2-CH3
					adc_diff_b4 = code_4;
					break;
				case 10: // DIFF_C = CH4-CH5
					adc_diff_c4 = code_4;
					break;
				case 11: // DIFF_D = CH6-CH7
					adc_diff_d4 = code_4;
					break;
				default:
					break;
			}
		}
                
					/*
			// Ultima muestra valida para inspeccion rapida en debugger.
			adc_val = (uint32_t)code_1;
                        adc_voltage = (float)code_1 * (3.3f / 8388608.0f);
            */
	}
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
	uint32_t reg24 = 0;
	
	// Following the MCP3561 Errata Sheet recommendation (section 3) when internal oscillator is selected
	uint32_t reg_val = 0x900F00;
	cmd[0]  = MCP3561_RSVD_WRITE;
	cmd[1] = (uint8_t)((reg_val >> 16) & 0xff);
	cmd[2] = (uint8_t)((reg_val >>  8) & 0xff);
	cmd[3] = (uint8_t)((reg_val)       & 0xff);
	_MCP3561_write(hspi, cs_port, cs_pin, cmd, 4);

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
	cmd[1]  = (MCP3561_CONFIG3_CONV_MODE_CONTINUOUS |
	           MCP3561_CONFIG3_DATA_FORMAT_32BIT_CHID_SGN |
	           MCP3561_CONFIG3_CRCCOM_OFF |
	           MCP3561_CONFIG3_GAINCAL_OFF |
	           MCP3561_CONFIG3_OFFCAL_OFF);
	_MCP3561_write(hspi, cs_port, cs_pin, cmd, 2);

	cmd[0]  = MCP3561_IRQ_WRITE;
	cmd[1]  = (MCP3561_IRQ_MODE_IRQ_HIGH | MCP3561_IRQ_FASTCMD_ON | MCP3561_IRQ_STP_ON);
	_MCP3561_write(hspi, cs_port, cs_pin, cmd, 2);

	// SCAN multicanal habilitado segun configuracion del proyecto.
	reg24 = MCP3561_USERCONF_SCAN_REG;
	cmd[0] = MCP3561_SCAN_WRITE;
	cmd[1] = (uint8_t)((reg24 >> 16) & 0xFF);
	cmd[2] = (uint8_t)((reg24 >> 8) & 0xFF);
	cmd[3] = (uint8_t)(reg24 & 0xFF);
	_MCP3561_write(hspi, cs_port, cs_pin, cmd, 4);

	reg24 = MCP3561_USERCONF_TIMER_VAL;
	cmd[0] = MCP3561_TIMER_WRITE;
	cmd[1] = (uint8_t)((reg24 >> 16) & 0xFF);
	cmd[2] = (uint8_t)((reg24 >> 8) & 0xFF);
	cmd[3] = (uint8_t)(reg24 & 0xFF);
	_MCP3561_write(hspi, cs_port, cs_pin, cmd, 4);


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
	UNUSED(reg8);

	/* @todo all the remaining registers, registros externos tocará..*/ 
}

static MCP3561_RegSnapshot MCP3561_ReadRegistersSnapshot(SPI_HandleTypeDef *hspi,
		GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
	MCP3561_RegSnapshot snap = {0};
	uint8_t cmd [5] = {0,0,0,0,0};
	uint8_t resp[5] = {0,0,0,0,0};

	cmd[0] = MCP3561_CONFIG0_SREAD;
	snap.config0 = _MCP3561_sread(hspi, cs_port, cs_pin, cmd);

	cmd[0] = MCP3561_CONFIG1_SREAD;
	snap.config1 = _MCP3561_sread(hspi, cs_port, cs_pin, cmd);

	cmd[0] = MCP3561_CONFIG2_SREAD;
	snap.config2 = _MCP3561_sread(hspi, cs_port, cs_pin, cmd);

	cmd[0] = MCP3561_CONFIG3_SREAD;
	snap.config3 = _MCP3561_sread(hspi, cs_port, cs_pin, cmd);

	cmd[0] = MCP3561_IRQ_SREAD;
	snap.irq = _MCP3561_sread(hspi, cs_port, cs_pin, cmd);

	cmd[0] = MCP3561_MUX_SREAD;
	snap.mux = _MCP3561_sread(hspi, cs_port, cs_pin, cmd);

	cmd[0] = MCP3561_SCAN_SREAD;
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, resp, 4, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
	snap.scan = ((uint32_t)resp[1] << 16) | ((uint32_t)resp[2] << 8) | resp[3];

	cmd[0] = MCP3561_TIMER_SREAD;
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hspi, cmd, resp, 4, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
	snap.timer = ((uint32_t)resp[1] << 16) | ((uint32_t)resp[2] << 8) | resp[3];

	return snap;
}

static void mcp3564_spi_write(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
		const uint8_t *data, uint16_t size)
{
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(hspi, (uint8_t *)data, size, MCP3561_HAL_TIMEOUT);
	HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
}

static void mcp3564_write_reg8(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
		uint8_t reg_cmd, uint8_t value)
{
	uint8_t buf[2] = {reg_cmd, value};
	mcp3564_spi_write(hspi, cs_port, cs_pin, buf, 2);
}

static void mcp3564_write_reg24(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
		uint8_t reg_cmd, uint32_t value)
{
	uint8_t buf[4] = {
		reg_cmd,
		(uint8_t)((value >> 16) & 0xFF),
		(uint8_t)((value >> 8) & 0xFF),
		(uint8_t)(value & 0xFF)
	};
	mcp3564_spi_write(hspi, cs_port, cs_pin, buf, 4);
}

static void mcp3564_send_cmd(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
		uint8_t cmd)
{
	mcp3564_spi_write(hspi, cs_port, cs_pin, &cmd, 1);
}

static void MCP3564_InitRaw(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
	mcp3564_send_cmd(hspi, cs_port, cs_pin, DEVICE_RESET_COMMAND);
	HAL_Delay(5);

	// Errata reserved write for internal oscillator
	mcp3564_write_reg24(hspi, cs_port, cs_pin, MCP3561_RSVD_WRITE, 0x900F00);

	// 8-bit CONFIG registers
	mcp3564_write_reg8(hspi, cs_port, cs_pin, MCP3561_CONFIG0_WRITE, MCP3561_USERCONF_REG0);
	mcp3564_write_reg8(hspi, cs_port, cs_pin, MCP3561_CONFIG1_WRITE, MCP3561_USERCONF_REG1);
	mcp3564_write_reg8(hspi, cs_port, cs_pin, MCP3561_CONFIG2_WRITE, (uint8_t)(MCP3561_USERCONF_REG2 + 3));
	mcp3564_write_reg8(hspi, cs_port, cs_pin, MCP3561_CONFIG3_WRITE, MCP3561_USERCONF_REG3);
	mcp3564_write_reg8(hspi, cs_port, cs_pin, MCP3561_IRQ_WRITE, MCP3561_USERCONF_IRQ_REG);

#ifdef MCP3561_USERCONF_SCAN_ENABLE
	mcp3564_write_reg24(hspi, cs_port, cs_pin, MCP3561_SCAN_WRITE, MCP3561_USERCONF_SCAN_REG);
	mcp3564_write_reg24(hspi, cs_port, cs_pin, MCP3561_TIMER_WRITE, MCP3561_USERCONF_TIMER_VAL);
#endif

	mcp3564_send_cmd(hspi, cs_port, cs_pin, DEVICE_ADC_START_RESTART_COMMAND);
}

static bool MCP3564_ReadOneSample(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
		GPIO_TypeDef *irq_port, uint16_t irq_pin, uint8_t *ch_id, int32_t *code, uint32_t timeout_ms)
{
	uint32_t t0 = HAL_GetTick();
	UNUSED(t0);
	UNUSED(irq_port);
	UNUSED(irq_pin);
	UNUSED(timeout_ms);

	uint8_t status = 0;
	int32_t value = MCP3561_ReadADCData_32Bit_Scan(hspi, cs_port, cs_pin, ch_id, &status);
	if ((status & MCP3561_DATA_READY_SMASK) == 0) {
		return false;
	}
	*code = value;
	return true;
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
	adc_meta_byte = val[1];
	if (ch_id) {
		// In DATA_FORMAT_32BIT_CHID_SGN, CH_ID is in the upper nibble of this byte.
		*ch_id = (uint8_t)((val[1] >> 4) & 0x0F);
		adc_ch_id_dbg = *ch_id;
	}

	int32_t value = ((int32_t)val[2] << 16) |
	               ((int32_t)val[3] << 8)  |
	               ((int32_t)val[4]);
	if (value & 0x800000) {
		value |= 0xFF000000; // sign-extend 24-bit
	}
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

static void CAN2_SendCounter(uint32_t can_id, uint8_t *data)
{
	HAL_StatusTypeDef status;
	FDCAN_TxHeaderTypeDef txHeader;
	uint8_t txData[8];

	txHeader.Identifier          = can_id;
	txHeader.IdType              = FDCAN_STANDARD_ID;
	txHeader.TxFrameType         = FDCAN_DATA_FRAME;
	txHeader.DataLength          = FDCAN_DLC_BYTES_8;
	txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	txHeader.BitRateSwitch       = FDCAN_BRS_OFF;
	txHeader.FDFormat            = FDCAN_CLASSIC_CAN;
	txHeader.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
	txHeader.MessageMarker       = 0;

	/* Copy 8 bytes from input parameter to tx buffer */
	for (uint8_t i = 0; i < 8; i++)
	{
		txData[i] = data[i];
	}

	status = HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &txHeader, txData);
	if (status != HAL_OK)
	{
		Error_Handler();
	}
}