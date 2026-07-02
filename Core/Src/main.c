/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "usart.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define N 32
uint16_t cnt = 0;
uint32_t adc_buff[3*N];
uint32_t adc_output[3];

__IO ITStatus UartReady = RESET;
uint16_t tmp0;
uint8_t tmp1;
uint8_t tmp2;
uint8_t tmp3;
uint8_t tmp4;
uint8_t tmp5;
uint8_t tmp6;
uint16_t tmp7;

char status_machine;
//char *g="sendme";
char *g="cls BLACK";
char *h="page page2";
char *k="n0.val=";
char *m="add 1,1,80";    //white line
char *mn="add 1,1,";     //real time data
char *o="add 1,2,107";   //red line
char *n="add 1,3,57";    //green line
uint8_t end[3]={0xff,0xff,0xff};
uint8_t data[4];
char wave_cmd[24];
uint8_t hmi_tx2_status;
uint8_t hmi_tx3_status;
uint16_t hmi_ref_cnt;

/* Buffer used for transmission */
uint8_t aTxBuffer[32];
/* Buffer used for reception */
uint8_t aRxBuffer[20]="";

uint32_t rxcnt;
uint8_t rx1byte[32];
uint8_t rx2byte[32];
uint8_t rx3byte[32];
uint8_t tx1byte[8];
uint8_t tx2byte[8];
uint8_t tx3byte[8];
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  int adc_buff1[3];
  status_machine = 0xA5;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  HAL_Delay(1000);  //waiting for pannel launching about 3s
  HAL_Delay(1000); 
  HAL_Delay(1000);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_LPUART1_UART_Init();
  MX_ADC1_Init();
  MX_TIM8_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  
  SPI_DAC_InitDAC();
  /* USER CODE BEGIN 2 */
  HAL_Delay(1);
  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
  HAL_TIM_Base_Start(&htim8);
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET);  //LED1 INIT
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_SET);  //LED2 INIT
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);  //RELAY INIT
  HAL_Delay(10);
  
  HAL_UART_Receive_IT(&huart2,(uint8_t*)aRxBuffer,5);
  HAL_UART_Transmit(&huart2,end,3,100);
  HAL_UART_Transmit(&huart2,g,strlen(g),100);
  HAL_UART_Transmit(&huart2,end,3,100); 
  HAL_UART_Transmit(&huart2,end,3,100);
  HAL_UART_Transmit(&huart2,h,strlen(h),100);
  HAL_UART_Transmit(&huart2,end,3,100);
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET)
    {
       HAL_Delay(10);  //��������
       if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET)
       {
         status_machine = ~(status_machine);
         
       }
    }
    if(status_machine == 0x5A)
    {
      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
    }
    else
    {
      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
    }
    
    adc_buff1[0] = 0;
    adc_buff1[1] = 0;
    adc_buff1[2] = 0;
    HAL_ADC_Start_DMA(&hadc1,(uint32_t*)adc_buff,3*N);
    for (int i = 0;i < N; ++i)  // N == 1024
    {
      adc_buff1[0] += adc_buff[3 * i];
      adc_buff1[1] += adc_buff[3 * i + 1];
      adc_buff1[2] += adc_buff[3 * i + 2];
    }

    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_RESET);
    
    adc_output[0] = adc_buff1[0] * 3300 / (4096 * N);
    adc_output[1] = adc_buff1[1] * 3300 / (4096 * N);  //0-3300
    //adc_output[2] = adc_buff1[2] * 3.3 / 4096 / N;
    adc_output[2] = adc_buff1[2] * 3300 / (4096 * N);
    
    printf("%lu,%lu\r\n", HAL_GetTick(), adc_output[2]);
    
    tmp0 = adc_output[2]%1000;
    tmp1 = adc_output[2]/1000+0x30;
    tmp2 = tmp0%100;
    tmp3 = tmp0/100+0x30;  //��λ��
    tmp4 = tmp2%10;
    tmp5 = tmp2/10+0x30;  //ʮλ��
    tmp6 = tmp4+0x30;  //��λ��    
    
    if (adc_output[2] <= 1500)
    {
      tmp7 = 0;
    }
    else if (adc_output[2] >= 2000)
    {
      tmp7 = 150;
    }
    else
    {
      tmp7 = (adc_output[2] - 1500) * 150 / 500;
    }
    sprintf(wave_cmd, "add 1,0,%d", tmp7);

    //transfer data to pannel    
    data[0] = tmp1;  
    data[1] = tmp3;
    data[2] = tmp5;
    hmi_tx2_status = HAL_UART_Transmit(&huart2,(uint8_t*)wave_cmd,strlen(wave_cmd),100);
    HAL_UART_Transmit(&huart2,end,3,100);

    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_14,GPIO_PIN_SET);
    HAL_Delay(2);
    
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/*
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == htim8.Instance)
    {
        HAL_ADC_Start_DMA(&hadc1,(uint32_t*)adcbuff,3);
        if (adcbuff[0] > 1 || (adcbuff[1] > 1 || adcbuff[2] > 1))
        {
            trigger_flag = 1;
        }
    }

}
 */



