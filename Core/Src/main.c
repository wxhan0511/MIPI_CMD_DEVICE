/**
 * @file       main.c
 * @brief      Main program body
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-12-31
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

/* ==================== 1. 头文件包含 ==================== */
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

/* ==================== 2. 宏定义 ==================== */
/* 无 */

/* ==================== 3. 类型定义（结构体、枚举、别名） ==================== */
/* 无 */

/* ==================== 4. 外部全局变量 ==================== */
/* 无 */

/* ==================== 5. 静态私有变量 ==================== */
/* 无 */

/* ==================== 6. 静态函数声明 ==================== */
/* 无 */

/* ==================== 外部函数声明 ==================== */
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);

/* ==================== 7. 外部可调用函数实现 ==================== */
int main(void)
{
  /* HAL基础初始化 */
  HAL_Init();

  /* 系统时钟配置 */
  SystemClock_Config();

  /* 外设初始化 */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init(); /* PB6 PB7 */
  MX_I2C2_Init(); /* PB10 PB11 */
  MX_CRC_Init();
  MX_FSMC_Init();
  MX_DAC_Init();
  MX_SPI1_Init(); /* ADS1256 */
  // MX_SPI2_Init(); /* M SPI */
  MX_SPI3_Init(); /* FLASH */
  MX_USART3_UART_Init();

  /* 板级初始化 */
  bsp_init();

  /* RTOS初始化与启动 */
  osKernelInitialize();
  MX_FREERTOS_Init();
  osKernelStart();

  /* 正常不会到达此处 */
  while (1)
  {
  }
}

/**
 * @brief  系统时钟配置：sysclk=168MHz, pclk1=42MHz, pclk2=84MHz
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /* 使能电源时钟并配置电压缩放 */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* 配置时钟源和PLL */
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

  /* 配置AHB/APB总线时钟 */
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

  /* 输出时钟到MCO1用于调试观察 */
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
 * @brief  参数断言失败回调
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* 可按需添加日志打印 */
  (void)file;
  (void)line;
}
#endif /* USE_FULL_ASSERT */

/* ==================== 8. 静态私有函数实现 ==================== */
/* 无 */
