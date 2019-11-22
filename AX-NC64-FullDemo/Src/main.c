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
#include "cmsis_os.h"

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

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// Internal Macros
#define HEX__(n) 0x##n##LU
#define B8__(x) ((x&0x0000000FLU)?1:0) \
+((x&0x000000F0LU)?2:0) \
+((x&0x00000F00LU)?4:0) \
+((x&0x0000F000LU)?8:0) \
+((x&0x000F0000LU)?16:0) \
+((x&0x00F00000LU)?32:0) \
+((x&0x0F000000LU)?64:0) \
+((x&0xF0000000LU)?128:0)

// User-visible Macros
#define B8(d) ((unsigned char)B8__(HEX__(d)))
#define B16(dmsb,dlsb) (((unsigned short)B8(dmsb)<<8) + B8(dlsb))
#define B32(dmsb,db2,db3,dlsb) \
(((unsigned long)B8(dmsb)<<24) \
+ ((unsigned long)B8(db2)<<16) \
+ ((unsigned long)B8(db3)<<8) \
+ B8(dlsb))
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;
DMA_HandleTypeDef hdma_adc;

I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim2;

osThreadId defaultTaskHandle;
osThreadId DotMatrixHandle;
osThreadId LCDHandle;
osThreadId SoundHandle;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC_Init(void);
static void MX_I2C2_Init(void);
static void MX_TIM2_Init(void);
void StartDefaultTask(void const * argument);
void DotMatrixTask(void const * argument);
void LCDTask(void const * argument);
void SoundTask(void const * argument);

/* USER CODE BEGIN PFP */
uint8_t mode = 0;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// ============ LCD ============
#define LCD_COMMAND 0
#define LCD_DATA    1

void lcd_nop() {
	HAL_Delay(1);
	HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, 1);
	HAL_Delay(1);
	HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, 0);
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
	HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, 0);
	
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
// ============ END of LCD ============

// ============ RTC ============
#define DS1307_ADDR (0x68<<1)

uint8_t bcd2dec(uint8_t num) {
  return ((num/16 * 10) + (num % 16));
}

