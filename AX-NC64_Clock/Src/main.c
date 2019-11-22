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
#include <stdio.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define DS1307_ADDR (0x68<<1)
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t bcd2dec(uint8_t num) {
  return ((num/16 * 10) + (num % 16));
}

uint8_t read_time(uint8_t *hour, uint8_t *min, uint8_t *sec, uint8_t *date, uint8_t *month, uint8_t *year) {
  uint8_t buff[7];
	
  // Set address to read
  buff[0] = 0x0; // read start at address 0x0
  if (HAL_I2C_Master_Transmit(&hi2c2, DS1307_ADDR, buff, 1, 1000) == HAL_ERROR) {
    // Fail !, Write error !
    return 0;
  }
	
  HAL_Delay(1);
	
  // Read 7 bytes
  if (HAL_I2C_Master_Receive(&hi2c2, DS1307_ADDR, buff, 7, 1000) == HAL_ERROR) {
    // Fail !, read error !
    return 0;
  }
	
  *sec = bcd2dec(buff[0] & 0x7f);
  *min = bcd2dec(buff[1]);
  *hour = bcd2dec(buff[2] & 0x3f);
  *date = bcd2dec(buff[4]);
  *month = bcd2dec(buff[5]);
  *year = bcd2dec(buff[6]);
	
  return 1;
}

// LCD
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

void lcd_write_c(char c) {
  lcd_write(LCD_DATA, c);
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
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  lcd_init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint8_t h, m, s, dd, mm, yy;
		
    lcd_clear();
    if (read_time(&h, &m, &s, &dd, &mm, &yy)) {
      char buff[10];
      
      lcd_set_cursor(1, 0);
      
      sprintf(buff, "%d", dd);
      lcd_print(buff);
      
      lcd_write_c('/');
      
      sprintf(buff, "%d", mm);
      lcd_print(buff);
      
      lcd_write_c('/');
      
      if (yy < 10) lcd_write_c('0');
      sprintf(buff, "%d", yy);
      lcd_print(buff);
      
      lcd_write_c(' ');
      
      if (h < 10) lcd_write_c('0');
      sprintf(buff, "%d", h);
      lcd_print(buff);
      
      lcd_write_c(':');
      
      if (m < 10) lcd_write_c('0');
      sprintf(buff, "%d", m);
      lcd_print(buff);
      
      lcd_write_c(':');
      
      if (s < 10) lcd_write_c('0');
      sprintf(buff, "%d", s);
      lcd_print(buff);
      
    } else {
      lcd_set_cursor(1, 0);
      lcd_print("Error !");
    }
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
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x6010C7FF;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter 
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter 
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

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
