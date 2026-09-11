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
/* Private variables ---------------------------------------------------------*/
/* Timer handler declaration */
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim8;
/* Timer Input Capture Configuration Structure declaration */
TIM_IC_InitTypeDef sConfig;
/* Slave configuration structure */
TIM_SlaveConfigTypeDef sSlaveConfig;
/* Captured value, duty cycle and frequency; volatile for cross-task visibility */
volatile uint32_t uwDutyCycle = 0;
volatile uint32_t uwFrequency = 0;
volatile uint8_t get_freq_flag = 0;

/* Supports duty-cycle computation across overflow */
static volatile uint32_t last_ccr1 = 0U;
static volatile uint32_t last_ccr1_ovf = 0U;
static volatile uint8_t last_ccr1_valid = 0U;

/* Timer mode and overflow counting */
static volatile uint32_t tim1_ovf_cnt = 0U;

/* Raw capture data record */
typedef struct
{
  uint32_t cc1;
  uint32_t ovf1;
  uint32_t cc2;
  uint32_t ovf2;
} TIM1_RawCapture_t;

#define MAX_SAMPLES 10
static volatile TIM1_RawCapture_t capture_buffer[MAX_SAMPLES];
static volatile uint8_t raw_sample_idx = 0;

enum
{
  TIM1_MODE_IDLE = 0,
  TIM1_MODE_CAP_MEAS = 1,
  TIM1_MODE_PWM_BURST = 2
};
static volatile uint8_t tim1_mode = TIM1_MODE_IDLE;
#define DUTY_MEASURE_MAX_HZ 80000U // Duty cycle forced to 0 above 80 kHz

void TIM1_CCP_Init(void)
{
  HAL_TIM_PWM_DeInit(&htim1); // Stop all PWM
  HAL_TIM_IC_DeInit(&htim1);  // Stop all input capture
  /*##-1- Configure the TIM peripheral #######################################*/
  /* Set TIMx instance */
  htim1.Instance = TIM1; // APB2 max 84 MHz, APB1 max 42 MHz (see CubeIDE clock config); TIM1 can reach up to 168 MHz

  /* Initialize TIMx peripheral as follow:
       + Period = 0xFFFF
       + Prescaler = 0
       + ClockDivision = 0
       + Counter direction = Up
  */
  htim1.Init.Period = 0xFFFF; // 65535: max measurable time = 65536 * (1 / 168,000,000) ~ 0.39 ms; the signal period must not exceed this, i.e. the signal frequency must be > ~2600 Hz
  htim1.Init.Prescaler = 0;   // TIM1 counting clock 168 MHz: fcnt = 168 MHz / (Prescaler + 1). Prescaler = 0 means no division, giving the highest time resolution for high-frequency measurement.
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_IC_Init(&htim1) != HAL_OK) // This timer is used mainly for input capture
  {
    /* Initialization Error */
    Error_Handler(__FILE__, __LINE__);
  }

  /*##-2- Configure the Input Capture channels ###############################*/
  /* Common configuration */
  sConfig.ICPrescaler = TIM_ICPSC_DIV1; // Input capture prescaler: DIV1 captures on every valid edge
  sConfig.ICFilter = 0;                 // Input filter: 0 means no filtering (fastest response, less noise immunity)

  /* Configure the Input Capture of channel 1 */
  sConfig.ICPolarity = TIM_ICPOLARITY_FALLING;      // Channel 1 captures the falling edge
  sConfig.ICSelection = TIM_ICSELECTION_INDIRECTTI; // Channel 1 uses the indirect input (TI2); one of the key settings for PWM measurement
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfig, TIM_CHANNEL_1) != HAL_OK)
  {
    /* Configuration Error */
    Error_Handler(__FILE__, __LINE__);
  }

  /* Configure the Input Capture of channel 2 */
  sConfig.ICPolarity = TIM_ICPOLARITY_RISING;     // Channel 2 captures the rising edge
  sConfig.ICSelection = TIM_ICSELECTION_DIRECTTI; // Channel 2 uses the direct input, wired to the TIM1_CH2 GPIO pin
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfig, TIM_CHANNEL_2) != HAL_OK)
  {
    /* Configuration Error */
    Error_Handler(__FILE__, __LINE__);
  }
  /*##-3- Configure the slave mode ###########################################*/
  /* Select the slave Mode: Reset Mode */
  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_RESET; // Reset mode: the counter (CNT) is cleared and restarts on each trigger
  sSlaveConfig.InputTrigger = TIM_TS_TI2FP2;    // TI2FP2 (filtered channel-2 input) as trigger: the counter resets on each channel-2 rising edge
  if (HAL_TIM_SlaveConfigSynchronization(&htim1, &sSlaveConfig) != HAL_OK)
  {
    /* Configuration Error */
    Error_Handler(__FILE__, __LINE__);
  }
  __HAL_TIM_URS_ENABLE(&htim1);
}

