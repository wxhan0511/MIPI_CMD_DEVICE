/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
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
#include "tim.h"
#include "stm32f4xx_hal_tim_ex.h"
/* USER CODE BEGIN 0 */
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

static uint16_t pwm_arr;  // Auto-reload value (stores arr, used for duty cycle calculation)
/**
  * @brief  TIM1 PWM Initialization (Channel 2, PE11 Pin)
  * @param  arr: Auto-reload value (determines PWM period)
  * @param  psc: Prescaler (determines the timer clock frequency)
  * T = (arr + 1) * (psc + 1) / Fclk
  */
void TIM1_PWM_Init(uint16_t arr, uint16_t  psc) {
   /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = arr-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = psc-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  /*当APB1的预分频系数为1时，这个倍频器不起作用，定时器的时钟频率等于APB1的频率；当APB1的预分频系数为其它数值(即预分频系数为2、4、8或16)时，这个倍频器起作用，定时器的时钟频率等于APB1的频率两倍。*/
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;//APB2 Bus Clock 84MHZ×2=168MHZ
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 10;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}


void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM1)
  {
    __HAL_RCC_TIM1_CLK_ENABLE();
  }
  if(tim_baseHandle->Instance==TIM2)
  {
    __HAL_RCC_TIM2_CLK_ENABLE();
  }
}
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* tim_pwmHandle)
{

  if(tim_pwmHandle->Instance==TIM2)
  {
  /* USER CODE BEGIN TIM2_MspInit 0 */

  /* USER CODE END TIM2_MspInit 0 */
    /* TIM2 clock enable */
    __HAL_RCC_TIM2_CLK_ENABLE();
  /* USER CODE BEGIN TIM2_MspInit 1 */

  /* USER CODE END TIM2_MspInit 1 */
  }
}
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM1)
  {
    __HAL_RCC_TIM1_CLK_DISABLE();
  }
  if(tim_baseHandle->Instance==TIM2)
    {
      __HAL_RCC_TIM2_CLK_DISABLE();
    }
}


void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(timHandle->Instance==TIM1)
  {
  /* USER CODE BEGIN TIM1_MspPostInit 0 */

  /* USER CODE END TIM1_MspPostInit 0 */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    /**TIM1 GPIO Configuration
    PE11     ------> TIM1_CH2
    */
    GPIO_InitStruct.Pin = LED_PWM_IN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(LED_PWM_IN_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM1_MspPostInit 1 */

  /* USER CODE END TIM1_MspPostInit 1 */
  }

}

/**
  * @brief  使用TIM1的重复计数器功能生成指定数量的PWM脉冲
  * @param  num_pulses: 要生成的脉冲数量
  * @retval None
  */
void TIM1_Generate_N_Pulses(uint16_t num_pulses)
{
  if (num_pulses == 0)
  {
    return;
  }
  /* 使能TIM1更新中断 */
  HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
  /* 设置重复计数值。硬件会在 (num_pulses) 次更新事件后才触发中断 */
  /* 注意：RepetitionCounter 寄存器需要写入 N-1 才能得到 N 个脉冲 */
    // 1. 设置重复计数值
  htim1.Instance->RCR = num_pulses - 1;

  // 2. 关键步骤：手动触发一次更新事件 (Update Event)
  //    这会强制将 RCR 寄存器中的值加载到有效计数器中。
  //    同时，它也会清除计数器 CNT，确保从0开始。
  HAL_TIM_GenerateEvent(&htim1, TIM_EVENTSOURCE_UPDATE);

  // 3. 清除更新中断标志位
  //    因为上一步手动触发了更新事件，会留下一个中断标志位，
  //    如果不清除，会立即进入中断，导致行为错误。
  __HAL_TIM_CLEAR_IT(&htim1, TIM_IT_UPDATE);


}

/**
  * @brief  定时器周期溢出回调函数
  * @note   当TIM1完成指定数量的脉冲后，会进入此函数
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */

  /* USER CODE BEGIN Callback 1 */

  // 判断是否是TIM1的更新中断
  if (htim->Instance == TIM1)
  {
    // 停止TIM1的PWM输出
    HAL_TIM_PWM_Stop_IT(&htim1, TIM_CHANNEL_2);
    // 停止定时器基础部分 (和中断)
    HAL_TIM_Base_Stop_IT(&htim1);
    printf("TIM1 PWM pulse generation completed.\n");
  }

  /* USER CODE END Callback 1 */
}



