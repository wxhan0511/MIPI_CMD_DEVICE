/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dac.c
  * @brief   This file provides code for the configuration
  *          of the DAC instances.
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
/* Includes ------------------------------------------------------------------*/
#include "dac.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

DAC_HandleTypeDef hdac;

/* DAC init function */
void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */

  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_2) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

void HAL_DAC_MspInit(DAC_HandleTypeDef* dacHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(dacHandle->Instance==DAC)
  {
  /* USER CODE BEGIN DAC_MspInit 0 */

  /* USER CODE END DAC_MspInit 0 */
    /* DAC clock enable */
    __HAL_RCC_DAC_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**DAC GPIO Configuration
    PA4     ------> DAC_OUT1
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DAC_LS_GPIO_Port, &GPIO_InitStruct);

    /* DAC interrupt Init */
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 15, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
  /* USER CODE BEGIN DAC_MspInit 1 */

  /* USER CODE END DAC_MspInit 1 */
  }
}

void HAL_DAC_MspDeInit(DAC_HandleTypeDef* dacHandle)
{

  if(dacHandle->Instance==DAC)
  {
  /* USER CODE BEGIN DAC_MspDeInit 0 */

  /* USER CODE END DAC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_DAC_CLK_DISABLE();

    /**DAC GPIO Configuration
    PA4     ------> DAC_OUT1
    */
    //HAL_GPIO_DeInit(LEVEL_SHIFT_DAC_GPIO_Port, LEVEL_SHIFT_DAC_Pin);

    /* DAC interrupt Deinit */
    HAL_NVIC_DisableIRQ(TIM6_DAC_IRQn);
  /* USER CODE BEGIN DAC_MspDeInit 1 */

  /* USER CODE END DAC_MspDeInit 1 */
  }
}

void DAC_SetValue(float value)
{
    uint32_t dac_value = (uint32_t)((value / 3.3f) * 4095.0f);
    // HAL_DAC_Start(&hdac, DAC_CHANNEL_2);
    // if (HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_value) != HAL_OK)
    // {
    //     Error_Handler(__FILE__, __LINE__);
    // }
    // HAL_StatusTypeDef st;
    // st = HAL_DAC_Start(&hdac, DAC_CHANNEL_2);
    // printf("DAC start = %d\r\n", st);
    // st = HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 2000);
    // printf("DAC set = %d\r\n", st);   

    // GPIO_InitTypeDef GPIO_InitStruct = {0};
    // __HAL_RCC_GPIOA_CLK_ENABLE();
    // GPIO_InitStruct.Pin = GPIO_PIN_5;
    // GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    // GPIO_InitStruct.Pull = GPIO_NOPULL;
    // GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    // HAL_GPIO_Init(DAC_LS_GPIO_Port, &GPIO_InitStruct);
    // while(1){
    // HAL_GPIO_WritePin(DAC_LS_GPIO_Port, GPIO_PIN_5, GPIO_PIN_RESET);
    // app_delay(100);
    // HAL_GPIO_WritePin(DAC_LS_GPIO_Port, GPIO_PIN_5, GPIO_PIN_SET);
    // app_delay(100);
    // }
}
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
