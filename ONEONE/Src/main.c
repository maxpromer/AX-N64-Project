/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>

#include "dotmatrix_8x8.h"
#include "lcd_1602.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim6;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
 uint8_t myfont[37][8]={
		
		{0x70>>2,0x88>>2,0x98>>2,0xA8>>2,0xC8>>2,0x88>>2,0x88>>2,0x70>>2}, //0
		{0x20>>2,0x60>>2,0x20>>2,0x20>>2,0x20>>2,0x20>>2,0x20>>2,0x70>>2}, //1
		{0x70>>2,0x88>>2,0x08>>2,0x10>>2,0x20>>2,0x40>>2,0x80>>2,0xF8>>2}, //2
		{0x70>>2,0x88>>2,0x08>>2,0x30>>2,0x08>>2,0x08>>2,0x88>>2,0x70>>2}, //3
		{0x10>>2,0x30>>2,0x50>>2,0x90>>2,0xF8>>2,0x10>>2,0x10>>2,0x10>>2}, //4
		{0xF8>>2,0x80>>2,0x80>>2,0xF0>>2,0x08>>2,0x08>>2,0x88>>2,0x70>>2}, //5
		{0x18>>2,0x20>>2,0x40>>2,0x80>>2,0xF0>>2,0x88>>2,0x88>>2,0x70>>2}, //6
		{0xF8>>2,0x08>>2,0x10>>2,0x20>>2,0x40>>2,0x40>>2,0x40>>2,0x40>>2}, //7
		{0x70>>2,0x88>>2,0x88>>2,0x70>>2,0x88>>2,0x88>>2,0x88>>2,0x70>>2}, //8
		{0x70>>2,0x88>>2,0x88>>2,0x88>>2,0x78>>2,0x08>>2,0x10>>2,0x60>>2}, //9
		{0x70>>2,0x88>>2,0x88>>2,0x88>>2,0xF8>>2,0x88>>2,0x88>>2,0x88>>2}, // A   ,10
		{0xF0>>2,0x88>>2,0x88>>2,0xF0>>2,0x88>>2,0x88>>2,0x88>>2,0xF0>>2}, // B   ,11
		{0x70>>2,0x88>>2,0x80>>2,0x80>>2,0x80>>2,0x80>>2,0x88>>2,0x70>>2}, // C		,12
		{0xE0>>2,0x90>>2,0x88>>2,0x88>>2,0x88>>2,0x88>>2,0x90>>2,0xE0>>2}, // D		,13
		{0xF8>>2,0x80>>2,0x80>>2,0xF0>>2,0x80>>2,0x80>>2,0x80>>2,0xF8>>2}, // E		,14
		{0xF8>>2,0x80>>2,0x80>>2,0xF0>>2,0x80>>2,0x80>>2,0x80>>2,0x80>>2}, // F		,15
		{0x70>>2,0x88>>2,0x80>>2,0x80>>2,0xB8>>2,0x88>>2,0x88>>2,0x70>>2}, // G		,16
		{0x88>>2,0x88>>2,0x88>>2,0xF8>>2,0x88>>2,0x88>>2,0x88>>2,0x88>>2}, // H		,17
		{0xF8>>2,0x20>>2,0x20>>2,0x20>>2,0x20>>2,0x20>>2,0x20>>2,0xF8>>2}, // I		,18
		{0x08>>2,0x08>>2,0x08>>2,0x08>>2,0x08>>2,0x88>>2,0x88>>2,0x70>>2}, // J		,19
		{0x88>>2,0x90>>2,0xA0>>2,0xC0>>2,0xC0>>2,0xA0>>2,0x90>>2,0x88>>2}, // K		,20
		{0x80>>2,0x80>>2,0x80>>2,0x80>>2,0x80>>2,0x80>>2,0x80>>2,0xF8>>2}, // L		,21
		{0x88>>2,0xD8>>2,0xA8>>2,0xA8>>2,0x88>>2,0x88>>2,0x88>>2,0x88>>2}, // M		,22
		{0x88>>2,0xC8>>2,0xA8>>2,0xA8>>2,0x98>>2,0x88>>2,0x88>>2,0x88>>2}, // N		,23
		{0x70>>2,0x88>>2,0x88>>2,0x88>>2,0x88>>2,0x88>>2,0x88>>2,0x70>>2}, // O		,24
		{0xF0>>2,0x88>>2,0x88>>2,0x88>>2,0xF0>>2,0x80>>2,0x80>>2,0x80>>2}, // P		,25
		{0x70>>2,0x88>>2,0x88>>2,0x88>>2,0x88>>2,0xA8>>2,0x90>>2,0x68>>2}, // Q		,26
		{0xF0>>2,0x88>>2,0x88>>2,0x88>>2,0xF0>>2,0xA0>>2,0x90>>2,0x88>>2}, // R		,27
		{0x78>>2,0x80>>2,0x80>>2,0x70>>2,0x08>>2,0x08>>2,0x08>>2,0xF0>>2}, // S		,28
		{0xF8>>2,0x20>>2,0x20>>2,0x20>>2,0x20>>2,0x20>>2,0x20>>2,0x20>>2}, // T		,29
		{0x88>>2,0x88>>2,0x88>>2,0x88>>2,0x88>>2,0x88>>2,0x88>>2,0x70>>2}, // U   ,30
		{0x88>>2,0x88>>2,0x88>>2,0x88>>2,0x50>>2,0x50>>2,0x20>>2,0x20>>2}, // V		,31
		{0x88>>2,0x88>>2,0x88>>2,0x88>>2,0xA8>>2,0xA8>>2,0xD8>>2,0x88>>2}, // W		,32
		{0x88>>2,0x88>>2,0x50>>2,0x20>>2,0x20>>2,0x50>>2,0x88>>2,0x88>>2}, // X		,33
		{0x88>>2,0x88>>2,0x88>>2,0x50>>2,0x20>>2,0x20>>2,0x20>>2,0x20>>2}, // Y		,34
		{0xF8>>2,0x08>>2,0x10>>2,0x20>>2,0x20>>2,0x40>>2,0x80>>2,0xF8>>2}, // Z 	,35
    {0x00>>2,0x00>>2,0x00>>2,0x00>>2,0x00>>2,0x00>>2,0x00>>2,0x00>>2}, //   	,36
};


