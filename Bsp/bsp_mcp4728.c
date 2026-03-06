/* Includes ------------------------------------------------------------------*/
#include "bsp_mcp4728.h"
#include "bsp_mcp4728_ctl.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <tgmath.h>
#include "main.h"
#include "bsp_dwt.h"
#include "bsp_i2c_gpio.h"

/*Static function declaration-------------------------------------- */

static void I2C_CTRL_init(void);
static void DAC_gpio_init(void);

/* User-defined variables -----------------------------------------------------*/
typedef enum
{
    MCP4728_CMD_GENERATE_RESET = 0x06,
    MCP4728_CMD_GENERATE_WAKE_UP = 0x09,
    MCP4728_CMD_GENERATE_SW_UPDATE = 0x08,
    MCP4728_CMD_GENERATE_READ_ADDR_BITS = 0x0c,
    MCP4728_CMD_WRITE_I2C_ADDR = 0x61,
} MCP4728_CMD;

/* User-defined function implementation -------------------------------------- */
/**
 * @brief 设置DAC单路输出电压
 * @param dev DAC设备结构体指针
 * @param channel 通道号（0-3）
 * @param voltage 输出电压（单位mV）
 * @param en 0: 立即生效，1: 重启生效
 * @retval BSP_OK 成功，BSP_ERROR 失败
 * @note 通过I2C发送3字节命令设置指定通道电压
 */
BSP_STATUS bsp_dac_single_voltage_set(dac_dev_t *dev, const uint8_t channel, const uint16_t voltage, const uint8_t en)
{
    uint8_t buf[3];
    dev->val[channel] = voltage;
    buf[0] = MCP4728_SINGLE_WRITE | (channel << 1) | en; // Command and channel
    buf[1] = dev->val[channel] >> 8 | dev->vref[channel] << 7 | dev->gain[channel] << 4 | dev->pd[channel] << 5;
    buf[2] = dev->val[channel] & 0xFF; // Lower 8 bits of the 12-bit DAC value
    const HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        dev->i2c_bus->handle, dev->i2c_bus->dev_addr[dev->chip_index], buf, 3, 100);
    if (status != HAL_OK)
    {
        printf("I2C transmit failed %d \r\n", status);
        return BSP_ERROR;
    }
    // 等待 EEPROM 写入周期完成 (RDY/BSY 判断)
    return bsp_mcp4728_wait_ready(dev, 100);
}

/**
 * @brief 一次性设置DAC全部4路输出电压
 * @param dev DAC设备结构体指针
 * @retval BSP_OK 成功，BSP_ERROR 失败
 * @note 依次填充4路通道的命令和数据，通过I2C一次性发送
 */
BSP_STATUS bsp_dac_multi_voltage_set(const dac_dev_t *dev)
{
    uint8_t buf[12];
    uint8_t buf_index = 0;
    for (uint8_t i = 0; i < 4; i++)
    {
        const uint8_t pd = 0;
        buf[buf_index++] = MCP4728_MULTI_WRITE | (i << 1);
        buf[buf_index++] = dev->val[i] >> 8 | dev->vref[i] << 7 | dev->gain[i] << 4 | dev->pd[i] << 5;
        buf[buf_index++] = dev->val[i] & 0xFF; // Lower 8 bits of the 12-bit DAC value
    }

    const HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        dev->i2c_bus->handle, dev->i2c_bus->dev_addr[dev->chip_index], buf, buf_index, 1000);
    if (status != HAL_OK)
    {
        printf("I2C transmit failed %d \r\n", status);
        return BSP_ERROR;
    }
    // Multi Write 通常不写 EEPROM，响应较快，但为了安全也可以增加 ready 判断
    return bsp_mcp4728_wait_ready(dev, 10);
}