uint8_t read_time(uint8_t *hour, uint8_t *min, uint8_t *sec, uint8_t *date, uint8_t *month, uint8_t *year) {
	uint8_t buff[7];
	
	// Set address to read
	buff[0] = 0x0; // read start at address 0x0
	if (HAL_I2C_Master_Transmit(&hi2c2, DS1307_ADDR, buff, 1, 100) == HAL_ERROR) {
		// Fail !, Write error !
		return 0;
	}
	
	// HAL_Delay(1);
	
	// Read 7 bytes
	if (HAL_I2C_Master_Receive(&hi2c2, DS1307_ADDR, buff, 7, 100) == HAL_ERROR) {
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
// ============ END of RTC ============

// ============== Dot Matrix ==============
uint8_t font_6_8[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x00
	0x00, 0x7c, 0xa2, 0x8a, 0xa2, 0x7c,  // 0x01
	0x00, 0x7c, 0xd6, 0xf6, 0xd6, 0x7c,  // 0x02
	0x00, 0x38, 0x7c, 0x3e, 0x7c, 0x38,  // 0x03
	0x00, 0x18, 0x3c, 0x7e, 0x3c, 0x18,  // 0x04
	0x00, 0x0c, 0x6c, 0xfe, 0x6c, 0x0c,  // 0x05
	0x00, 0x18, 0x3a, 0x7e, 0x3a, 0x18,  // 0x06
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x07
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x08
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x09
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x0a
	0x00, 0x0c, 0x12, 0x52, 0x6c, 0x70,  // 0x0b
	0x00, 0x60, 0x94, 0x9e, 0x94, 0x60,  // 0x0c
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x0d
	0x00, 0x06, 0x7e, 0x50, 0xac, 0xfc,  // 0x0e
	0x00, 0x54, 0x38, 0x6c, 0x38, 0x54,  // 0x0f
	0x00, 0x00, 0xfe, 0x7c, 0x38, 0x10,  // 0x10
	0x00, 0x10, 0x38, 0x7c, 0xfe, 0x00,  // 0x11
	0x00, 0x28, 0x6c, 0xfe, 0x6c, 0x28,  // 0x12
	0x00, 0x00, 0xfa, 0x00, 0xfa, 0x00,  // 0x13
	0x00, 0x60, 0x90, 0xfe, 0x80, 0xfe,  // 0x14
	0x00, 0x44, 0xb2, 0xaa, 0x9a, 0x44,  // 0x15
	0x00, 0x06, 0x06, 0x06, 0x06, 0x00,  // 0x16
	0x00, 0x28, 0x6d, 0xff, 0x6d, 0x28,  // 0x17
	0x00, 0x20, 0x60, 0xfe, 0x60, 0x20,  // 0x18
	0x00, 0x08, 0x0c, 0xfe, 0x0c, 0x08,  // 0x19
	0x00, 0x10, 0x10, 0x7c, 0x38, 0x10,  // 0x1a
	0x00, 0x10, 0x38, 0x7c, 0x10, 0x10,  // 0x1b
	0x00, 0x1e, 0x02, 0x02, 0x02, 0x02,  // 0x1c
	0x00, 0x10, 0x7c, 0x10, 0x7c, 0x10,  // 0x1d
	0x00, 0x0c, 0x3c, 0xfc, 0x3c, 0x0c,  // 0x1e
	0x00, 0xc0, 0xf0, 0xfc, 0xf0, 0xc0,  // 0x1f
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x20
	0x00, 0x00, 0x60, 0xfa, 0x60, 0x00,  // 0x21
	0x00, 0xe0, 0xc0, 0x00, 0xe0, 0xc0,  // 0x22
	0x00, 0x24, 0x7e, 0x24, 0x7e, 0x24,  // 0x23
	0x00, 0x24, 0xd4, 0x56, 0x48, 0x00,  // 0x24
	0x00, 0xc6, 0xc8, 0x10, 0x26, 0xc6,  // 0x25
	0x00, 0x6c, 0x92, 0x6a, 0x04, 0x0a,  // 0x26
	0x00, 0x00, 0xe0, 0xc0, 0x00, 0x00,  // 0x27
	0x00, 0x00, 0x7c, 0x82, 0x00, 0x00,  // 0x28
	0x00, 0x00, 0x82, 0x7c, 0x00, 0x00,  // 0x29
	0x00, 0x10, 0x7c, 0x38, 0x7c, 0x10,  // 0x2a
	0x00, 0x10, 0x10, 0x7c, 0x10, 0x10,  // 0x2b
	0x00, 0x00, 0x07, 0x06, 0x00, 0x00,  // 0x2c
	0x00, 0x10, 0x10, 0x10, 0x10, 0x10,  // 0x2d
	0x00, 0x00, 0x06, 0x06, 0x00, 0x00,  // 0x2e
	0x00, 0x04, 0x08, 0x10, 0x20, 0x40,  // 0x2f
	0x00, 0x7c, 0x8a, 0x92, 0xa2, 0x7c,  // 0x30
	0x00, 0x00, 0x42, 0xfe, 0x02, 0x00,  // 0x31
	0x00, 0x46, 0x8a, 0x92, 0x92, 0x62,  // 0x32
	0x00, 0x44, 0x92, 0x92, 0x92, 0x6c,  // 0x33
	0x00, 0x18, 0x28, 0x48, 0xfe, 0x08,  // 0x34
	0x00, 0xf4, 0x92, 0x92, 0x92, 0x8c,  // 0x35
	0x00, 0x3c, 0x52, 0x92, 0x92, 0x0c,  // 0x36
	0x00, 0x80, 0x8e, 0x90, 0xa0, 0xc0,  // 0x37
	0x00, 0x6c, 0x92, 0x92, 0x92, 0x6c,  // 0x38
	0x00, 0x60, 0x92, 0x92, 0x94, 0x78,  // 0x39
	0x00, 0x00, 0x36, 0x36, 0x00, 0x00,  // 0x3a
	0x00, 0x00, 0x37, 0x36, 0x00, 0x00,  // 0x3b
	0x00, 0x10, 0x28, 0x44, 0x82, 0x00,  // 0x3c
	0x00, 0x24, 0x24, 0x24, 0x24, 0x24,  // 0x3d
	0x00, 0x00, 0x82, 0x44, 0x28, 0x10,  // 0x3e
	0x00, 0x40, 0x80, 0x9a, 0x90, 0x60,  // 0x3f
	0x00, 0x7c, 0x82, 0xba, 0xaa, 0x78,  // 0x40
	0x00, 0x7e, 0x88, 0x88, 0x88, 0x7e,  // 0x41
	0x00, 0xfe, 0x92, 0x92, 0x92, 0x6c,  // 0x42
	0x00, 0x7c, 0x82, 0x82, 0x82, 0x44,  // 0x43
	0x00, 0xfe, 0x82, 0x82, 0x82, 0x7c,  // 0x44
	0x00, 0xfe, 0x92, 0x92, 0x92, 0x82,  // 0x45
	0x00, 0xfe, 0x90, 0x90, 0x90, 0x80,  // 0x46
	0x00, 0x7c, 0x82, 0x92, 0x92, 0x5e,  // 0x47
	0x00, 0xfe, 0x10, 0x10, 0x10, 0xfe,  // 0x48
	0x00, 0x00, 0x82, 0xfe, 0x82, 0x00,  // 0x49
	0x00, 0x0c, 0x02, 0x02, 0x02, 0xfc,  // 0x4a
	0x00, 0xfe, 0x10, 0x28, 0x44, 0x82,  // 0x4b
	0x00, 0xfe, 0x02, 0x02, 0x02, 0x02,  // 0x4c
	0x00, 0xfe, 0x40, 0x20, 0x40, 0xfe,  // 0x4d
	0x00, 0xfe, 0x40, 0x20, 0x10, 0xfe,  // 0x4e
	0x00, 0x7c, 0x82, 0x82, 0x82, 0x7c,  // 0x4f
	0x00, 0xfe, 0x90, 0x90, 0x90, 0x60,  // 0x50
	0x00, 0x7c, 0x82, 0x8a, 0x84, 0x7a,  // 0x51
	0x00, 0xfe, 0x90, 0x90, 0x98, 0x66,  // 0x52
	0x00, 0x64, 0x92, 0x92, 0x92, 0x4c,  // 0x53
	0x00, 0x80, 0x80, 0xfe, 0x80, 0x80,  // 0x54
	0x00, 0xfc, 0x02, 0x02, 0x02, 0xfc,  // 0x55
	0x00, 0xf8, 0x04, 0x02, 0x04, 0xf8,  // 0x56
	0x00, 0xfc, 0x02, 0x3c, 0x02, 0xfc,  // 0x57
	0x00, 0xc6, 0x28, 0x10, 0x28, 0xc6,  // 0x58
	0x00, 0xe0, 0x10, 0x0e, 0x10, 0xe0,  // 0x59
	0x00, 0x8e, 0x92, 0xa2, 0xc2, 0x00,  // 0x5a
	0x00, 0x00, 0xfe, 0x82, 0x82, 0x00,  // 0x5b
	0x00, 0x40, 0x20, 0x10, 0x08, 0x04,  // 0x5c
	0x00, 0x00, 0x82, 0x82, 0xfe, 0x00,  // 0x5d
	0x00, 0x20, 0x40, 0x80, 0x40, 0x20,  // 0x5e
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01,  // 0x5f
	0x00, 0x00, 0xc0, 0xe0, 0x00, 0x00,  // 0x60
	0x00, 0x04, 0x2a, 0x2a, 0x2a, 0x1e,  // 0x61
	0x00, 0xfe, 0x22, 0x22, 0x22, 0x1c,  // 0x62
	0x00, 0x1c, 0x22, 0x22, 0x22, 0x04,  // 0x63 'c' = 0x00, 0x1c, 0x22, 0x22, 0x22, 0x14
	0x00, 0x1c, 0x22, 0x22, 0x22, 0xfe,  // 0x64
	0x00, 0x1c, 0x2a, 0x2a, 0x2a, 0x10,  // 0x65
	0x00, 0x10, 0x7e, 0x90, 0x90, 0x00,  // 0x66
	0x00, 0x18, 0x25, 0x25, 0x25, 0x3e,  // 0x67
	0x00, 0xfe, 0x20, 0x20, 0x1e, 0x00,  // 0x68
	0x00, 0x00, 0x00, 0x5e, 0x02, 0x00,  // 0x69 'i' = 0x00, 0x00, 0x00, 0xbe, 0x02, 0x00
	0x00, 0x02, 0x01, 0x21, 0xbe, 0x00,  // 0x6a
	0x00, 0xfe, 0x08, 0x14, 0x22, 0x00,  // 0x6b
	0x00, 0x00, 0x00, 0xfe, 0x02, 0x00,  // 0x6c
	0x00, 0x3e, 0x20, 0x18, 0x20, 0x1e,  // 0x6d
	0x00, 0x3e, 0x20, 0x20, 0x1e, 0x00,  // 0x6e
	0x00, 0x1c, 0x22, 0x22, 0x22, 0x1c,  // 0x6f
	0x00, 0x3f, 0x22, 0x22, 0x22, 0x1c,  // 0x70
	0x00, 0x1c, 0x22, 0x22, 0x22, 0x3f,  // 0x71
	0x00, 0x22, 0x1e, 0x22, 0x20, 0x10,  // 0x72
	0x00, 0x10, 0x2a, 0x2a, 0x2a, 0x04,  // 0x73
	0x00, 0x20, 0x7c, 0x22, 0x24, 0x00,  // 0x74
	0x00, 0x3c, 0x02, 0x04, 0x3e, 0x00,  // 0x75
	0x00, 0x38, 0x04, 0x02, 0x04, 0x38,  // 0x76
	0x00, 0x3c, 0x06, 0x0c, 0x06, 0x3c,  // 0x77
	0x00, 0x36, 0x08, 0x08, 0x36, 0x00,  // 0x78
	0x00, 0x39, 0x05, 0x06, 0x3c, 0x00,  // 0x79
	0x00, 0x26, 0x2a, 0x2a, 0x32, 0x00,  // 0x7a
	0x00, 0x10, 0x7c, 0x82, 0x82, 0x00,  // 0x7b
	0x00, 0x00, 0x00, 0xee, 0x00, 0x00,  // 0x7c
	0x00, 0x00, 0x82, 0x82, 0x7c, 0x10,  // 0x7d
	0x00, 0x40, 0x80, 0x40, 0x80, 0x00,  // 0x7e
	0x00, 0x3c, 0x64, 0xc4, 0x64, 0x3c,  // 0x7f
	0x00, 0x78, 0x85, 0x87, 0x84, 0x48,  // 0x80
	0x00, 0xbc, 0x02, 0x04, 0xbe, 0x00,  // 0x81
	0x00, 0x1c, 0x2a, 0x2a, 0xaa, 0x90,  // 0x82
	0x00, 0x04, 0xaa, 0xaa, 0xaa, 0x1e,  // 0x83
	0x00, 0x04, 0xaa, 0x2a, 0xaa, 0x1e,  // 0x84
	0x00, 0x04, 0xaa, 0xaa, 0x2a, 0x1e,  // 0x85
	0x00, 0x04, 0xea, 0xaa, 0xea, 0x1e,  // 0x86
	0x00, 0x38, 0x45, 0x47, 0x44, 0x28,  // 0x87
	0x00, 0x1c, 0xaa, 0xaa, 0xaa, 0x10,  // 0x88
	0x00, 0x1c, 0xaa, 0x2a, 0xaa, 0x10,  // 0x89
	0x00, 0x1c, 0xaa, 0xaa, 0x2a, 0x10,  // 0x8a
	0x00, 0x00, 0x80, 0x3e, 0x82, 0x00,  // 0x8b
	0x00, 0x00, 0x80, 0xbe, 0x82, 0x00,  // 0x8c
	0x00, 0x00, 0x80, 0x3e, 0x02, 0x00,  // 0x8d
	0x00, 0x0e, 0x94, 0x24, 0x94, 0x0e,  // 0x8e
	0x00, 0x1e, 0xf4, 0xa4, 0xf4, 0x1e,  // 0x8f
	0x00, 0x3e, 0x2a, 0x2a, 0xaa, 0xa2,  // 0x90
	0x00, 0x2c, 0x2a, 0x3e, 0x2a, 0x1a,  // 0x91
	0x00, 0x7e, 0x90, 0xfe, 0x92, 0x92,  // 0x92
	0x00, 0x1c, 0xa2, 0xa2, 0x9c, 0x00,  // 0x93
	0x00, 0x1c, 0xa2, 0x22, 0x9c, 0x00,  // 0x94
	0x00, 0x9c, 0xa2, 0x22, 0x1c, 0x00,  // 0x95
	0x00, 0x3c, 0x82, 0x84, 0xbe, 0x00,  // 0x96
	0x00, 0xbc, 0x82, 0x04, 0x3e, 0x00,  // 0x97
	0x00, 0x39, 0x85, 0x06, 0xbc, 0x00,  // 0x98
	0x00, 0xbc, 0x42, 0x42, 0xbc, 0x00,  // 0x99
	0x00, 0x3c, 0x82, 0x02, 0xbc, 0x00,  // 0x9a
	0x01, 0x0e, 0x16, 0x1a, 0x1c, 0x20,  // 0x9b
	0x00, 0x12, 0x7c, 0x92, 0x92, 0x46,  // 0x9c
	0x00, 0x7e, 0x86, 0xba, 0xc2, 0xfc,  // 0x9d
	0x00, 0x44, 0x28, 0x10, 0x28, 0x44,  // 0x9e
	0x00, 0x02, 0x11, 0x7e, 0x90, 0x40,  // 0x9f
	0x00, 0x04, 0x2a, 0xaa, 0xaa, 0x1e,  // 0xa0
	0x00, 0x00, 0x00, 0xbe, 0x82, 0x00,  // 0xa1
	0x00, 0x1c, 0x22, 0xa2, 0x9c, 0x00,  // 0xa2
	0x00, 0x3c, 0x02, 0x84, 0xbe, 0x00,  // 0xa3
	0x00, 0x5e, 0x90, 0x50, 0x8e, 0x00,  // 0xa4
	0x00, 0x5e, 0x88, 0x44, 0x9e, 0x00,  // 0xa5
	0x00, 0x10, 0xaa, 0xaa, 0xaa, 0x7a,  // 0xa6
	0x00, 0x72, 0x8a, 0x8a, 0x72, 0x00,  // 0xa7
	0x00, 0x0c, 0x12, 0xb2, 0x02, 0x04,  // 0xa8
	0x7c, 0x82, 0xba, 0xd2, 0xaa, 0x7c,  // 0xa9
	0x20, 0x20, 0x20, 0x20, 0x20, 0x38,  // 0xaa
	0x00, 0xe8, 0x10, 0x32, 0x56, 0x0a,  // 0xab
	0x00, 0xe8, 0x10, 0x2c, 0x54, 0x1e,  // 0xac
	0x00, 0x00, 0x0c, 0xbe, 0x0c, 0x00,  // 0xad
	0x00, 0x10, 0x28, 0x00, 0x10, 0x28,  // 0xae
	0x00, 0x28, 0x10, 0x00, 0x28, 0x10,  // 0xaf
	0x22, 0x88, 0x22, 0x88, 0x22, 0x88,  // 0xb0
	0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa,  // 0xb1
	0xdd, 0x77, 0xdd, 0x77, 0xdd, 0x77,  // 0xb2
	0x00, 0x00, 0x00, 0xff, 0x00, 0x00,  // 0xb3
	0x10, 0x10, 0x10, 0xff, 0x00, 0x00,  // 0xb4
	0x00, 0x0e, 0x14, 0xa4, 0x94, 0x0e,  // 0xb5
	0x00, 0x0e, 0x94, 0xa4, 0x94, 0x0e,  // 0xb6
	0x00, 0x0e, 0x94, 0xa4, 0x14, 0x0e,  // 0xb7
	0x7c, 0x82, 0xba, 0xaa, 0x82, 0x7c,  // 0xb8
	0x50, 0xdf, 0x00, 0xff, 0x00, 0x00,  // 0xb9
	0x00, 0xff, 0x00, 0xff, 0x00, 0x00,  // 0xba
	0x50, 0x5f, 0x40, 0x7f, 0x00, 0x00,  // 0xbb
	0x50, 0xd0, 0x10, 0xf0, 0x00, 0x00,  // 0xbc
	0x00, 0x18, 0x24, 0x66, 0x24, 0x00,  // 0xbd
	0x00, 0x94, 0x54, 0x3e, 0x54, 0x94,  // 0xbe
	0x10, 0x10, 0x10, 0x1f, 0x00, 0x00,  // 0xbf
	0x00, 0x00, 0x00, 0xf0, 0x10, 0x10,  // 0xc0
	0x10, 0x10, 0x10, 0xf0, 0x10, 0x10,  // 0xc1
	0x10, 0x10, 0x10, 0x1f, 0x10, 0x10,  // 0xc2
	0x00, 0x00, 0x00, 0xff, 0x10, 0x10,  // 0xc3
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10,  // 0xc4
	0x10, 0x10, 0x10, 0xff, 0x10, 0x10,  // 0xc5
	0x00, 0x04, 0x6a, 0xaa, 0x6a, 0x9e,  // 0xc6
	0x00, 0x0e, 0x54, 0xa4, 0x54, 0x8e,  // 0xc7
	0x00, 0xf0, 0x10, 0xd0, 0x50, 0x50,  // 0xc8
	0x00, 0x7f, 0x40, 0x5f, 0x50, 0x50,  // 0xc9
	0x50, 0xd0, 0x10, 0xd0, 0x50, 0x50,  // 0xca
	0x50, 0x5f, 0x40, 0x5f, 0x50, 0x50,  // 0xcb
	0x00, 0xff, 0x00, 0xdf, 0x50, 0x50,  // 0xcc
	0x50, 0x50, 0x50, 0x50, 0x50, 0x50,  // 0xcd
	0x50, 0xdf, 0x00, 0xdf, 0x50, 0x50,  // 0xce
	0x00, 0xba, 0x44, 0x44, 0x44, 0xba,  // 0xcf
	0x00, 0x44, 0xaa, 0x9a, 0x0c, 0x00,  // 0xd0
	0x00, 0x10, 0xfe, 0x92, 0x82, 0x7c,  // 0xd1
	0x00, 0x3e, 0xaa, 0xaa, 0xaa, 0x22,  // 0xd2
	0x00, 0x3e, 0xaa, 0x2a, 0xaa, 0x22,  // 0xd3
	0x00, 0x3e, 0xaa, 0xaa, 0x2a, 0x22,  // 0xd4
	0x00, 0x00, 0x00, 0xe0, 0x00, 0x00,  // 0xd5
	0x00, 0x00, 0x22, 0xbe, 0xa2, 0x00,  // 0xd6
	0x00, 0x00, 0xa2, 0xbe, 0xa2, 0x00,  // 0xd7
	0x00, 0x00, 0xa2, 0x3e, 0xa2, 0x00,  // 0xd8
	0x10, 0x10, 0x10, 0xf0, 0x00, 0x00,  // 0xd9
	0x00, 0x00, 0x00, 0x1f, 0x10, 0x10,  // 0xda
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // 0xdb
	0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,  // 0xdc
	0x00, 0x00, 0x00, 0xee, 0x00, 0x00,  // 0xdd
	0x00, 0x00, 0xa2, 0xbe, 0x22, 0x00,  // 0xde
	0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0,  // 0xdf
	0x00, 0x3c, 0x42, 0xc2, 0xbc, 0x00,  // 0xe0
	0x00, 0x7f, 0x52, 0x52, 0x2c, 0x00,  // 0xe1
	0x00, 0x3c, 0xc2, 0xc2, 0xbc, 0x00,  // 0xe2
	0x00, 0xbc, 0xc2, 0x42, 0x3c, 0x00,  // 0xe3
	0x00, 0x4c, 0x92, 0x52, 0x8c, 0x00,  // 0xe4
	0x00, 0x5c, 0xa2, 0x62, 0x9c, 0x00,  // 0xe5
	0x00, 0x3f, 0x04, 0x04, 0x38, 0x00,  // 0xe6
	0x00, 0x7f, 0x55, 0x14, 0x08, 0x00,  // 0xe7
	0x00, 0xff, 0xa5, 0x24, 0x18, 0x00,  // 0xe8
	0x00, 0x3c, 0x02, 0x82, 0xbc, 0x00,  // 0xe9
	0x00, 0x3c, 0x82, 0x82, 0xbc, 0x00,  // 0xea
	0x00, 0xbc, 0x82, 0x02, 0x3c, 0x00,  // 0xeb
	0x00, 0x39, 0x05, 0x86, 0xbc, 0x00,  // 0xec
	0x00, 0x20, 0x10, 0x8e, 0x90, 0x20,  // 0xed
	0x00, 0x00, 0x40, 0x40, 0x40, 0x00,  // 0xee
	0x00, 0x00, 0xe0, 0xc0, 0x00, 0x00,  // 0xef
	0x00, 0x00, 0x10, 0x10, 0x10, 0x00,  // 0xf0
	0x00, 0x00, 0x24, 0x74, 0x24, 0x00,  // 0xf1
	0x00, 0x24, 0x24, 0x24, 0x24, 0x24,  // 0xf2
	0xa0, 0xe8, 0x50, 0x2c, 0x54, 0x1e,  // 0xf3
	0x00, 0x60, 0x90, 0xfe, 0x80, 0xfe,  // 0xf4
	0x00, 0x44, 0xb2, 0xaa, 0x9a, 0x44,  // 0xf5
	0x00, 0x10, 0x10, 0x54, 0x10, 0x10,  // 0xf6
	0x00, 0x00, 0x10, 0x18, 0x18, 0x00,  // 0xf7
	0x00, 0x60, 0x90, 0x90, 0x60, 0x00,  // 0xf8
	0x00, 0x00, 0x10, 0x00, 0x10, 0x00,  // 0xf9
	0x00, 0x00, 0x10, 0x00, 0x00, 0x00,  // 0xfa
	0x00, 0x40, 0xf0, 0x00, 0x00, 0x00,  // 0xfb
	0x00, 0x90, 0xf0, 0xa0, 0x00, 0x00,  // 0xfc
	0x00, 0x90, 0xb0, 0x50, 0x00, 0x00,  // 0xfd
	0x00, 0x3c, 0x3c, 0x3c, 0x3c, 0x00,  // 0xfe
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // 0xff
};

void shiftOut(uint8_t data) {
	HAL_GPIO_WritePin(DOTMATRIX_C_LD_GPIO_Port, DOTMATRIX_C_LD_Pin, 0); // Set CS to LOW
  for (int ibit=7;ibit>=0;ibit--) {
    HAL_GPIO_WritePin(DOTMATRIX_C_DT_GPIO_Port, DOTMATRIX_C_DT_Pin, (data>>ibit)&0x01);
    
		HAL_GPIO_WritePin(DOTMATRIX_C_CLK_GPIO_Port, DOTMATRIX_C_CLK_Pin, 1);
		HAL_GPIO_WritePin(DOTMATRIX_C_CLK_GPIO_Port, DOTMATRIX_C_CLK_Pin, 0);
  }
  HAL_GPIO_WritePin(DOTMATRIX_C_LD_GPIO_Port, DOTMATRIX_C_LD_Pin, 1); // Set CS to HIGH
}

void resetCounter() {
	HAL_GPIO_WritePin(DOTMATRIX_R_RST_GPIO_Port, DOTMATRIX_R_RST_Pin, 1);
	HAL_GPIO_WritePin(DOTMATRIX_R_RST_GPIO_Port, DOTMATRIX_R_RST_Pin, 0);
}

void tickClock() {
	HAL_GPIO_WritePin(DOTMATRIX_R_CLK_GPIO_Port, DOTMATRIX_R_CLK_Pin, 1);
	HAL_GPIO_WritePin(DOTMATRIX_R_CLK_GPIO_Port, DOTMATRIX_R_CLK_Pin, 0);
}

void scan_dotmatrix(uint8_t *data) {
  for (int i=0;i<8;i++) {
    shiftOut(~data[i]);
    tickClock();
    HAL_Delay(1);
  }
  resetCounter();
}

uint8_t scan_bit = 0;
void scan_dotmatrix2(uint8_t *data) {
  if (scan_bit == 0) {
    resetCounter();
  }
  shiftOut(~data[scan_bit]);
  tickClock();
  scan_bit = scan_bit < 8 ? scan_bit + 1 : 0;
}

void rotate(uint8_t *oldBuff, uint8_t *newBuff) {
  memset(newBuff, 0, 8);
  for(uint8_t a=0;a<8;a++) {
    for(uint8_t i=0;i<8;i++) {
      newBuff[a] |= (oldBuff[i]&(0x80>>a)) ? (1<<i) : 0;
    }
  }
}

uint8_t frame_buffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
void rotateToBuffer(uint8_t *c) {
  uint8_t newBuff[8];
  rotate(c, newBuff);
  memcpy(frame_buffer, newBuff, 8);
}

uint8_t buffText[(30 * 6) + (8 * 2)]; // Max 30 chars, 2 space for start and end
void showText(char *text) {
  uint8_t inx = 8;
  for(uint8_t cInx=0;cInx<strlen(text);cInx++) {
    for(uint8_t i=0;i<6;i++) {
      buffText[inx++] = font_6_8[(text[cInx] * 6) + i];
    }
  }
  
  for(uint8_t start=0;start<((strlen(text) * 6) + (8));start++) {
    uint8_t buff[8];
    memcpy(buff, &buffText[start], 8);
    rotateToBuffer(buff);
    HAL_Delay(100);
  }
}

void showTextAsync(char *text) {
  static uint32_t timer = 0;
  static uint8_t state = 0;
  
  if (state == 0) {
    uint8_t inx = 8;
    for(uint8_t cInx=0;cInx<strlen(text);cInx++) {
      for(uint8_t i=0;i<6;i++) {
        buffText[inx++] = font_6_8[(text[cInx] * 6) + i];
      }
    }
    timer = HAL_GetTick();
    state = 1;
  } else if (state == 1) {
    if ((HAL_GetTick() - timer) >= 100) {
      timer = HAL_GetTick();
      static uint8_t start=0;
      if (start<((strlen(text) * 6) + (8))) {
        uint8_t buff[8];
        memcpy(buff, &buffText[start], 8);
        rotateToBuffer(buff);
        
        start++;
      } else {
        start = 0;
        state = 0;
      }
    }
  }
}

void showTextRTOS(char *text) {
  uint8_t inx = 8;
  for(uint8_t cInx=0;cInx<strlen(text);cInx++) {
    for(uint8_t i=0;i<6;i++) {
      buffText[inx++] = font_6_8[(text[cInx] * 6) + i];
    }
  }
  
  for(uint8_t start=0;start<((strlen(text) * 6) + (8));start++) {
    uint8_t buff[8];
    memcpy(buff, &buffText[start], 8);
    rotateToBuffer(buff);
    osDelay(100);
  }
}

void showChar(char c) {
  uint8_t buff[8];
  memset(buff, 0, 8);
  memcpy(&buff[2], &font_6_8[c * 6], 6);
  rotateToBuffer(buff);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  scan_dotmatrix2(frame_buffer);
}
// ============== END of Dot Matrix ==============

// ============== Encoder ==============
int8_t counter = 0;
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == ENCODER_A_Pin) {
		if (HAL_GPIO_ReadPin(ENCODER_B_GPIO_Port, ENCODER_B_Pin) == 0) {
			counter++;
		} else {
			counter--;
		}
    
    if (counter == 0) {
      HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, 0);
      HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 1);
      HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, 0);
    } else if (counter <= -1) {
      HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, 1);
      HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 0);
      HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, 0);
      counter = -1;
    } else if (counter >= 1) {
      HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, 0);
      HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, 0);
      HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, 1);
      counter = 1;
    }
	}
}
// ============== END of Encoder ==============