/**
  * @brief  Tx Transfer completed callback
  * @param  UartHandle: UART handle. 
  * @note   This example shows a simple way to report end of IT Tx transfer, and 
  *         you can add your own implementation. 
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  /* Set transmission flag: transfer complete*/
  UartReady = SET;
}

void HMISends(char *buf)
{
    uint8_t i=0;
    while(1)
    {
      if (buf[i]!=0)
      {
        HAL_UART_Transmit_IT(&huart2, (uint8_t *)&buf, 0x1);
        HAL_Delay(2);
        i++;
      }
      else
      {
        return ;
      }
    }
}

/**
  * @brief  Rx Transfer completed callback
  * @param  UartHandle: UART handle
  * @note   This example shows a simple way to report end of IT Rx transfer, and 
  *         you can add your own implementation.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	aRxBuffer[rxcnt] = rx1byte[0];     //��������ת��������
	rxcnt=rxcnt+1;
 
	if (aRxBuffer[rxcnt-1] == 0xFF  && aRxBuffer[rxcnt-4] == 0x0a)  //�жϽ������ݵ�֡ͷ֡β
	{
		//HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
        //printf("show_trans.n1.val=%d\xff\xff\xff",aRxBuffer[rx_cnt-3]+aRxBuffer[rx_cnt-2]*256);
		//���յ������ݷ��ͻش�����
		rxcnt =0;
	}
	 HAL_UART_Receive_IT(&hlpuart1, (uint8_t *)&rx1byte, 1);//�ٿ����ж�
}

void SPI_DAC_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  
  /* Configure SPI1 pins: SCK, and MOSI */
  GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_5;  //PA7 - SPI_DATA;PA5 - SPI_CLK
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Configure PD14 Chip select��CS */ 
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}


void SPI_DAC_data_send(unsigned char Passage, unsigned int data)
{
  unsigned char i, mode;

  HAL_GPIO_WritePin(GPIOD,DAC1_SYNC,GPIO_PIN_SET); //DAC1_SYNC = 1;
  HAL_GPIO_WritePin(GPIOA,DAC_SCLK,GPIO_PIN_RESET);  //DAC_SCLK = 0;
  HAL_Delay(1);

  switch(Passage)
  {
    case(0x0): 
      {
        mode = 0x18;
        HAL_GPIO_WritePin(GPIOD,DAC1_SYNC,GPIO_PIN_SET); //DAC1_SYNC = 1;
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOD,DAC1_SYNC,GPIO_PIN_RESET); //DAC1_SYNC = 0;
        HAL_Delay(1);
      }
      break;	
    case(0x1): 
      {
        mode = 0x19;
        HAL_GPIO_WritePin(GPIOD,DAC1_SYNC,GPIO_PIN_SET); //DAC1_SYNC = 1;
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOD,DAC1_SYNC,GPIO_PIN_RESET); //DAC1_SYNC = 0;
        HAL_Delay(1);
      }
      break;
    default:
      break;
  }

//8bits command and address, 24bits data in frame totally.
  for(i = 0; i < 8; i++) 
  {
    if(mode & 0x80) 
    {
      HAL_GPIO_WritePin(GPIOA,DAC_DIN,GPIO_PIN_SET); //DAC_DIN = 1;  
    }
    else
    {
      HAL_GPIO_WritePin(GPIOA,DAC_DIN,GPIO_PIN_RESET);  //DAC_DIN = 0;
    }
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA,DAC_SCLK,GPIO_PIN_SET);  //DAC_SCLK = 1;  
    HAL_Delay(1);
    mode <<= 1;
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA,DAC_SCLK,GPIO_PIN_RESET);  //DAC_SCLK = 0;
    HAL_Delay(1);
  }
  
//to transfer 16bits data
  for(i = 0; i < 16; i++) 
  {
    if(data & 0x8000) 
    {
       HAL_GPIO_WritePin(GPIOA,DAC_DIN,GPIO_PIN_SET); //DAC_DIN = 1;  
    }
    else
    {
       HAL_GPIO_WritePin(GPIOA,DAC_DIN,GPIO_PIN_RESET);  //DAC_DIN = 0;
    }
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA,DAC_SCLK,GPIO_PIN_SET);  //DAC_SCLK = 1; 
    HAL_Delay(1);
    data <<= 1;
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA,DAC_SCLK,GPIO_PIN_RESET);  //DAC_SCLK = 0;
    HAL_Delay(1);
  }
    HAL_GPIO_WritePin(GPIOD,DAC1_SYNC,GPIO_PIN_SET); //DAC1_SYNC = 1; 
}