// ANCHOR - MCP4728 Address Read
uint8_t bsp_mcp4728_read_address(dev_mcp4728_t *dev){
    uint8_t rt = rt = 1;
    uint8_t address = 0;
    HAL_GPIO_WritePin(dev->ldac_port,dev->ldac_pin,GPIO_PIN_SET);   //LDAC_ON

    i2c_Start();
    i2c_SendByte(0x00);//������ַ
    rt = i2c_WaitAck();
    i2c_SendByte(0x0C);

    HAL_GPIO_WritePin(dev->ldac_port,dev->ldac_pin,GPIO_PIN_RESET);

    rt = i2c_WaitAck();
		i2c_Start();
    i2c_SendByte(0xC0|0x01);
    rt = i2c_WaitAck();
    address = i2c_ReadByte();
    i2c_Stop();
    address = ((address >> 4) & 0x0E) | 0xC0; //�õ���ַ��Ϣ
    return address;
}

// ANCHOR - MCP4728 Address Change
// 由于 MCP4728 的地址写入命令（01100 + A2 A1 A0）具有极其特殊的硬件时序要求：必须在发送命令字节的第 8 个 SCL 时钟周期为高电平期间，将 LDAC 引脚从高拉低。
void bsp_mcp4728_change_address(dev_mcp4728_t *dev, uint8_t dev_address)
{
    uint8_t current_address_bits = (dev->i2c_dev_address >> 1) & 0x07;
    uint8_t cmd = MCP4728_CMD_WRITE_I2C_ADDR | (current_address_bits << 2);
    uint8_t ack;

    I2C_CTRL_init();

    uint8_t addr = bsp_mcp4728_read_address(dev);

    HAL_GPIO_WritePin(dac_1.ldac_port, dac_1.ldac_pin, GPIO_PIN_SET); // LDAC_ON
    HAL_GPIO_WritePin(dac_2.ldac_port, dac_2.ldac_pin, GPIO_PIN_SET); // LDAC_ON
    HAL_GPIO_WritePin(dac_3.ldac_port, dac_3.ldac_pin, GPIO_PIN_SET); // LDAC_ON
    HAL_GPIO_WritePin(dac_4.ldac_port, dac_4.ldac_pin, GPIO_PIN_SET); // LDAC_ON
    HAL_GPIO_WritePin(dac_5.ldac_port, dac_5.ldac_pin, GPIO_PIN_SET); // LDAC_ON

    i2c_Start();
    i2c_SendByte(addr);
    if (i2c_WaitAck() != 0)
    {
        printf("NACK1\r\n");
        i2c_Stop();
    }

    i2c_SendByte(((addr & 0x0e) << 1) | 0x61);

    HAL_GPIO_WritePin(dev->ldac_port, dev->ldac_pin, GPIO_PIN_RESET); // LDAC_OFF
    if (i2c_WaitAck() != 0)
    {
        printf("NACK2\r\n");
        i2c_Stop();
    }
    i2c_SendByte(((dev_address & 0x0e) << 1) | 0x62);
    if (i2c_WaitAck() != 0)
    {
        printf("NACK3\r\n");
        i2c_Stop();
    }
    i2c_SendByte(((dev_address & 0x0e) << 1) | 0x63);
    if (i2c_WaitAck() != 0)
    {
        printf("NACK4\r\n");
        i2c_Stop();
    }
    i2c_Stop();

    bsp_delay_ms(100);
}

/**
 * @brief 检查 MCP4728 是否就绪（EEPROM 写入完成）
 * @param dev DAC设备结构体指针
 * @return bool true: 就绪, false: 繁忙
 */
bool bsp_mcp4728_is_ready(const dac_dev_t *dev)
{
    uint8_t status_byte;
    // 使用 I2C 读操作获取状态。MCP4728 读回的第一字节最高位即 RDY/BSY
    // 1 = Ready (Not Busy), 0 = Busy (Writing EEPROM)
    if (HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->i2c_bus->handle,
                               dev->i2c_bus->dev_addr[dev->chip_index] | 0x01,
                               &status_byte, 1, 10) == HAL_OK)
    {
        return (status_byte & 0x80) != 0;
    }
    return false;
}

