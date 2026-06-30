/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32l4xx_hal.h"

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
void HMISends(char *buf);
void SPI_DAC_Init(void);
void SPI_DAC_data_send(unsigned char Passage, unsigned int data);
void SPI_DAC_command_send(unsigned char Passage, unsigned char  mode_in, unsigned int data);
void SPI_DAC_AccInit_Val(void);
void SPI_DAC_reg_init(void);
void SPI_DAC_InitDAC(void);
void SPI_DAC_Acc_Val(unsigned char Passage, double Val);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */
#define DAC1_SYNC 	GPIO_PIN_14
#define DAC_SCLK 	GPIO_PIN_5
#define DAC_DIN  	GPIO_PIN_7

#define DACLimit 	50 //2.5V
#define DACMinVal 	0x23 //35/20V=1.75V
#define DACScale 	3
#define DACSendTimes 	5
#define DACPassageA  	1
#define DACPassageB  	0
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
