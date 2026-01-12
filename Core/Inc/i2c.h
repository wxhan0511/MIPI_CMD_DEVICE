/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    i2c.h
 * @brief   Function prototypes and definitions for i2c.c
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

#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "cmsis_os2.h"      // For osMutexId_t
#include "main.h"
#include <stdbool.h>

/* I2C configuration macros */

#define RA_LP3907_2_ADDRESS      0xC0
#define RA_LP3907_2_MVDD_CMD     0x39
#define RA_LP3907_2_VDDIO_CMD    0x3A



/* I2C handles */
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

/* Function prototypes */

/**
 * @brief Initialize I2C1 peripheral.
 */
void MX_I2C1_Init(void);

/**
 * @brief Initialize I2C2 peripheral.
 */
void MX_I2C2_Init(void);

/* USER CODE BEGIN Prototypes */
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */