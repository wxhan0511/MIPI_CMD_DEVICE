/**
 * @file    boot_spi.c
 * @brief   SPI2 slave transport (polling, no DMA).
 *
 * The host PC is the SPI master; the device stays a slave exactly like in the
 * application firmware:
 *   PB12 = SPI2_NSS (hardware input), PB13 = SPI2_SCK,
 *   PC2  = SPI2_MISO,               PC3  = SPI2_MOSI.
 *
 * Upgrade framing is two-phase simplex per command:
 *   1) the host clocks out a 64-byte command frame, the slave receives it;
 *   2) the bootloader processes the command, then the host clocks in the
 *      64-byte response with a dummy read transaction.
 */

#include "boot_spi.h"

static SPI_HandleTypeDef hspi2;
static volatile uint8_t rx_complete;
static volatile uint8_t tx_complete;

void Boot_Spi_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_SPI2_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /**SPI2 GPIO Configuration
  PB12 ------> SPI2_NSS
  PB13 ------> SPI2_SCK
  PC2  ------> SPI2_MISO
  PC3  ------> SPI2_MOSI
  */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_13;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_SLAVE;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_HARD_INPUT;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }

  /* M_INT (PC4) is a push-pull output used to notify the host */
  GPIO_InitStruct.Pin = BOOT_M_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_WritePin(BOOT_M_INT_GPIO_Port, BOOT_M_INT_Pin, GPIO_PIN_RESET);
  HAL_GPIO_Init(BOOT_M_INT_GPIO_Port, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(SPI2_IRQn, 0U, 0U);
  HAL_NVIC_EnableIRQ(SPI2_IRQn);
}

void Boot_Spi_DeInit(void)
{
  HAL_NVIC_DisableIRQ(SPI2_IRQn);
  HAL_SPI_DeInit(&hspi2);
}

HAL_StatusTypeDef Boot_Spi_ReceiveFrame(uint8_t *frame, uint32_t timeout_ms)
{
  rx_complete = 0U;
  HAL_StatusTypeDef status = HAL_SPI_Receive_IT(&hspi2, frame, BOOT_FRAME_LEN);
  if (status != HAL_OK)
    return status;

  Boot_Spi_SignalBusy();
  uint32_t busy_start = HAL_GetTick();
  while (HAL_GetTick() - busy_start < 2U)
  {
  }
  Boot_Spi_SignalReady();

  uint32_t start = HAL_GetTick();
  while (!rx_complete)
  {
    if (HAL_GetTick() - start > timeout_ms)
    {
      HAL_SPI_Abort(&hspi2);
      Boot_Spi_SignalBusy();
      return HAL_TIMEOUT;
    }
  }
  return HAL_OK;
}

HAL_StatusTypeDef Boot_Spi_SendFrame(const uint8_t *frame, uint32_t timeout_ms)
{
  return HAL_SPI_Transmit(&hspi2, (uint8_t *)frame, BOOT_FRAME_LEN, timeout_ms);
}

/* Arm the response non-blocking: M_INT must go HIGH as soon as the response
   is loaded (matching the application's semantics), and the host's dummy
   read transaction clocks it out via the interrupt. */
HAL_StatusTypeDef Boot_Spi_ArmResponse(const uint8_t *frame)
{
  tx_complete = 0U;
  return HAL_SPI_Transmit_IT(&hspi2, (uint8_t *)frame, BOOT_FRAME_LEN);
}

/* Wait until the armed response has been clocked out by the host. */
HAL_StatusTypeDef Boot_Spi_WaitResponseDone(uint32_t timeout_ms)
{
  uint32_t start = HAL_GetTick();
  while (!tx_complete)
  {
    if (HAL_GetTick() - start > timeout_ms)
    {
      HAL_SPI_Abort(&hspi2);
      return HAL_TIMEOUT;
    }
  }
  return HAL_OK;
}

void Boot_Spi_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hspi2);
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI2)
  {
    rx_complete = 1U;
    Boot_Spi_SignalBusy();
  }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI2)
  {
    tx_complete = 1U;
    Boot_Spi_SignalBusy();
  }
}

void Boot_Spi_SignalReady(void)
{
  HAL_GPIO_WritePin(BOOT_M_INT_GPIO_Port, BOOT_M_INT_Pin, GPIO_PIN_SET);
}

void Boot_Spi_SignalBusy(void)
{
  HAL_GPIO_WritePin(BOOT_M_INT_GPIO_Port, BOOT_M_INT_Pin, GPIO_PIN_RESET);
}
