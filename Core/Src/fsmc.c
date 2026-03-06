/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : FSMC.c
  * Description        : This file provides code for the configuration
  *                      of the FSMC peripheral.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "fsmc.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

SRAM_HandleTypeDef hsram1;

/* FSMC initialization function */
void MX_FSMC_Init(void)
{
  /* USER CODE BEGIN FSMC_Init 0 */

  /* USER CODE END FSMC_Init 0 */

  FSMC_NORSRAM_TimingTypeDef Timing = {0};

  /* USER CODE BEGIN FSMC_Init 1 */
  FSMC_NORSRAM_TimingTypeDef Timing2 = {0};
  /* USER CODE END FSMC_Init 1 */

  /** Perform the SRAM1 memory initialization sequence
  */
  hsram1.Instance = FSMC_NORSRAM_DEVICE;
  hsram1.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
  /* hsram1.Init */
  hsram1.Init.NSBank = FSMC_NORSRAM_BANK1;
  hsram1.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
  hsram1.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
  hsram1.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_8;
  hsram1.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
  hsram1.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
  hsram1.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
  hsram1.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
  hsram1.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
  hsram1.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
  hsram1.Init.ExtendedMode = FSMC_EXTENDED_MODE_ENABLE;
  hsram1.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
  hsram1.Init.WriteBurst = FSMC_WRITE_BURST_ENABLE;
  hsram1.Init.PageSize = FSMC_PAGE_SIZE_NONE;
  /* Timing */
  // Timing.AddressSetupTime = 1;
  // Timing.AddressHoldTime = 15;
  // Timing.DataSetupTime = 4;
  // Timing.BusTurnAroundDuration = 15;
  // Timing.CLKDivision = 16;
  // Timing.DataLatency = 17;
  // Timing.AccessMode = FSMC_ACCESS_MODE_A;
  // /* ExtTiming */
  //
  // if (HAL_SRAM_Init(&hsram1, &Timing, NULL) != HAL_OK)
  // {
  //   Error_Handler(__FILE__, __LINE__);
  // }

  /* USER CODE BEGIN FSMC_Init 2 */
  /* FSMC读时序控制寄存器 */
  Timing.AddressSetupTime = 0x0F;           /* 地址建立时间(ADDSET)为15个fsmc_ker_ck(1/168=6)即6*15=90ns */
  Timing.AddressHoldTime = 0x00;            /* 地址保持时间(ADDHLD) 模式A是没有用到 */
  Timing.DataSetupTime = 60;                /* 数据保存时间(DATAST)为60个fsmc_ker_ck=6*60=360ns */
  /* 因为液晶驱动IC的读数据的时候,速度不能太快,尤其是个别奇葩芯片 */
  Timing.AccessMode = FSMC_ACCESS_MODE_A;   /* 模式A */
  /* FSMC写时序控制寄存器 */
  // 调整时序参数
  Timing2.AddressSetupTime = 10;        // 增大地址建立时间
  Timing.AddressHoldTime = 2;          // 增大地址保持时间
  Timing.DataSetupTime = 15;           // 增大数据建立时间
  Timing.BusTurnAroundDuration = 2;    // 增大总线转换时间
  Timing.CLKDivision = 2;
  Timing.DataLatency = 2;
  Timing.AccessMode = FSMC_ACCESS_MODE_A;  // 使用AccessMode A
  if (HAL_SRAM_Init(&hsram1, &Timing, &Timing2) != HAL_OK)
  {
    printf("fsmc write burst enable failed\r\n");
  }
  /* USER CODE END FSMC_Init 2 */
}

static uint32_t FSMC_Initialized = 0;

static void HAL_FSMC_MspInit(void){
  /* USER CODE BEGIN FSMC_MspInit 0 */

  /* USER CODE END FSMC_MspInit 0 */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (FSMC_Initialized) {
    return;
  }
  FSMC_Initialized = 1;

  /* Peripheral clock enable */
  __HAL_RCC_FSMC_CLK_ENABLE();

  /** FSMC GPIO Configuration
  PE7   ------> FSMC_D4
  PE8   ------> FSMC_D5
  PE9   ------> FSMC_D6
  PE10   ------> FSMC_D7
  PD11   ------> FSMC_A16
  PD14   ------> FSMC_D0
  PD15   ------> FSMC_D1
  PD0   ------> FSMC_D2
  PD1   ------> FSMC_D3
  PD4   ------> FSMC_NOE
  PD5   ------> FSMC_NWE
  PD7   ------> FSMC_NE1
  */
  /* GPIO_InitStruct */
  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;

  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* GPIO_InitStruct */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0
                          |GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;

  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN FSMC_MspInit 1 */

  /* USER CODE END FSMC_MspInit 1 */
}

void HAL_SRAM_MspInit(SRAM_HandleTypeDef* sramHandle){
  /* USER CODE BEGIN SRAM_MspInit 0 */

  /* USER CODE END SRAM_MspInit 0 */
  HAL_FSMC_MspInit();
  /* USER CODE BEGIN SRAM_MspInit 1 */

  /* USER CODE END SRAM_MspInit 1 */
}