uint16_t accelerometer_value[3];

uint8_t state = 0;
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
  MX_DMA_Init();
  MX_ADC_Init();
  MX_I2C2_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  lcd_init();
  
  // Setup 74HC595
	HAL_GPIO_WritePin(DOTMATRIX_C_LD_GPIO_Port, DOTMATRIX_C_LD_Pin, 1); // Set CS to HIGH
	HAL_GPIO_WritePin(DOTMATRIX_C_DT_GPIO_Port, DOTMATRIX_C_DT_Pin, 0);
	HAL_GPIO_WritePin(DOTMATRIX_C_CLK_GPIO_Port, DOTMATRIX_C_CLK_Pin, 0);
	
	// Setup 4017
	HAL_GPIO_WritePin(DOTMATRIX_R_RST_GPIO_Port, DOTMATRIX_R_RST_Pin, 0);
	HAL_GPIO_WritePin(DOTMATRIX_R_CLK_GPIO_Port, DOTMATRIX_R_CLK_Pin, 0);
	
	resetCounter();
	shiftOut(0x00); // trun off all
  
  HAL_TIM_Base_Start_IT(&htim2);
  
  // Setup ADC
	HAL_ADC_Start_DMA(&hadc, (uint32_t*)accelerometer_value, 3);
  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 64);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of DotMatrix */
  osThreadDef(DotMatrix, DotMatrixTask, osPriorityIdle, 0, 64);
  DotMatrixHandle = osThreadCreate(osThread(DotMatrix), NULL);

  /* definition and creation of LCD */
  osThreadDef(LCD, LCDTask, osPriorityIdle, 0, 64);
  LCDHandle = osThreadCreate(osThread(LCD), NULL);

  /* definition and creation of Sound */
  osThreadDef(Sound, SoundTask, osPriorityIdle, 0, 64);
  SoundHandle = osThreadCreate(osThread(Sound), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    osDelay(10);
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
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
  */
  hadc.Instance = ADC1;
  hadc.Init.OversamplingMode = DISABLE;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.SamplingTime = ADC_SAMPLETIME_160CYCLES_5;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ContinuousConvMode = ENABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.DMAContinuousRequests = ENABLE;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerFrequencyMode = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel to be converted. 
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel to be converted. 
  */
  sConfig.Channel = ADC_CHANNEL_1;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel to be converted. 
  */
  sConfig.Channel = ADC_CHANNEL_4;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

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
  hi2c2.Init.Timing = 0x00707CBB;
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
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 32;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

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
  HAL_GPIO_WritePin(GPIOC, DOTMATRIX_R_RST_Pin|DOTMATRIX_R_CLK_Pin|DOTMATRIX_C_DT_Pin|DOTMATRIX_C_CLK_Pin 
                          |DOTMATRIX_C_LD_Pin|LED_RED_Pin|LED_GREEN_Pin|LED_BLUE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

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

  /*Configure GPIO pins : DOTMATRIX_R_RST_Pin DOTMATRIX_R_CLK_Pin DOTMATRIX_C_DT_Pin DOTMATRIX_C_CLK_Pin 
                           DOTMATRIX_C_LD_Pin LED_RED_Pin LED_GREEN_Pin LED_BLUE_Pin */
  GPIO_InitStruct.Pin = DOTMATRIX_R_RST_Pin|DOTMATRIX_R_CLK_Pin|DOTMATRIX_C_DT_Pin|DOTMATRIX_C_CLK_Pin 
                          |DOTMATRIX_C_LD_Pin|LED_RED_Pin|LED_GREEN_Pin|LED_BLUE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : BUZZER_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN1_Pin BTN2_Pin BTN3_Pin BTN4_Pin */
  GPIO_InitStruct.Pin = BTN1_Pin|BTN2_Pin|BTN3_Pin|BTN4_Pin;
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
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{

  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */ 
}

/* USER CODE BEGIN Header_DotMatrixTask */
/**
* @brief Function implementing the DotMatrix thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_DotMatrixTask */
void DotMatrixTask(void const * argument)
{
  /* USER CODE BEGIN DotMatrixTask */
  /* Infinite loop */
  for(;;)
  {
    showTextRTOS("E-TECH");
  }
  /* USER CODE END DotMatrixTask */
}

/* USER CODE BEGIN Header_LCDTask */
/**
* @brief Function implementing the LCD thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LCDTask */
void LCDTask(void const * argument)
{
  /* USER CODE BEGIN LCDTask */
  /* Infinite loop */
  for(;;)
  {
    if (HAL_GPIO_ReadPin(BTN4_GPIO_Port, BTN4_Pin) == 0) {
      while(HAL_GPIO_ReadPin(BTN4_GPIO_Port, BTN4_Pin) == 0) osDelay(20);
      
      if (state == 0) {
        state = 1;
      } else if (state == 1) {
        state = 0;
      }
      lcd_clear();
    }
    
    char buff[16];

    if (state == 0) {
      uint8_t h, m, s, dd, mm, yy;
      
      read_time(&h, &m, &s, &dd, &mm, &yy);
      
      sprintf(buff, "Date: %d/%d/%d      ", dd, mm, yy + 1900);
      lcd_set_cursor(1, 0);
      lcd_print(buff);
      
      sprintf(buff, "Time: %02d:%02d:%02d", h, m, s);
      lcd_set_cursor(2, 0);
      lcd_print(buff);
    } else if (state == 1) {
      sprintf(buff, "X: %d  Y: %d    ", accelerometer_value[0], accelerometer_value[1]);
		  lcd_set_cursor(1, 0);
		  lcd_print(buff);

		  sprintf(buff, "Z: %d    ", accelerometer_value[2]);
		  lcd_set_cursor(2, 0);
		  lcd_print(buff);
    }
    
    osDelay(20);
  }
  /* USER CODE END LCDTask */
}

/* USER CODE BEGIN Header_SoundTask */
/**
* @brief Function implementing the Sound thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_SoundTask */
void SoundTask(void const * argument)
{
  /* USER CODE BEGIN SoundTask */
  /* Infinite loop */
  for(;;)
  {
    // A
    if (HAL_GPIO_ReadPin(BTN1_GPIO_Port, BTN1_Pin) == 0) {
      while(HAL_GPIO_ReadPin(BTN1_GPIO_Port, BTN1_Pin) == 0) osDelay(10);
      
      for(int x=0;x<100;x++) {
        HAL_GPIO_TogglePin(BUZZER_GPIO_Port, BUZZER_Pin);
        osDelay(1);
      }
      HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, 0);
    }
    
    // B
    if (HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin) == 0) {
      while(HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin) == 0) osDelay(10);
      
      for(int x=0;x<300;x++) {
        HAL_GPIO_TogglePin(BUZZER_GPIO_Port, BUZZER_Pin);
        osDelay(1);
      }
      HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, 0);
    }
    
    // C
    if (HAL_GPIO_ReadPin(BTN3_GPIO_Port, BTN3_Pin) == 0) {
      while(HAL_GPIO_ReadPin(BTN3_GPIO_Port, BTN3_Pin) == 0) osDelay(10);
      
      for(int x=0;x<600;x++) {
        HAL_GPIO_TogglePin(BUZZER_GPIO_Port, BUZZER_Pin);
        osDelay(1);
      }
      HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, 0);
    }
    
    osDelay(1);
  }
  /* USER CODE END SoundTask */
}

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