/**
 * @brief 等待 MCP4728 就绪
 * @param dev DAC设备结构体指针
 * @param timeout_ms 超时时间
 * @retval BSP_OK 成功, BSP_ERROR 超时或失败
 */
BSP_STATUS bsp_mcp4728_wait_ready(const dac_dev_t *dev, uint32_t timeout_ms)
{
    uint32_t tickstart = HAL_GetTick();
    while (!bsp_mcp4728_is_ready(dev))
    {
        if ((HAL_GetTick() - tickstart) > timeout_ms)
        {
            return BSP_ERROR;
        }
        bsp_delay_ms(1); // 避免死循环占用过多 CPU
    }
    return BSP_OK;
}

/*
*********************************************************************************************************
*	函 数 名: I2C_CTRL_init
*	功能说明: 板载芯片I2C通讯初始化
*********************************************************************************************************
*/
static void I2C_CTRL_init(void)
{
    bsp_InitI2C();

    DAC_gpio_init();
    
}

/*
*********************************************************************************************************
*	函 数 名: DAC_gpio_init
*	功能说明: 配置DAC芯片控制管脚
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
static void DAC_gpio_init(void)
{
    uint8_t Power_flash_read[6];
    uint16_t Power_vsn,Power_vsp,Power_vdd;

    GPIO_InitTypeDef gpio_init;

    __HAL_RCC_GPIOA_CLK_ENABLE(); 
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;	         /* 设置推挽输出 */
    gpio_init.Pull = GPIO_NOPULL;			             /* 上下拉电阻不使能 */
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;	       /* GPIO速度等级 */

    gpio_init.Pin = DAC_LDAC1_Pin;	             //OutControl_DAC_LDAC_1
    HAL_GPIO_Init(DAC_LDAC1_GPIO_Port, &gpio_init);
    gpio_init.Pin = DAC_LDAC2_Pin;	           //OutControl_DAC_LDAC_2
    HAL_GPIO_Init(DAC_LDAC2_GPIO_Port, &gpio_init);
    gpio_init.Pin = DAC_LDAC3_Pin;	             //OutControl_DAC_LDAC_3
    HAL_GPIO_Init(DAC_LDAC3_GPIO_Port, &gpio_init);
    gpio_init.Pin = DAC_LDAC4_Pin;	           //OutControl_DAC_LDAC_4
    HAL_GPIO_Init(DAC_LDAC4_GPIO_Port, &gpio_init);
    gpio_init.Pin = DAC_LDAC5_Pin;	             //OutControl_DAC_LDAC_5
    HAL_GPIO_Init(DAC_LDAC5_GPIO_Port, &gpio_init);


    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pin = DAC_BSY1_Pin;	             
    HAL_GPIO_Init(DAC_BSY1_GPIO_Port, &gpio_init);
    gpio_init.Pin = DAC_BSY2_Pin;	             
    HAL_GPIO_Init(DAC_BSY2_GPIO_Port, &gpio_init);
    gpio_init.Pin = DAC_BSY3_Pin;	        
    HAL_GPIO_Init(DAC_BSY3_GPIO_Port, &gpio_init);
    gpio_init.Pin = DAC_BSY4_Pin;                 
    HAL_GPIO_Init(DAC_BSY4_GPIO_Port, &gpio_init);
    gpio_init.Pin = DAC_BSY5_Pin;                 
    HAL_GPIO_Init(DAC_BSY5_GPIO_Port, &gpio_init);

    DAC_LDAC_1_H();                          //拉高DAC_LDAC_1
    DAC_LDAC_2_H();                          //拉高DAC_LDAC_2
    DAC_LDAC_3_H();                          //拉高DAC_LDAC_3
    DAC_LDAC_4_H();                          //拉高DAC_LDAC_4
    DAC_LDAC_5_H();                          //拉高DAC_LDAC_5

}