void SPI_DAC_command_send(unsigned char Passage, unsigned char  mode_in, unsigned int data)
{
  unsigned char i, mode;
  
  mode = mode_in;
  HAL_GPIO_WritePin(GPIOD,DAC1_SYNC,GPIO_PIN_SET);     //DAC1_SYNC = 1;
  HAL_GPIO_WritePin(GPIOA,DAC_SCLK,GPIO_PIN_RESET);   //DAC_SCLK = 0;
  HAL_Delay(1);

  switch(Passage)
  {
    case(0x0): 
      {
        HAL_GPIO_WritePin(GPIOD,DAC1_SYNC,GPIO_PIN_SET);     //DAC1_SYNC = 1;
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOD,DAC1_SYNC,GPIO_PIN_RESET);    //DAC1_SYNC = 0;
        HAL_Delay(1);
      }
      break;	
    case(0x1): 
      {
        HAL_GPIO_WritePin(GPIOD,DAC1_SYNC,GPIO_PIN_SET);       //DAC1_SYNC = 1;
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOD,DAC1_SYNC,GPIO_PIN_RESET);      //DAC1_SYNC = 0;
        HAL_Delay(1);
      }
      break;
    default:
      break;
  }

//8bits command and address, 24bits data in frame totally.
  for(i = 0; i < 8; i++) 
  {
    if(mode & 0x80) 
    {
      HAL_GPIO_WritePin(GPIOA,DAC_DIN,GPIO_PIN_SET);   //DAC_DIN = 1;
    }
    else
    {
      HAL_GPIO_WritePin(GPIOA,DAC_DIN,GPIO_PIN_RESET);  //DAC_DIN = 0;
    }
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA,DAC_SCLK,GPIO_PIN_SET);    //DAC_SCLK = 1;
    HAL_Delay(1);
    mode <<= 1;
    HAL_Delay(1);
     HAL_GPIO_WritePin(GPIOA,DAC_SCLK,GPIO_PIN_RESET);   //DAC_SCLK = 0;
    HAL_Delay(1);
  }
  
//to transfer 16bits data
  for(i = 0; i < 16; i++) 
  {
    if(data & 0x8000) 
    {
       HAL_GPIO_WritePin(GPIOA,DAC_DIN,GPIO_PIN_SET);   //DAC_DIN = 1;
    }
    else
    {
       HAL_GPIO_WritePin(GPIOA,DAC_DIN,GPIO_PIN_RESET);   //DAC_DIN = 0;
    }
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA,DAC_SCLK,GPIO_PIN_SET);    //DAC_SCLK = 1;
    HAL_Delay(1);
    data <<= 1;
    HAL_Delay(1); 
    HAL_GPIO_WritePin(GPIOA,DAC_SCLK,GPIO_PIN_RESET);    //DAC_SCLK = 0;
    HAL_Delay(1);
  }
    HAL_GPIO_WritePin(GPIOD,DAC1_SYNC,GPIO_PIN_SET);       //DAC1_SYNC = 1;
}


void SPI_DAC_AccInit_Val(void)
{
  //double Val = 0.15*18;
  uint16_t Val = 0.4*20;
  uint16_t ValTmp = 0;

  Val = (0x20)*65530/50;
  ValTmp = Val;

  if(Val-ValTmp>=0.5)//avoid 65536
  {
    ValTmp = Val+1;
  }
  SPI_DAC_data_send(DACPassageA, ValTmp);
  SPI_DAC_data_send(DACPassageB, ValTmp);
}

//DAC8162 REGISTERs Configuration
void SPI_DAC_reg_init(void)
{
  SPI_DAC_command_send(0, 0x3f, 1); ///Enable internal reference
  SPI_DAC_command_send(1, 0x3f, 1);

  SPI_DAC_command_send(0, 0x20, 3); //Power up
  SPI_DAC_command_send(1, 0x20, 3);

  SPI_DAC_command_send(0, 0x30, 3);
  SPI_DAC_command_send(1, 0x30, 3);

  SPI_DAC_AccInit_Val();  //��ʼ��DA���ֵ
}

void SPI_DAC_InitDAC(void)
{
  SPI_DAC_Init( );
  SPI_DAC_reg_init();
  //SPI_DAC_data_send(DACPassageB,0);  //0ͨ���������0
}

void SPI_DAC_Acc_Val(unsigned char Passage, double Val)
{
  double ValTmp;
  Val = Val*65530/5;
  ValTmp = Val;

  if(Val-ValTmp>=0.5)//avoid 65536
  {
     ValTmp = Val+1;
  }
  switch(Passage)
  {
    case(0x0): 
      SPI_DAC_data_send(DACPassageB,ValTmp);
      break;	
    case(0x1): 
      SPI_DAC_data_send(DACPassageA,ValTmp);
      break;
    default:
      break;  
  }
}

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
