/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.h
  * @brief   This file contains all the function prototypes for
  *          the tim.c file
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
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */
void TIM1_PWM_Init(uint16_t arr, uint16_t  psc, uint16_t pulse);
void TIM2_PWM_Init(uint16_t arr, uint16_t  psc, uint16_t pulse);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
void TIM1_Generate_N_Pulses(uint16_t num_pulses);
void TIM1_CCP_Init(void);
void enableTim1CaptureCompareInterrupt(void);
void disableTim1CaptureCompareInterrupt(void);
void enableTim1PWMOutput(void);
void disableTim1PWMOutput(void);
void enableTim2PWMOutput(void);
void disableTim2PWMOutput(void);
void app_delay(uint32_t delay_ms);
/* USER CODE BEGIN EFP */
/* USER CODE BEGIN Prototypes */
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern __IO uint32_t            uwIC2Value;
/* Duty Cycle Value */
extern __IO uint32_t            uwDutyCycle;
/* Frequency Value */
extern __IO uint32_t            uwFrequency;
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */

