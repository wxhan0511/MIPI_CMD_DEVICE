
#include "i2c_utils.h"
#include "main.h"
#include <stdbool.h>
#include "i2c.h"
#include "stm32f4xx_hal_gpio_ex.h"

void I2C_RecoverSDA(I2C_HandleTypeDef *hi2c , GPIO_TypeDef* port , uint16_t scl , uint16_t sda) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. Disable the I2C peripheral
    __HAL_I2C_DISABLE(hi2c);

    // 2. Configure SDA and SCL pins as GPIO outputs
    GPIO_InitStruct.Pin = sda | scl; // assumes SDA: PB8, SCL: PB9; adjust to the actual pins
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;    // open-drain output
    GPIO_InitStruct.Pull = GPIO_PULLUP;            // pull-up
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 3. Generate several SCL clock pulses to release SDA
    for (uint8_t i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(port, scl, GPIO_PIN_SET);   // SCL high
        HAL_GPIO_WritePin(port, scl, GPIO_PIN_RESET); // SCL low
    }

    // 4. Check whether SDA has been released (reads high)
    if (HAL_GPIO_ReadPin(port, sda) == GPIO_PIN_SET) {
        // SDA recovered
        printf("SDA line recovered successfully.\n");
    } else {
        // SDA not recovered, likely a hardware issue
        printf("Failed to recover SDA line.\n");
    }

    // 5. Reconfigure SDA and SCL for I2C alternate function
    GPIO_InitStruct.Pin = sda | scl;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;        // alternate-function open-drain mode
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    if(hi2c == &hi2c1) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1; // adjust for the actual I2C peripheral
    } else if(hi2c == &hi2c2) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2; // adjust for the actual I2C peripheral
    }
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 6. Re-enable the I2C peripheral
    __HAL_I2C_ENABLE(hi2c);

    // 7. Optional: reset the I2C peripheral
    HAL_I2C_DeInit(hi2c);
    HAL_I2C_Init(hi2c);
}

bool I2C_IsSDALow(I2C_HandleTypeDef *hi2c , GPIO_TypeDef* port , uint16_t scl , uint16_t sda) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    bool isLow = false;

    // 1. Disable the I2C peripheral to avoid interference
    __HAL_I2C_DISABLE(hi2c);

    // 2. Configure the SDA pin as input
    GPIO_InitStruct.Pin = sda; // assumes SDA is PB8; adjust to the actual pin
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // input mode
    GPIO_InitStruct.Pull = GPIO_PULLUP;     // pull-up
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 3. Read the SDA pin state
    if (HAL_GPIO_ReadPin(port, sda) == GPIO_PIN_RESET) {
        isLow = true; // SDA is pulled low
        //printf("SDA is low.\n");
    } else {
        //printf("SDA is high.\n");
    }

    // 4. Restore the SDA pin to its I2C function
    GPIO_InitStruct.Pin = sda;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;    // alternate-function open-drain mode
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    if(hi2c == &hi2c1) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1; // adjust for the actual I2C peripheral
    } else if(hi2c == &hi2c2) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2; // adjust for the actual I2C peripheral
    }
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 5. Re-enable the I2C peripheral
    __HAL_I2C_ENABLE(hi2c);

    return isLow;
}


bool I2C_IsSCLLow(I2C_HandleTypeDef *hi2c , GPIO_TypeDef* port , uint16_t scl , uint16_t sda) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    bool isLow = false;

    // 1. Disable the I2C peripheral to avoid interference
    __HAL_I2C_DISABLE(hi2c);

    // 2. Configure the SCL pin as input
    GPIO_InitStruct.Pin = scl; // assumes SCL is PB9; adjust to the actual pin
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // input mode
    GPIO_InitStruct.Pull = GPIO_PULLUP;     // pull-up
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 3. Read the SCL pin state
    if (HAL_GPIO_ReadPin(port, scl) == GPIO_PIN_RESET) {
        isLow = true; // SCL is pulled low
        //printf("SCL is low.\n");
    } else {
        //printf("SCL is high.\n");
    }

    // 4. Restore the SCL pin to its I2C function
    GPIO_InitStruct.Pin = scl;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;    // alternate-function open-drain mode
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    if(hi2c == &hi2c1) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1; // adjust for the actual I2C peripheral
    } else if(hi2c == &hi2c2) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2; // adjust for the actual I2C peripheral
    }
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 5. Re-enable the I2C peripheral
    __HAL_I2C_ENABLE(hi2c);

    return isLow;
}

void I2C_RecoverSCL(I2C_HandleTypeDef *hi2c , GPIO_TypeDef* port , uint16_t scl , uint16_t sda) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. Disable the I2C peripheral
    __HAL_I2C_DISABLE(hi2c);

    // 2. Configure SCL and SDA pins as GPIO outputs
    GPIO_InitStruct.Pin = scl | sda; // assumes SCL: PB9, SDA: PB8; adjust to the actual pins
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;    // open-drain output
    GPIO_InitStruct.Pull = GPIO_PULLUP;            // pull-up
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 3. Try to release SCL (by toggling the level)
    for (uint8_t i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(port, scl, GPIO_PIN_SET);   // SCL high
        HAL_GPIO_WritePin(port, scl, GPIO_PIN_RESET); // SCL low
    }

    // 4. Check whether SCL has been released (reads high)
    GPIO_InitStruct.Pin = scl;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // switch to input mode for detection
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    if (HAL_GPIO_ReadPin(port, scl) == GPIO_PIN_SET) {
        //printf("SCL line recovered successfully.\n");
    } else {
        //printf("Failed to recover SCL line.\n");
    }

    // 5. Restore SCL and SDA to their I2C functions
    GPIO_InitStruct.Pin = scl | sda;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;    // alternate-function open-drain mode
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    if(hi2c == &hi2c1) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1; // adjust for the actual I2C peripheral
    } else if(hi2c == &hi2c2) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2; // adjust for the actual I2C peripheral
    }
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 6. Re-enable the I2C peripheral
    __HAL_I2C_ENABLE(hi2c);

    // 7. Optional: reset the I2C peripheral
    HAL_I2C_DeInit(hi2c);
    HAL_I2C_Init(hi2c);
}