void enableTim1CaptureCompareInterrupt(void)
{
  tim1_mode = TIM1_MODE_CAP_MEAS;
  tim1_ovf_cnt = 0U;
  get_freq_flag = 0U;

  /*##-4- Start the Input Capture in interrupt mode ##########################*/
  if (HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_2) != HAL_OK) // Start channel-2 input capture in interrupt mode
  {
    /* Starting Error */
    Error_Handler(__FILE__, __LINE__);
  }

  /*##-5- Start the Input Capture in interrupt mode ##########################*/
  if (HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1) != HAL_OK)
  {
    /* Starting Error */
    Error_Handler(__FILE__, __LINE__);
  }
  /*##-6- Enable the TIM1 global Interrupt ####################################*/
  HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);

  // Key: enable the update interrupt for overflow counting
  __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE);
}

void disableTim1CaptureCompareInterrupt(void)
{
  /*##-7- Disable the TIM1 global Interrupt ####################################*/
  HAL_NVIC_DisableIRQ(TIM1_CC_IRQn);
  __HAL_TIM_DISABLE_IT(&htim1, TIM_IT_UPDATE);
  tim1_mode = TIM1_MODE_IDLE;
  tim1_ovf_cnt = 0U;
  /*##-8- Stop the Input Capture in interrupt mode ##########################*/
  if (HAL_TIM_IC_Stop_IT(&htim1, TIM_CHANNEL_2) != HAL_OK)
  {
    /* Stopping Error */
    Error_Handler(__FILE__, __LINE__);
  }

  /*##-9- Stop the Input Capture in interrupt mode ##########################*/
  if (HAL_TIM_IC_Stop_IT(&htim1, TIM_CHANNEL_1) != HAL_OK)
  {
    /* Stopping Error */
    Error_Handler(__FILE__, __LINE__);
  }
}

