/**
 * @file    boot_log.c
 * @brief   Blocking (polling) UART log output so the bootloader works without
 *          interrupts, DMA or RTOS -- the same USART3 console the application
 *          uses for its own logs.
 */

#include "boot_log.h"

#include <stdarg.h>
#include <stdio.h>

static UART_HandleTypeDef huart3;

void Boot_Log_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_USART3_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /**USART3 GPIO Configuration
  PD8  ------> USART3_TX
  PD9  ------> USART3_RX
  */
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200; /* Must match the application console */
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
}

void Boot_Log_Printf(const char *fmt, ...)
{
  static char buf[160];
  va_list args;

  va_start(args, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (n < 0)
  {
    return;
  }
  if (n >= (int)sizeof(buf) - 2)
  {
    n = (int)sizeof(buf) - 2;
  }
  buf[n++] = '\r';
  buf[n++] = '\n';

  (void)HAL_UART_Transmit(&huart3, (uint8_t *)buf, (uint16_t)n, 100U);
}
