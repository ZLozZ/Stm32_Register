/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
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
void spi_active_slave(){
	uint32_t* GPIOE_ODR = (uint32_t*)(0x40021014);
	//set LOW for PE3
	*GPIOE_ODR &= ~(1<<3);
}

void spi_inactive_slave()
{
	uint32_t* GPIOE_ODR = (uint32_t*)(0x40021014);
	//set HIGH for PE3
	*GPIOE_ODR |= (1<<3);
}

#define WHO_AM_I_REG 0x0f
#define CTRL1_REG 0x20

void spi_init(){
	__HAL_RCC_GPIOA_CLK_ENABLE();
	//set PA5(alternate function - SPI1_SCK), PA6(alternate function-SPI1_MISO), PA7(alternate-MOSI)
	uint32_t* GPIOA_MODER = (uint32_t*)(0x40020000);
	uint32_t* GPIOA_AFRL = (uint32_t*)(0x40020020);
	*GPIOA_MODER |= (0b10 << 10) | (0b10 << 12) | (0b10 << 14);
	*GPIOA_AFRL |= (5 << 20) | (5 << 24) | (5 << 28);
	//set PE3 in OUTPUT mode
	__HAL_RCC_GPIOE_CLK_ENABLE();
	uint32_t* GPIOE_MODER = (uint32_t*)(0x40021000);
	*GPIOE_MODER |= (0b01 << 6);
	//init spi1 in master
	__HAL_RCC_SPI1_CLK_ENABLE(); // --> enable 16Mhz clock spi
	uint32_t* SPI_CR1 = (uint32_t*)(0x40013000);
	*SPI_CR1 |= (1<<2); //set master mode for spi1
	*SPI_CR1 |= (0b100 << 3);//set baudrate 500kHz(16Mhz/32);
	*SPI_CR1 |= (1<<8) | (1<<9); //ss pin is controlled by GPIO without SPI1
	*SPI_CR1 |= 1<<6; //enable SPI1
	spi_inactive_slave();
}

char spi_read_data(char reg){
	uint32_t* DR = (uint32_t*)(0x4001300c);
	uint32_t* SR = (uint32_t*)(0x40013008);
	//active slave - set PE3 to low
	spi_active_slave();
	//send reg to slave - write reg value to DR of SPI1
	while(((*SR >> 1) & 1) != 1); //wait TX empty to write data DR
	*DR = reg;
	while(((*SR >> 1) & 1) == 1);//wait data be transfered to Tx buffer
	while(((*SR >> 0) & 1) != 1); //wait RX not empty (has recv data) to read data
	while(((*SR >> 7) & 1) == 1); //wait not busy

	//clear spam data - read data from DR
	uint8_t temp = *DR;
	//send clock for slave to slave send data to master. write dummy data(0x00 or 0xFF)
	while(((*SR >> 1) & 1) != 1); //wait TX empty to write data DR
	*DR = 0xff;
	while(((*SR >> 1) & 1) == 1);//wait data be transfered to Tx buffer
	while(((*SR >> 0) & 1) != 1); //wait RX not empty (has recv data) to read data
	while(((*SR >> 7) & 1) == 1); //wait not busy

	//read data from DR
	temp = *DR;

	//inactive slave - set PE3 to HIGH
	spi_inactive_slave();
	return temp;
}

void spi_write_data(char reg, char data)
{
	uint32_t* DR = (uint32_t*)(0x4001300c);
	uint32_t* SR = (uint32_t*)(0x40013008);
	//active slave - set PE3 to LOW
	spi_active_slave();
	//send reg to slave - write reg value to DR of SPI1
	while(((*SR >> 1) & 1) != 1); //wait TX empty to write data DR
	*DR = reg;
	while(((*SR >> 1) & 1) == 1);//wait data be transfered to Tx buffer
	while(((*SR >> 0) & 1) != 1); //wait RX not empty (has recv data) to read data
	while(((*SR >> 7) & 1) == 1); //wait not busy
	//clear spam data - read data from DR
	uint8_t temp = *DR;

	//send clock for slave to slave send data to master. write dummy data(0x00 or 0xFF)
	while(((*SR >> 1) & 1) != 1); //wait TX empty to write data DR
	*DR = data;
	while(((*SR >> 1) & 1) == 1);//wait data be transfered to Tx buffer
	while(((*SR >> 0) & 1) != 1); //wait RX not empty (has recv data) to read data
	while(((*SR >> 7) & 1) == 1); //wait not busy

	//read data from DR
	temp = *DR;

	//inactive slave - set PE3 to HIGH
	spi_inactive_slave();
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
  spi_init();
  uint8_t gyro_id = spi_read_data(WHO_AM_I_REG);
  gyro_id = spi_read_data(CTRL1_REG);
  spi_write_data(CTRL1_REG, 0b00001111);
  gyro_id = spi_read_data(CTRL1_REG);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
