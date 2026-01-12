#ifndef __I2C_UTILS_H
#define __I2C_UTILS_H

#include "stm32f4xx_hal.h"
#include "stm32f407xx.h"
#include <stdbool.h>

void I2C_RecoverSDA(I2C_HandleTypeDef *hi2c , GPIO_TypeDef* port , uint16_t scl , uint16_t sda);
bool I2C_IsSDALow(I2C_HandleTypeDef *hi2c , GPIO_TypeDef* port , uint16_t scl , uint16_t sda);
bool I2C_IsSCLLow(I2C_HandleTypeDef *hi2c , GPIO_TypeDef* port , uint16_t scl , uint16_t sda);
void I2C_RecoverSCL(I2C_HandleTypeDef *hi2c , GPIO_TypeDef* port , uint16_t scl , uint16_t sda);

#endif