void enableTim1PWMOutput(void)
{
  tim1_mode = TIM1_MODE_PWM_BURST;
  HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
  if (HAL_TIM_Base_Start_IT(&htim1) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
}
void disableTim1PWMOutput(void)
{
  HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_3);
  if (HAL_TIM_Base_Stop_IT(&htim1) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  tim1_mode = TIM1_MODE_IDLE;
}
void enableTim2PWMOutput(void)
{
  if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
}
void disableTim2PWMOutput(void)
{
  if (HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  if (HAL_TIM_Base_Stop_IT(&htim2) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
}
void app_delay(uint32_t delay_ms)
{
  if (osKernelGetState() != osKernelRunning)
  {
    HAL_Delay(delay_ms);
  }
  else
  {
    osDelay(delay_ms);
  }
}

void TIM2_PWM_Init(uint16_t arr, uint16_t psc, uint16_t pulse)
{
  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = psc - 1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = arr - 1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.RepetitionCounter = 0;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  /* When the APB1 bus clock has to be lowered (prescaler > 1), the timer clock is automatically multiplied by 2. */
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL; // APB2 Bus Clock 84MHZ×2=168MHZ
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = pulse;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  /* TIM2 is a general-purpose timer: no complementary output / dead-time config */
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, pulse);
}
/**
 * @brief  TIM1 PWM Initialization (Channel 4, PE14 Pin)
 * @param  arr: Auto-reload value (determines PWM period)
 * @param  psc: Prescaler (determines the timer clock frequency)
 * @param  pulse: Pulse value (determines PWM duty cycle)
 * @retval None
 * arr, psc: f = 168MHz / ((arr+1) * (psc+1)), max usable ~28 MHz, e.g. TIM1_PWM_Init(2,3) with compare value 1: __HAL_TIM_SET_COMPARE(&htim1, LED_PWM_IN_CHANNEL, 1);
 */
void TIM1_PWM_Init(uint16_t arr, uint16_t psc, uint16_t pulse)
{
  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = psc - 1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = arr - 1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  /* When the APB1 bus clock has to be lowered (prescaler > 1), the timer clock is automatically multiplied by 2. */
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL; // APB2 Bus Clock 84MHZ×2=168MHZ
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = pulse;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
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
    Error_Handler(__FILE__, __LINE__);
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, pulse);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *tim_baseHandle)
{

  if (tim_baseHandle->Instance == TIM1)
  {
    /* 1. Enable clocks */
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /* 2. Configure PE11 (TIM1_CH2) as alternate function for pulse counting */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* 3. Configure NVIC (for overflow counting in gate mode) */
    HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
  }
  if (tim_baseHandle->Instance == TIM2)
  {
    __HAL_RCC_TIM2_CLK_ENABLE();
  }
}
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *tim_pwmHandle)
{

  if (tim_pwmHandle->Instance == TIM2)
  {
    /* USER CODE BEGIN TIM2_MspInit 0 */

    /* USER CODE END TIM2_MspInit 0 */
    /* TIM2 clock enable */
    __HAL_RCC_TIM2_CLK_ENABLE();
    /* USER CODE BEGIN TIM2_MspInit 1 */

    /* USER CODE END TIM2_MspInit 1 */
  }
  if (tim_pwmHandle->Instance == TIM1)
  {
    /* USER CODE BEGIN TIM1_MspInit 0 */
    __HAL_RCC_TIM1_CLK_ENABLE();
    /* USER CODE END TIM1_MspInit 0 */
  }
}
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *tim_baseHandle)
{

  if (tim_baseHandle->Instance == TIM1)
  {
    __HAL_RCC_TIM1_CLK_DISABLE();
  }
  if (tim_baseHandle->Instance == TIM2)
  {
    __HAL_RCC_TIM2_CLK_DISABLE();
  }
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *timHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (timHandle->Instance == TIM1)
  {
    /* USER CODE BEGIN TIM1_MspPostInit 0 */

    /* USER CODE END TIM1_MspPostInit 0 */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    /**TIM1 GPIO Configuration
    PE12     ------> TIM1_CH3
    */
    GPIO_InitStruct.Pin = LED_PWM_IN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(LED_PWM_IN_GPIO_Port, &GPIO_InitStruct);

    /* USER CODE BEGIN TIM1_MspPostInit 1 */

    /* USER CODE END TIM1_MspPostInit 1 */
  }
  if (timHandle->Instance == TIM2)
  {
    /* USER CODE BEGIN TIM2_MspPostInit 0 */

    /* USER CODE END TIM2_MspPostInit 0 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM2 GPIO Configuration
    PA3     ------> TIM2_CH4
    */
    GPIO_InitStruct.Pin = PWM_BLASI;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(PWM_BLASI_GPIO_Port, &GPIO_InitStruct);

    /* USER CODE BEGIN TIM2_MspPostInit 1 */

    /* USER CODE END TIM2_MspPostInit 1 */
  }
}

/**
 * @brief  Generate a fixed number of PWM pulses using the TIM1 repetition counter
 * @param  num_pulses: number of pulses to generate
 * @retval None
 */
void TIM1_Generate_N_Pulses(uint16_t num_pulses)
{
  if (num_pulses == 0)
  {
    return;
  }
  /* Enable the TIM1 update interrupt */
  HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
  /* Set the repetition counter. The hardware raises the interrupt after (num_pulses) update events */
  /* Note: the RepetitionCounter register must be loaded with N-1 to get N pulses */
  // 1. Set the repetition counter value
  htim1.Instance->RCR = num_pulses - 1;

  // 2. Key step: manually generate an update event
  //    This forces the RCR value into the active repetition counter,
  //    and also clears CNT so counting starts from 0.
  HAL_TIM_GenerateEvent(&htim1, TIM_EVENTSOURCE_UPDATE);

  // 3. Clear the update interrupt flag
  //    The manual update event above leaves the flag set; if not cleared,
  //    the ISR fires immediately and breaks the pulse count.
  __HAL_TIM_CLEAR_IT(&htim1, TIM_IT_UPDATE);
}

/**
 * @brief  Timer period elapsed callback
 * @note   Entered when TIM1 completes the requested number of pulses
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

  // Check for the TIM1 update interrupt
  if (htim->Instance == TIM1)
  {
    if (tim1_mode == TIM1_MODE_CAP_MEAS)
    {
      // Capture-measurement mode: update events only count overflows
      tim1_ovf_cnt++;
      return;
    }
    if (tim1_mode == TIM1_MODE_PWM_BURST)
    {
      // Stop the PWM pulse output
      HAL_TIM_PWM_Stop_IT(&htim1, TIM_CHANNEL_3);
      HAL_TIM_Base_Stop_IT(&htim1);
      tim1_mode = TIM1_MODE_IDLE;
      return;
    }
  }
  /* USER CODE END Callback 1 */
}
static uint32_t TIM1_GetCaptureClockHz(void)
{
  RCC_ClkInitTypeDef clk = {0};
  uint32_t latency = 0;
  uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();

  HAL_RCC_GetClockConfig(&clk, &latency);
  if (clk.APB2CLKDivider == RCC_HCLK_DIV1)
  {
    return pclk2;
  }
  return pclk2 * 2U;
}

/**
 * @brief Configure TIM1 in external clock mode (for high-frequency measurement)
 */
void TIM1_GateMode_Init(void)
{
  HAL_TIM_Base_DeInit(&htim1);

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 0xFFFF; // Must be 65535
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_Base_Init(&htim1);

  // External clock mode 1: counting pulses supplied by PE11 (TIM1_CH2)
  TIM_SlaveConfigTypeDef sSlaveConfig = {0};
  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;
  sSlaveConfig.InputTrigger = TIM_TS_TI2FP2;
  sSlaveConfig.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
  sSlaveConfig.TriggerFilter = 0;
  HAL_TIM_SlaveConfigSynchronization(&htim1, &sSlaveConfig);

  tim1_mode = TIM1_MODE_CAP_MEAS;
  tim1_ovf_cnt = 0;

  // Enable the overflow interrupt handling
  HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);

  __HAL_TIM_URS_ENABLE(&htim1);
  __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE);
}
/**
 * @brief Full-scale adaptive frequency measurement (gate method, 2 MHz - 50 MHz)
 * @return 0: success, -1: timeout
 */
int Measure_Frequency_Adaptive(void)
{
  // Force capture interrupts off
  HAL_NVIC_DisableIRQ(TIM1_CC_IRQn);
  __HAL_TIM_DISABLE_IT(&htim1, TIM_IT_CC1 | TIM_IT_CC2);
  tim1_mode = TIM1_MODE_CAP_MEAS; // Measurement mode so overflows are accumulated

  // Initialize external clock mode
  TIM1_GateMode_Init();

  uint64_t valid_sum = 0;

  // Measure 12 times, discard the first 2 and average the last 10
  for (int i = 0; i < 12; i++)
  {
    tim1_ovf_cnt = 0;
    __HAL_TIM_SET_COUNTER(&htim1, 0);

    HAL_TIM_Base_Start_IT(&htim1);
    HAL_Delay(100); // Wait 100 ms so the counter has enough time to count pulses
    uint32_t count = __HAL_TIM_GET_COUNTER(&htim1);
    uint32_t ovfs = tim1_ovf_cnt;
    HAL_TIM_Base_Stop_IT(&htim1);

    if (i >= 2)
    {
      valid_sum += ((uint64_t)count + (uint64_t)ovfs * 65536);
    }
  }
  uint32_t avg_freq = (uint32_t)((valid_sum * 0.009662149) * 100);

  uwFrequency = avg_freq;
  uwDutyCycle = 0; // Gate method cannot measure duty at high frequency; fixed at 0
  get_freq_flag = 1;
  return 0;
}
#if 1
/**
 * @brief  Input Capture callback in non blocking mode
 * @param  htim: TIM IC handle
 * @retval None
 */
