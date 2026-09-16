/**
 * @file    boot_spi.h
 * @brief   SPI2 slave transport used by the upgrade protocol.
 */

#ifndef BOOT_SPI_H
#define BOOT_SPI_H

#include "main.h"

#define BOOT_FRAME_LEN 64U

void Boot_Spi_Init(void);
void Boot_Spi_DeInit(void);
HAL_StatusTypeDef Boot_Spi_ReceiveFrame(uint8_t *frame, uint32_t timeout_ms);
HAL_StatusTypeDef Boot_Spi_SendFrame(const uint8_t *frame, uint32_t timeout_ms);
HAL_StatusTypeDef Boot_Spi_ArmResponse(const uint8_t *frame);
HAL_StatusTypeDef Boot_Spi_WaitResponseDone(uint32_t timeout_ms);
void Boot_Spi_IRQHandler(void);
void Boot_Spi_SignalReady(void);
void Boot_Spi_SignalBusy(void);

#endif /* BOOT_SPI_H */
