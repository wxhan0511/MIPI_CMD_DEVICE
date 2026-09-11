/**
 * @file       main.c
 * @brief      Main program body
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-12-31
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

/* ==================== 1. Header includes ==================== */
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>

#include "gpio.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "crc.h"
#include "fsmc.h"
#include "dac.h"

#include "bsp.h"

/* ==================== 2. Macro definitions ==================== */
/* None */

/* ==================== 3. Type definitions (structs, enums, aliases) ==================== */
/* None */

/* ==================== 4. External global variables ==================== */
/* None */

/* ==================== 5. Static private variables ==================== */
/* None */

/* ==================== 6. Static function declarations ==================== */
/* None */

/* ==================== External function declarations ==================== */
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);

/* ==================== 7. Public function implementations ==================== */
int main(void)
{
  /* HAL base initialization */
  HAL_Init();

  /* System clock configuration */
  SystemClock_Config();

  /* Peripheral initialization */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init(); /* PB6 PB7 */
  MX_I2C2_Init(); /* PB10 PB11 */
  MX_CRC_Init();
  MX_FSMC_Init();
  MX_DAC_Init();
  MX_SPI1_Init(); /* ADS1256 */
  // MX_SPI2_Init();
  MX_SPI3_Init(); /* FLASH */
  MX_USART3_UART_Init();

  /* Board-level initialization */
  bsp_init();

  /* RTOS initialization and start */
  osKernelInitialize();
  MX_FREERTOS_Init();
  osKernelStart();

  /* Normally never reached */
  while (1)
  {
  }
}

/**
 * @brief  System clock configuration: sysclk=168MHz, pclk1=42MHz, pclk2=84MHz
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* Enable PWR clock and configure voltage scaling */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Configure clock sources and PLL */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON; /* 8MHz */
  RCC_OscInitStruct.HSIState = RCC_HSI_ON; /* 16MHz */
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }

  /* Configure AHB/APB bus clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }

  /* Output clock on MCO1 for debug observation */
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSI, RCC_MCODIV_1);
}

void Error_Handler(const char *FileName, int LineNumber)
{
  printf("Error Handler Entered, File: %s, Line: %d!\r\n", FileName, LineNumber);
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Parameter assertion failure callback
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* Add log output here if needed */
  (void)file;
  (void)line;
}
#endif /* USE_FULL_ASSERT */

/* ==================== 8. Static private function implementations ==================== */
/* None */
