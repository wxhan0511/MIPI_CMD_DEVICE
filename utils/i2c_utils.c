
#include "i2c_utils.h"
#include "main.h"
#include <stdbool.h>
#include "i2c.h"
#include "stm32f4xx_hal_gpio_ex.h"

void I2C_RecoverSDA(I2C_HandleTypeDef *hi2c , GPIO_TypeDef* port , uint16_t scl , uint16_t sda) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. 禁用 I2C 外设
    __HAL_I2C_DISABLE(hi2c);

    // 2. 将 SDA 和 SCL 引脚配置为 GPIO 输出模式
    GPIO_InitStruct.Pin = sda | scl; // 假设 SDA: PB8, SCL: PB9，需根据实际引脚调整
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;    // 开漏输出
    GPIO_InitStruct.Pull = GPIO_PULLUP;            // 上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 3. 发送若干 SCL 时钟脉冲以释放 SDA
    for (uint8_t i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(port, scl, GPIO_PIN_SET);   // SCL 高                                        // 短暂延时
        HAL_GPIO_WritePin(port, scl, GPIO_PIN_RESET); // SCL 低
    }

    // 4. 检查 SDA 是否被释放（变为高电平）
    if (HAL_GPIO_ReadPin(port, sda) == GPIO_PIN_SET) {
        // SDA 已恢复
        printf("SDA line recovered successfully.\n");
    } else {
        // SDA 未恢复，可能是硬件问题
        printf("Failed to recover SDA line.\n");
    }

    // 5. 重新配置 SDA 和 SCL 为 I2C 功能
    GPIO_InitStruct.Pin = sda | scl;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;        // 复用开漏模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    if(hi2c == &hi2c1) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1; // 根据实际 I2C 外设调整
    } else if(hi2c == &hi2c2) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2; // 根据实际 I2C 外设调整
    }
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 6. 重新启用 I2C 外设
    __HAL_I2C_ENABLE(hi2c);

    // 7. 可选：复位 I2C 外设
    HAL_I2C_DeInit(hi2c);
    HAL_I2C_Init(hi2c);
}

bool I2C_IsSDALow(I2C_HandleTypeDef *hi2c , GPIO_TypeDef* port , uint16_t scl , uint16_t sda) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    bool isLow = false;

    // 1. 禁用 I2C 外设以避免干扰
    __HAL_I2C_DISABLE(hi2c);

    // 2. 将 SDA 引脚配置为输入模式
    GPIO_InitStruct.Pin = sda; // 假设 SDA 为 PB8，需根据实际引脚调整
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // 输入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;     // 上拉
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 3. 读取 SDA 引脚状态
    if (HAL_GPIO_ReadPin(port, sda) == GPIO_PIN_RESET) {
        isLow = true; // SDA 被拉低
        //printf("SDA is low.\n");
    } else {
        //printf("SDA is high.\n");
    }

    // 4. 恢复 SDA 引脚为 I2C 功能
    GPIO_InitStruct.Pin = sda;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;    // 复用开漏模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    if(hi2c == &hi2c1) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1; // 根据实际 I2C 外设调整
    } else if(hi2c == &hi2c2) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2; // 根据实际 I2C 外设调整
    }
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 5. 重新启用 I2C 外设
    __HAL_I2C_ENABLE(hi2c);

    return isLow;
}


bool I2C_IsSCLLow(I2C_HandleTypeDef *hi2c , GPIO_TypeDef* port , uint16_t scl , uint16_t sda) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    bool isLow = false;

    // 1. 禁用 I2C 外设以避免干扰
    __HAL_I2C_DISABLE(hi2c);

    // 2. 将 SCL 引脚配置为输入模式
    GPIO_InitStruct.Pin = scl; // 假设 SCL 为 PB9，需根据实际引脚调整
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // 输入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;     // 上拉
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 3. 读取 SCL 引脚状态
    if (HAL_GPIO_ReadPin(port, scl) == GPIO_PIN_RESET) {
        isLow = true; // SCL 被拉低
        //printf("SCL is low.\n");
    } else {
        //printf("SCL is high.\n");
    }

    // 4. 恢复 SCL 引脚为 I2C 功能
    GPIO_InitStruct.Pin = scl;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;    // 复用开漏模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    if(hi2c == &hi2c1) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1; // 根据实际 I2C 外设调整
    } else if(hi2c == &hi2c2) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2; // 根据实际 I2C 外设调整
    }
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 5. 重新启用 I2C 外设
    __HAL_I2C_ENABLE(hi2c);

    return isLow;
}

void I2C_RecoverSCL(I2C_HandleTypeDef *hi2c , GPIO_TypeDef* port , uint16_t scl , uint16_t sda) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. 禁用 I2C 外设
    __HAL_I2C_DISABLE(hi2c);

    // 2. 将 SCL 和 SDA 引脚配置为 GPIO 输出模式
    GPIO_InitStruct.Pin = scl | sda; // 假设 SCL: PB9, SDA: PB8，需根据实际引脚调整
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;    // 开漏输出
    GPIO_InitStruct.Pull = GPIO_PULLUP;            // 上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 3. 尝试释放 SCL（通过切换电平）
    for (uint8_t i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(port, scl, GPIO_PIN_SET);   // SCL 高                                       // 短暂延时
        HAL_GPIO_WritePin(port, scl, GPIO_PIN_RESET); // SCL 低
    }

    // 4. 检查 SCL 是否被释放（变为高电平）
    GPIO_InitStruct.Pin = scl;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // 切换为输入模式以检测
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    if (HAL_GPIO_ReadPin(port, scl) == GPIO_PIN_SET) {
        //printf("SCL line recovered successfully.\n");
    } else {
        //printf("Failed to recover SCL line.\n");
    }

    // 5. 恢复 SCL 和 SDA 为 I2C 功能
    GPIO_InitStruct.Pin = scl | sda;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;    // 复用开漏模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    if(hi2c == &hi2c1) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1; // 根据实际 I2C 外设调整
    } else if(hi2c == &hi2c2) {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2; // 根据实际 I2C 外设调整
    }
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // 6. 重新启用 I2C 外设
    __HAL_I2C_ENABLE(hi2c);

    // 7. 可选：复位 I2C 外设
    HAL_I2C_DeInit(hi2c);
    HAL_I2C_Init(hi2c);
}
