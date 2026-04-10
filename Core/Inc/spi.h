/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.h
  * @brief   This file contains all the function prototypes for
  *          the spi.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_SPI1_Init(void);
void MX_SPI2_Init(void);
void MX_SPI3_Init(void);

#define SPI2_SLAVE_RX_LEN 64U
#define SPI2_SLAVE_TX_LEN 64U

extern volatile uint8_t spi_rx_flag;
extern volatile uint8_t spi_tx_flag;
extern uint8_t g_spi2_rx_buf[SPI2_SLAVE_RX_LEN];
extern uint8_t g_spi2_tx_buf[SPI2_SLAVE_TX_LEN];

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern SPI_HandleTypeDef hspi3;

void SPI2_Slave_StartRx_IT(void);
HAL_StatusTypeDef SPI2_Slave_Send_IT(const uint8_t *data, uint16_t len);

/* 供中断回调转发调用 */
void SPI2_Slave_OnRxCplt_IT(SPI_HandleTypeDef *hspi);
void SPI2_Slave_OnTxCplt_IT(SPI_HandleTypeDef *hspi);
void SPI2_Slave_OnError_IT(SPI_HandleTypeDef *hspi);

/* 业务处理：你可在别的 .c 里重写 */
void SPI2_Slave_ProcessFrame(const uint8_t *rx, uint16_t rx_len, uint8_t *tx, uint16_t *tx_len);

void M_INT_LOW();
void M_INT_HIGH();
#ifdef __cplusplus
}
#endif

#endif /* __SPI_H__ */

