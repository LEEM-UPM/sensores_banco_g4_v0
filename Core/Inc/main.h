/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MCLK_3_Pin GPIO_PIN_13
#define MCLK_3_GPIO_Port GPIOC
#define CS_ADC_3_Pin GPIO_PIN_14
#define CS_ADC_3_GPIO_Port GPIOC
#define IRQ_3_Pin GPIO_PIN_15
#define IRQ_3_GPIO_Port GPIOC
#define ASENSING5V_Pin GPIO_PIN_0
#define ASENSING5V_GPIO_Port GPIOA
#define VSENSING3V3_Pin GPIO_PIN_1
#define VSENSING3V3_GPIO_Port GPIOA
#define ASENSING3V3_Pin GPIO_PIN_2
#define ASENSING3V3_GPIO_Port GPIOA
#define LED1_Pin GPIO_PIN_3
#define LED1_GPIO_Port GPIOA
#define MCLK2_Pin GPIO_PIN_4
#define MCLK2_GPIO_Port GPIOA
#define SCK_1_Pin GPIO_PIN_5
#define SCK_1_GPIO_Port GPIOA
#define MISO_1_Pin GPIO_PIN_6
#define MISO_1_GPIO_Port GPIOA
#define MOSI_1_Pin GPIO_PIN_7
#define MOSI_1_GPIO_Port GPIOA
#define CS_ADC_2_Pin GPIO_PIN_0
#define CS_ADC_2_GPIO_Port GPIOB
#define IRQ_2_Pin GPIO_PIN_1
#define IRQ_2_GPIO_Port GPIOB
#define MCLK_1_Pin GPIO_PIN_2
#define MCLK_1_GPIO_Port GPIOB
#define IRQ_1_Pin GPIO_PIN_10
#define IRQ_1_GPIO_Port GPIOB
#define ps_no_configurado_Pin GPIO_PIN_11
#define ps_no_configurado_GPIO_Port GPIOB
#define INT0_ITDS_Pin GPIO_PIN_14
#define INT0_ITDS_GPIO_Port GPIOB
#define VSENSING5V_Pin GPIO_PIN_15
#define VSENSING5V_GPIO_Port GPIOB
#define SDA_Pin GPIO_PIN_8
#define SDA_GPIO_Port GPIOA
#define SCL_Pin GPIO_PIN_9
#define SCL_GPIO_Port GPIOA
#define CS_ADC_1_Pin GPIO_PIN_10
#define CS_ADC_1_GPIO_Port GPIOA
#define CAN_1_STB_Pin GPIO_PIN_15
#define CAN_1_STB_GPIO_Port GPIOA
#define SCK_3_Pin GPIO_PIN_3
#define SCK_3_GPIO_Port GPIOB
#define MISO_3_Pin GPIO_PIN_4
#define MISO_3_GPIO_Port GPIOB
#define MOSI_3_Pin GPIO_PIN_5
#define MOSI_3_GPIO_Port GPIOB
#define MCLK_4_Pin GPIO_PIN_6
#define MCLK_4_GPIO_Port GPIOB
#define IRQ_4_Pin GPIO_PIN_7
#define IRQ_4_GPIO_Port GPIOB
#define CS_ADC_4_Pin GPIO_PIN_9
#define CS_ADC_4_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
