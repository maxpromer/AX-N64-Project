/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define LCD_COMMAND 0
#define LCD_DATA    1

void lcd_nop() {
	HAL_Delay(1);
	HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, 1);
	HAL_Delay(1);
	HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, 0);
}

void lcd_data(uint8_t data) {
	HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, (data&0x10) != 0 ? 1 : 0);
	HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, (data&0x20) != 0 ? 1 : 0);
	HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, (data&0x40) != 0 ? 1 : 0);
	HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, (data&0x80) != 0 ? 1 : 0);
}

void lcd_write(uint8_t command_or_data, uint8_t data) {
	HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, command_or_data);
	
	lcd_data(data&0xF0);
	lcd_nop();
	lcd_data(data<<4);
	lcd_nop();
}

void lcd_set_cursor(uint8_t line, uint8_t b) {
  lcd_write(LCD_COMMAND, (line==1 ? 0x80 : 0xC0) + b);
}

void lcd_print(char *buf) {
	int index=0;
  while(buf[index] != 0) {
    lcd_write(LCD_DATA, buf[index]);
    index++;
  }
}

void lcd_clear() {
	lcd_write(LCD_COMMAND, 0x01);
}

void lcd_init() {
	HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, 1);
	
	HAL_Delay(20);
	
	HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, 0);
	HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, 0);
	
	lcd_data(0x30);
	lcd_nop(); 
	lcd_nop(); 
	lcd_nop();
	lcd_data(0x20);
	lcd_nop(); 
	lcd_nop();
	lcd_data(0x10);
	lcd_nop();
	lcd_data(0x00);
	lcd_nop();
	lcd_data(0xC0); // 0xD0 Show Cuser, 0xC0 Hide Cuser
	lcd_nop();
	
	lcd_write(LCD_COMMAND, 0x28);
	lcd_clear();
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
  /* USER CODE BEGIN 2 */
  lcd_init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		lcd_set_cursor(1, 0);
		lcd_print("Hello, AX-NC64 !");
		
		lcd_set_cursor(2, 0);
		lcd_print("     E-TECH     ");
		
		HAL_Delay(1000);
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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_RS_Pin|LCD_RW_Pin|LCD_EN_Pin|LCD_D4_Pin 
                          |LCD_D5_Pin|LCD_D6_Pin|LCD_D7_Pin|LCD_BL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LCD_RS_Pin LCD_RW_Pin LCD_EN_Pin LCD_D4_Pin 
                           LCD_D5_Pin LCD_D6_Pin LCD_D7_Pin LCD_BL_Pin */
  GPIO_InitStruct.Pin = LCD_RS_Pin|LCD_RW_Pin|LCD_EN_Pin|LCD_D4_Pin 
                          |LCD_D5_Pin|LCD_D6_Pin|LCD_D7_Pin|LCD_BL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