/* ANCHOR - TIM1 input capture interrupt callback */
/* Records data only; no computation here */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance != TIM1 || tim1_mode != TIM1_MODE_CAP_MEAS)
    return;

  uint32_t current_ovf = tim1_ovf_cnt;

  /* --- Channel 1: falling edge (pulse width) --- */
  if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
  {
    last_ccr1 = htim->Instance->CCR1;
    last_ccr1_ovf = current_ovf;
    last_ccr1_valid = 1U;
  }

  /* --- Channel 2: rising edge (period start / reset) --- */
  if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
  {
    uint32_t ccr2 = htim->Instance->CCR2;
    uint32_t ovf2 = current_ovf; // Overflow count as of just before the reset
    tim1_ovf_cnt = 0U;           // Reset the software overflow counter

    // A sample is complete only if the falling edge (CC1) was caught this period
    if (last_ccr1_valid && raw_sample_idx < MAX_SAMPLES)
    {
      capture_buffer[raw_sample_idx].cc1 = last_ccr1;
      capture_buffer[raw_sample_idx].ovf1 = last_ccr1_ovf;
      capture_buffer[raw_sample_idx].cc2 = ccr2;
      capture_buffer[raw_sample_idx].ovf2 = ovf2;
      // printf("Captured Sample %u: CCR1 = %lu (Ovf: %lu), CCR2 = %lu (Ovf: %lu)\r\n",
      // raw_sample_idx, last_ccr1, last_ccr1_ovf, ccr2, ovf2);
      raw_sample_idx++;
      last_ccr1_valid = 0U;

      if (raw_sample_idx >= MAX_SAMPLES)
      {
        get_freq_flag = 1;                    // Sample buffer full
        disableTim1CaptureCompareInterrupt(); // Stop interrupts to protect the data
      }
    }
  }
}
#endif
/**
 * @brief Called at task level: process the raw samples in the buffer and update the results
 */
void TIM1_Calculate_Results(void)
{
  if (get_freq_flag == 0)
    return;

  uint64_t sum_freq = 0;
  uint64_t sum_duty = 0;
  uint32_t valid_count = 0;
  uint32_t duty_valid_count = 0;

  uint32_t tim_clk = TIM1_GetCaptureClockHz();
  uint32_t cnt_clk = tim_clk / (htim1.Init.Prescaler + 1U);
  uint64_t arrp1 = (uint64_t)htim1.Init.Period + 1ULL;
  // printf("Timer Clock = %lu Hz, Count Clock = %lu Hz\r\n", tim_clk, cnt_clk);
  for (uint8_t i = 2; i < raw_sample_idx; i++)
  {
    // 1. Compute the period in timer ticks
    uint64_t period_ticks = (uint64_t)capture_buffer[i].cc2 + (uint64_t)capture_buffer[i].ovf2 * arrp1;
    if (period_ticks == 0)
      continue;
    // printf("arr1 = %lu, ovf1 = %lu, arr2 = %lu, ovf2 = %lu\r\n",
    // capture_buffer[i].cc1, capture_buffer[i].ovf1,
    // capture_buffer[i].cc2, capture_buffer[i].ovf2);
    // printf("Sample %u: Period Ticks = %lu\r\n", i, period_ticks);
    //  2. Compute the high-level time in ticks
    uint64_t high_ticks = (uint64_t)capture_buffer[i].cc1 + (uint64_t)capture_buffer[i].ovf1 * arrp1;
    if (high_ticks > period_ticks)
      high_ticks = period_ticks;

    // 3. Compute frequency and duty cycle
    uint32_t f = (uint32_t)((uint64_t)cnt_clk / period_ticks);
    uint32_t d = (uint32_t)((high_ticks * 100ULL + (period_ticks / 2)) / period_ticks);
    // printf("Sample %u: Freq = %lu Hz, Duty = %lu%%\r\n", i, f, d);
    sum_freq += f;
    if (f < DUTY_MEASURE_MAX_HZ)
    {
      sum_duty += d;
      duty_valid_count++;
    }
    valid_count++;
  }

  if (valid_count > 0)
  {
    uwFrequency = (uint32_t)(sum_freq / valid_count);
    if (uwFrequency >= DUTY_MEASURE_MAX_HZ || duty_valid_count == 0U)
    {
      uwDutyCycle = 0U;
    }
    else
    {
      uwDutyCycle = (uint32_t)(sum_duty / duty_valid_count);
    }
  }

  // Reset the sampling state for the next acquisition
  raw_sample_idx = 0;
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
  GPIO_InitTypeDef GPIO_InitStruct;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* TIMx Peripheral clock enable */
  __HAL_RCC_TIM1_CLK_ENABLE();

  /* Enable GPIO channels Clock */
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /* Configure  (TIMx_Channel) in Alternate function, push-pull and 100MHz speed */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*##-2- Configure the NVIC for TIMx #########################################*/
  /* Sets the priority grouping field */
  HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 1);

  /* Enable the TIM1 global Interrupt */
  HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
}
