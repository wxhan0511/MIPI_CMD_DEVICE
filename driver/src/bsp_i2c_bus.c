#include "bsp_i2c_bus.h"
#include "bsp.h"
#include "main.h"
#include "i2c_utils.h"
#include "i2c.h"

BSP_STATUS_T bsp_i2c_bus_hw_write_data(void *handle, uint8_t address, uint8_t command, const uint8_t *data, uint32_t size)
{
    uint8_t send_data[1 + size];
    HAL_StatusTypeDef status = HAL_OK;

    send_data[0] = command;

    for (uint8_t i = 0; i < (1 + size); i++)
    {
        send_data[1 + i] = data[i];
    }
    status = HAL_I2C_Master_Transmit_IT((I2C_HandleTypeDef *)handle, (address) | 0x00, send_data, size + 1);
    while (HAL_I2C_GetState((I2C_HandleTypeDef *)handle) != HAL_I2C_STATE_READY)
        ;
    if (status == HAL_OK)
    {
        return BSP_OK;
    }
    else
    {
        printf("i2c2 error\r\n");
        return BSP_ERROR;
    }
}

BSP_STATUS_T bsp_i2c_bus_hw_read_data(void *handle, uint8_t address, uint8_t command, uint8_t *data, uint32_t size)
{
    uint8_t send_data[2];
    HAL_StatusTypeDef status = HAL_OK;

    send_data[0] = command;

    status = HAL_I2C_Master_Transmit(handle, address | 0x00, send_data, 1, 1000);
#if 0
    if (status == HAL_OK)
		{
		}   
    else
    {
        printf("T fail\r\n");
        HAL_I2C_DeInit(handle);
        HAL_I2C_Init(handle);
        if (I2C_IsSDALow(handle, GPIOB, GPIO_PIN_6, GPIO_PIN_7))
        {
            // 如果 SDA 被拉低，调用恢复函数
            I2C_RecoverSDA(handle, GPIOB, GPIO_PIN_6, GPIO_PIN_7);
        }
        if (I2C_IsSCLLow(handle, GPIOB, GPIO_PIN_6, GPIO_PIN_7))
        {
            // 如果 SCL 被拉低，调用恢复函数
            I2C_RecoverSCL(handle, GPIOB, GPIO_PIN_6, GPIO_PIN_7);
        }
        __disable_irq();
        MX_I2C2_Init();
        __enable_irq();
    }
#endif
    while (HAL_I2C_GetState((I2C_HandleTypeDef *)handle) != HAL_I2C_STATE_READY)
        ;
    // osDelay(1);
    if (status != HAL_OK)	return BSP_ERROR;

    status = HAL_I2C_Master_Receive(handle, address | 0x01, data, size, 1000);
#if 0
    if (status == HAL_OK)
    {
    }
    else
    {
        printf("R fail\r\n");
        HAL_I2C_DeInit(handle);
        HAL_I2C_Init(handle);
        if (I2C_IsSDALow(handle, GPIOB, GPIO_PIN_6, GPIO_PIN_7))
        {
            // 如果 SDA 被拉低，调用恢复函数
            I2C_RecoverSDA(handle, GPIOB, GPIO_PIN_6, GPIO_PIN_7);
        }
        if (I2C_IsSCLLow(handle, GPIOB, GPIO_PIN_6, GPIO_PIN_7))
        {
            // 如果 SCL 被拉低，调用恢复函数
            I2C_RecoverSCL(handle, GPIOB, GPIO_PIN_6, GPIO_PIN_7);
        }
        __disable_irq();
        MX_I2C2_Init();
        __enable_irq();
    }
#endif
    // if(status != HAL_OK)
    while (HAL_I2C_GetState((I2C_HandleTypeDef *)handle) != HAL_I2C_STATE_READY)
        ;

    if (status == HAL_OK)
        return BSP_OK;
    else
        return BSP_ERROR;
}

bsp_i2c_hw_t bsp_i2c_hw_ra = {
    .handle = &hi2c2,
    .read_data = bsp_i2c_bus_hw_read_data,
    .write_data = bsp_i2c_bus_hw_write_data,
};