uint8_t buff[8];

#define CHAR_TO_FONT_INDEX(a) (a >= '0' && a <= '9' ? a - '0' : a - 'A' + 10)
void DotMatrixTextMove(char *text) {
  for(int i=0;i<8;i++) {
    for(int x=0;x<8;x++) {
      buff[x] = 0x0;
      buff[x] |= myfont[CHAR_TO_FONT_INDEX(text[0])][x]>>(8 - i);
    }
    dotmatrix_put(buff);
    HAL_Delay(100);
  }
  unsigned int slen = strlen((const char*)text);
  for(int i=0;i<(slen *  8);i++) {
    uint8_t text_index = i / 8;
    uint8_t font1 = CHAR_TO_FONT_INDEX(text[text_index]);
    uint8_t font2 = CHAR_TO_FONT_INDEX((text_index + 1 > (slen - 1)) ? 36 : text[text_index + 1]);
    for(int x=0;x<8;x++) {
      buff[x] = myfont[font1][x]<<(i%8);
      buff[x] |= myfont[font2][x]>>(8 - (i%8));
    }
    dotmatrix_put(buff);
    HAL_Delay(100);
  }
}

int counter = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == ENCODER_A_Pin) {
		if (HAL_GPIO_ReadPin(ENCODER_B_GPIO_Port, ENCODER_B_Pin) == 0) {
			counter++;
		} else {
			counter--;
		}
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim6);
  lcd_init();
  lcd_bl(1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == 0) {
      while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == 0) HAL_Delay(10);
      DotMatrixTextMove("TASK1");
      lcd_gotoxy(0, 0);
      lcd_puts("  World skills  ");
      lcd_gotoxy(0, 1);
      lcd_puts("  Thailand 2020 ");
      HAL_Delay(4000);
      lcd_clr();
      while(1) {
        lcd_gotoxy(0, 0);
        
        char buffText[20];
        sprintf(buffText, "Count = %d     ", counter);
        lcd_puts(buffText);
        HAL_Delay(500);
      }
    }
    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 31;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 999;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, DOT_R_RST_Pin|DOT_R_CLK_Pin|DOT_C_DT_Pin|DOT_C_CLK_Pin 
                          |DOT_C_LD_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_RS_Pin|LCD_RW_Pin|LCD_E_Pin|LCD_D4_Pin 
                          |LCD_D5_Pin|LCD_D6_Pin|LCD_D7_Pin|LCD_BL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : ENCODER_A_Pin */
  GPIO_InitStruct.Pin = ENCODER_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ENCODER_A_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ENCODER_B_Pin */
  GPIO_InitStruct.Pin = ENCODER_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ENCODER_B_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DOT_R_RST_Pin DOT_R_CLK_Pin DOT_C_DT_Pin DOT_C_CLK_Pin 
                           DOT_C_LD_Pin */
  GPIO_InitStruct.Pin = DOT_R_RST_Pin|DOT_R_CLK_Pin|DOT_C_DT_Pin|DOT_C_CLK_Pin 
                          |DOT_C_LD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : SW1_Pin SW2_Pin SW3_Pin SW4_Pin */
  GPIO_InitStruct.Pin = SW1_Pin|SW2_Pin|SW3_Pin|SW4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RS_Pin LCD_RW_Pin LCD_E_Pin LCD_D4_Pin 
                           LCD_D5_Pin LCD_D6_Pin LCD_D7_Pin LCD_BL_Pin */
  GPIO_InitStruct.Pin = LCD_RS_Pin|LCD_RW_Pin|LCD_E_Pin|LCD_D4_Pin 
                          |LCD_D5_Pin|LCD_D6_Pin|LCD_D7_Pin|LCD_BL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
