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
#include <math.h>
/* Private variables ---------------------------------------------------------*/
/* Timer handler declaration */
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim8;
/* Timer Input Capture Configuration Structure declaration */
TIM_IC_InitTypeDef       sConfig;
/* Slave configuration structure */
TIM_SlaveConfigTypeDef   sSlaveConfig;
/* Captured Value */
__IO uint32_t            uwIC2Value = 0;
/* Duty Cycle Value */
__IO uint32_t            uwDutyCycle = 0;
/* Frequency Value */
__IO uint32_t            uwFrequency = 0;


static uint32_t sample_count = 0;

#define SAMPLE_WINDOW 1000U
#define GRADIENT_STEP 0.1f
#define GRADIENT_BUCKETS 100U  // 0.1%, 0.2%, …, 1.0%

static uint32_t freq_samples[SAMPLE_WINDOW];
static uint32_t duty_samples[SAMPLE_WINDOW];
static uint32_t sample_index;

/**
 * @brief Store each sample until the window is filled.
 */
static inline void StoreSample(uint32_t freq, uint32_t duty)
{
    if (sample_index < SAMPLE_WINDOW) {
        freq_samples[sample_index] = freq;
        duty_samples[sample_index] = duty;
        sample_index++;
    }
}

/**
 * @brief Analyze deviations in 0.1% steps up to 1% and print counts.
 */
static void PrintDeviationHistogram(const uint32_t *samples, const char *label)
{
    float sum = 0.0f;
    for (uint32_t i = 0; i < SAMPLE_WINDOW; ++i) {
        sum += (float)samples[i];
    }
    const float mean = sum / SAMPLE_WINDOW;

    uint32_t bucket_counts[GRADIENT_BUCKETS] = {0};
    uint32_t over_one_percent = 0;

    for (uint32_t i = 0; i < SAMPLE_WINDOW; ++i) {
        const float deviation_pct = fabsf((samples[i] - mean) / mean) * 100.0f;
        const uint32_t bucket = (uint32_t)(deviation_pct / GRADIENT_STEP);

        if (bucket < GRADIENT_BUCKETS) {
            bucket_counts[bucket]++;
        } else {
            over_one_percent++;
        }
    }

    printf("%s mean = %.3f\r\n", label, mean);
    for (uint32_t i = 0; i < GRADIENT_BUCKETS; ++i) {
        const float upper = (i + 1) * GRADIENT_STEP;
        printf(" %.1f%%: %lu samples\r\n", upper, bucket_counts[i]);
    }
    printf(" Greater than 3.0%%: %lu samples\r\n", over_one_percent);
}
/**
 * Call this once you collected 1000 samples.
 */
static void FinishSampleWindow(void)
{
    PrintDeviationHistogram(freq_samples, "Frequency");
    PrintDeviationHistogram(duty_samples, "Duty");
    sample_index = 0;
}

void TIM1_CCP_Init(void)
{
  /*##-1- Configure the TIM peripheral #######################################*/ 
  /* Set TIMx instance */
  htim1.Instance = TIM1;// APB2上限84MHZ,APB1上限42MHZ(见cubeide clock confi图),TIM1 的最高时钟可以达到 168 MHz

  /* Initialize TIMx peripheral as follow:
       + Period = 0xFFFF
       + Prescaler = 0
       + ClockDivision = 0
       + Counter direction = Up
  */
  htim1.Init.Period = 0xFFFF;//65535 最大测量时间 = 65536 * (1 / 168,000,000) ≈ 0.00039 秒 ≈ 0.39 毫秒 (ms),实际信号周期 不可以大于定时器最大测量时间 (0.39 ms),实际信号频率要大于2600hz
  htim1.Init.Prescaler = 0;//TIM8 的计数时钟168MHz
  htim1.Init.ClockDivision = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if(HAL_TIM_IC_Init(&htim1) != HAL_OK)//表明这个定时器主要用于输入捕获功能
  {
    /* Initialization Error */
    Error_Handler();
  }
  
  /*##-2- Configure the Input Capture channels ###############################*/ 
  /* Common configuration */
  sConfig.ICPrescaler = TIM_ICPSC_DIV1;//设置输入捕获的预分频器。DIV1 表示每个有效的边沿都会触发一次捕获
  sConfig.ICFilter = 0;//  设置输入滤波器。0 表示不使用滤波器，可以获得最快的响应，但抗干扰能力较弱。
  
  /* Configure the Input Capture of channel 1 */
  sConfig.ICPolarity = TIM_ICPOLARITY_FALLING;//设置通道1捕获下降沿。
  sConfig.ICSelection = TIM_ICSELECTION_INDIRECTTI;//设置通道1的输入选择为间接输入（Indirect TI）。这意味着它连接到另一个通道的输入（TI2）。这是实现 PWM 信号测量的关键配置之一。    
  if(HAL_TIM_IC_ConfigChannel(&htim1, &sConfig, TIM_CHANNEL_1) != HAL_OK)
  {
    /* Configuration Error */
    Error_Handler();
  }
  
  /* Configure the Input Capture of channel 2 */
  sConfig.ICPolarity = TIM_ICPOLARITY_RISING;//设置通道2捕获上升沿
  sConfig.ICSelection = TIM_ICSELECTION_DIRECTTI;//设置通道2的输入选择为直接输入（Direct TI）。这意味着它直接连接到 TIM1_CH2 的 GPIO 引脚
  if(HAL_TIM_IC_ConfigChannel(&htim1, &sConfig, TIM_CHANNEL_2) != HAL_OK)
  {
    /* Configuration Error */
    Error_Handler();
  }
  /*##-3- Configure the slave mode ###########################################*/
  /* Select the slave Mode: Reset Mode */
  sSlaveConfig.SlaveMode     = TIM_SLAVEMODE_RESET;//选择复位模式。在这种模式下，当触发信号到来时，计数器 CNT 会被清零并重新开始计数。
  sSlaveConfig.InputTrigger  = TIM_TS_TI2FP2;//选择 TI2FP2（通道2的滤波后输入）作为触发源。结合上一条，这意味着每当通道2检测到一个上升沿时，定时器计数器就会复位为0。
  if(HAL_TIM_SlaveConfigSynchronization(&htim1, &sSlaveConfig) != HAL_OK)
  {
    /* Configuration Error */
    Error_Handler();
  }

}