static uint32_t FSMC_DeInitialized = 0;

static void HAL_FSMC_MspDeInit(void){
  /* USER CODE BEGIN FSMC_MspDeInit 0 */

  /* USER CODE END FSMC_MspDeInit 0 */
  if (FSMC_DeInitialized) {
    return;
  }
  FSMC_DeInitialized = 1;
  /* Peripheral clock enable */
  __HAL_RCC_FSMC_CLK_DISABLE();

  /** FSMC GPIO Configuration
  PE7   ------> FSMC_D4
  PE8   ------> FSMC_D5
  PE9   ------> FSMC_D6
  PE10   ------> FSMC_D7
  PD11   ------> FSMC_A16
  PD14   ------> FSMC_D0
  PD15   ------> FSMC_D1
  PD0   ------> FSMC_D2
  PD1   ------> FSMC_D3
  PD4   ------> FSMC_NOE
  PD5   ------> FSMC_NWE
  PD7   ------> FSMC_NE1
  */

  HAL_GPIO_DeInit(GPIOE, GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10);

  HAL_GPIO_DeInit(GPIOD, GPIO_PIN_11|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0
                          |GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7);

  /* USER CODE BEGIN FSMC_MspDeInit 1 */

  /* USER CODE END FSMC_MspDeInit 1 */
}

void HAL_SRAM_MspDeInit(SRAM_HandleTypeDef* sramHandle){
  /* USER CODE BEGIN SRAM_MspDeInit 0 */

  /* USER CODE END SRAM_MspDeInit 0 */
  HAL_FSMC_MspDeInit();
  /* USER CODE BEGIN SRAM_MspDeInit 1 */

  /* USER CODE END SRAM_MspDeInit 1 */
}
/**
  * @}
  */

/**
  * @}
  */
void fsmc_write_burst_en(uint8_t enable)
{
  /* USER CODE BEGIN FSMC_Init 0 */

  /* USER CODE END FSMC_Init 0 */

  FSMC_NORSRAM_TimingTypeDef Timing = {0};

  /* USER CODE BEGIN FSMC_Init 1 */
  FSMC_NORSRAM_TimingTypeDef Timing2 = {0};
  /* USER CODE END FSMC_Init 1 */

  /** Perform the SRAM1 memory initialization sequence
  */
  hsram1.Instance = FSMC_NORSRAM_DEVICE;
  hsram1.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
  /* hsram1.Init */
  hsram1.Init.NSBank = FSMC_NORSRAM_BANK1;
  hsram1.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
  hsram1.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
  hsram1.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_8;
  hsram1.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
  hsram1.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
  hsram1.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
  hsram1.Init.WaitSignalActive = FSMC_WAIT_TIMING_DURING_WS;
  hsram1.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
  hsram1.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
  hsram1.Init.ExtendedMode = FSMC_EXTENDED_MODE_ENABLE;
  hsram1.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
  if(enable == 1)
    hsram1.Init.WriteBurst = FSMC_WRITE_BURST_ENABLE;
  else
    hsram1.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
  hsram1.Init.PageSize = FSMC_PAGE_SIZE_NONE;
  /* Timing */
  // Timing.AddressSetupTime = 1;
  // Timing.AddressHoldTime = 15;
  // Timing.DataSetupTime = 4;
  // Timing.BusTurnAroundDuration = 15;
  // Timing.CLKDivision = 16;
  // Timing.DataLatency = 17;
  // Timing.AccessMode = FSMC_ACCESS_MODE_A;
  // /* ExtTiming */
  //
  // if (HAL_SRAM_Init(&hsram1, &Timing, NULL) != HAL_OK)
  // {
  //   Error_Handler(__FILE__, __LINE__);
  // }

  /* USER CODE BEGIN FSMC_Init 2 */
  /* FSMC读时序控制寄存器 */
  Timing.AddressSetupTime = 0x11;           /* 地址建立时间(ADDSET)为15个fsmc_ker_ck(1/168=6)即6*15=90ns */
  Timing.AddressHoldTime = 0x00;            /* 地址保持时间(ADDHLD) 模式A是没有用到 */
  Timing.DataSetupTime = 0x55;                /* 数据保存时间(DATAST)为60个fsmc_ker_ck=6*60=360ns */
  /* 因为液晶驱动IC的读数据的时候,速度不能太快,尤其是个别奇葩芯片 */
  Timing.AccessMode = FSMC_ACCESS_MODE_B;   /* 模式A */
  /* FSMC写时序控制寄存器 */
  Timing2.AddressSetupTime = 10;        // 地址建立时间
  Timing2.AddressHoldTime = 10;         // 地址保持时间
  Timing2.DataSetupTime = 9;           // 数据建立时间
  Timing2.BusTurnAroundDuration = 1;
  Timing2.CLKDivision = 2;
  Timing2.DataLatency = 2;
  Timing2.AccessMode = FSMC_ACCESS_MODE_A;  // 使用AccessMode A
  if (HAL_SRAM_Init(&hsram1, &Timing, &Timing2) != HAL_OK)
  {
    printf("fsmc write burst enable failed\r\n");
  }
  /* USER CODE END FSMC_Init 2 */
}