void enableTim1CaptureCompareInterrupt(void)
{
      /*##-4- Start the Input Capture in interrupt mode ##########################*/
  if(HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_2) != HAL_OK)//以中断方式启动通道2的输入捕获
  {
    /* Starting Error */
    Error_Handler();
  }
  
  /*##-5- Start the Input Capture in interrupt mode ##########################*/
  if(HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1) != HAL_OK)
  {
    /* Starting Error */
    Error_Handler();
  }
  
  /*##-6- Enable the TIM1 global Interrupt ####################################*/
  HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
}

void disableTim1CaptureCompareInterrupt(void)
{
    /*##-7- Disable the TIM1 global Interrupt ####################################*/
    HAL_NVIC_DisableIRQ(TIM1_CC_IRQn);
  
    /*##-8- Stop the Input Capture in interrupt mode ##########################*/
    if(HAL_TIM_IC_Stop_IT(&htim1, TIM_CHANNEL_2) != HAL_OK)
    {
      /* Stopping Error */
      Error_Handler();
    }
    
    /*##-9- Stop the Input Capture in interrupt mode ##########################*/
    if(HAL_TIM_IC_Stop_IT(&htim1, TIM_CHANNEL_1) != HAL_OK)
    {
      /* Stopping Error */
      Error_Handler();
    }
}

void enableTim1PWMOutput(void)
{
    HAL_TIM_PWM_Start(&htim1,LED_PWM_IN_CHANNEL);
    if (HAL_TIM_Base_Start_IT(&htim1) != HAL_OK)
    {
        Error_Handler();
    }
}
void disableTim1PWMOutput(void)
{
    HAL_TIM_PWM_Stop(&htim1,LED_PWM_IN_CHANNEL);
    if (HAL_TIM_Base_Stop_IT(&htim1) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief  TIM1 PWM Initialization (Channel 2, PE11 Pin)
  * @param  arr: Auto-reload value (determines PWM period)
  * @param  psc: Prescaler (determines the timer clock frequency)
  * @param  pulse: Pulse value (determines PWM duty cycle)
  * @retval None
  * T = (arr + 1) * (psc + 1) / Fclk
  * arr,psc f=168MHz/(arry*psc)    最大可用28MHZ TIM1_PWM_Init(2,3),比较值设置为1,__HAL_TIM_SET_COMPARE(&htim1, LED_PWM_IN_CHANNEL, 1);
  */
void TIM1_PWM_Init(uint16_t arr, uint16_t  psc, uint16_t pulse) {
   /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = psc-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = arr-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  /*当你因为总线速度限制而不得不降低整个 APB1 总线的时钟时（即预分频系数 > 1），系统会自动将供给定时器的时钟频率乘以2。*/
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

  __HAL_TIM_SET_COMPARE(&htim1, LED_PWM_IN_CHANNEL, pulse);

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

  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
    return;
  }

  // 判断是否是TIM1的更新中断
  if (htim->Instance == TIM1)
  {
    // 停止TIM1的PWM输出
    HAL_TIM_PWM_Stop_IT(&htim1, TIM_CHANNEL_2);
    // 停止定时器基础部分 (和中断)
    HAL_TIM_Base_Stop_IT(&htim1);
  }

  /* USER CODE END Callback 1 */
}

/**
  * @brief  Input Capture callback in non blocking mode 
  * @param  htim: TIM IC handle
  * @retval None
  */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
  {
    /* Get the Input Capture value */
    uwIC2Value = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);//周期值
    
    if (uwIC2Value != 0)
    {
      /* Duty cycle computation */
      uwDutyCycle = ((HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1)) * 100) / uwIC2Value;//占空比
      
      /* uwFrequency computation */    
      uwFrequency = 168000000 / uwIC2Value;
      
      //测量1000次取第100次到1100次的平均值
      if (sample_count >= 100 && sample_count <= 1100)
      {
        StoreSample(uwFrequency, uwDutyCycle);
      }
      sample_count++;
      
      if (sample_count >= 1100)
      {
        //关闭中断
        HAL_NVIC_DisableIRQ(TIM1_CC_IRQn);
        FinishSampleWindow();
        //printf("1000-time average :Frequency: %lu Hz, Duty Cycle: %lu %%\n", uwFrequency, uwDutyCycle);
      }

    }
    else
    {
      uwDutyCycle = 0;
      uwFrequency = 0;
    }
  }
}

/**
  * @brief TIM MSP Initialization 
  *        This function configures the hardware resources used in this example: 
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration  
  * @param htim: TIM handle pointer
  * @retval None
  */
void HAL_TIM_IC_MspInit(TIM_HandleTypeDef *htim)
{
  GPIO_InitTypeDef   GPIO_InitStruct;
 
  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* TIMx Peripheral clock enable */
  __HAL_RCC_TIM8_CLK_ENABLE();
    
  /* Enable GPIO channels Clock */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /* Configure  (TIMx_Channel) in Alternate function, push-pull and 100MHz speed */
  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*##-2- Configure the NVIC for TIMx #########################################*/
  /* Sets the priority grouping field */
  HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 1);
  
  /* Enable the TIM2 global Interrupt */
  HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
}

