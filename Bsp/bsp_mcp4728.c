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
 * @brief Set the output voltage of a single DAC channel
 * @param dev Pointer to the DAC device struct
 * @param channel Channel number (0-3)
 * @param voltage Output voltage (in mV)
 * @param en 0: take effect immediately, 1: take effect after restart
 * @retval BSP_OK on success, BSP_ERROR on failure
 * @note Sends a 3-byte command over I2C to set the voltage of the given channel
 * If dev->gain[channel] is 1, the output range is doubled (4.096V); if 0, max is 2.048V
 */
BSP_STATUS bsp_dac_single_voltage_set(dac_dev_t *dev, const uint8_t channel, const uint16_t voltage, const uint8_t en)
{
    uint8_t buf[3];
    dev->val[channel] = voltage;
    buf[0] = MCP4728_SINGLE_WRITE | (channel << 1) | en; // Command and channel
    buf[1] = dev->val[channel] >> 8 | dev->vref[channel] << 7 | dev->gain[channel] << 4 | dev->pd[channel] << 5;
    buf[2] = dev->val[channel] & 0xFF; // Lower 8 bits of the 12-bit DAC value
    // ADS1256_DEBUG("chip_i2c_addr: 0x%02X, channel: %d, DAC_voltage: %d\r\n", dev->i2c_bus->dev_addr[dev->chip_index], channel, voltage);
    const HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        dev->i2c_bus->handle, dev->i2c_bus->dev_addr[dev->chip_index], buf, 3, 1000);
    if (status != HAL_OK)
    {
        ADS1256_DEBUG("I2C transmit failed %d \r\n", status);
        return BSP_ERROR;
    }
    // Wait for the EEPROM write cycle to complete (RDY/BSY check)
    return bsp_mcp4728_wait_ready(dev, 100);
}

/**
 * @brief Set all 4 DAC output voltages at once
 * @param dev Pointer to the DAC device struct
 * @retval BSP_OK on success, BSP_ERROR on failure
 * @note Fills the command and data for all 4 channels, then sends them over I2C in one transaction
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
        ADS1256_DEBUG("I2C transmit failed %d \r\n", status);
        return BSP_ERROR;
    }
    // Multi Write usually does not write to EEPROM, so the response is fast, but a ready check is added for safety
    return bsp_mcp4728_wait_ready(dev, 10);
}

// ANCHOR - MCP4728 Address Read
uint8_t bsp_mcp4728_read_address(dev_mcp4728_t *dev){
    uint8_t rt = rt = 1;
    uint8_t address = 0;
    HAL_GPIO_WritePin(dev->ldac_port,dev->ldac_pin,GPIO_PIN_SET);   //LDAC_ON

    i2c_Start();
    i2c_SendByte(0x00);// Send command address
    rt = i2c_WaitAck();
    i2c_SendByte(0x0C);

    HAL_GPIO_WritePin(dev->ldac_port,dev->ldac_pin,GPIO_PIN_RESET);

    rt = i2c_WaitAck();
		i2c_Start();
    i2c_SendByte(0xC0|0x01);
    rt = i2c_WaitAck();
    address = i2c_ReadByte();
    i2c_Stop();
    address = ((address >> 4) & 0x0E) | 0xC0; // Extract the address bits
    return address;
}

// ANCHOR - MCP4728 Address Change
// The MCP4728 address write command (01100 + A2 A1 A0) has extremely special hardware timing requirements: LDAC must be pulled from high to low while SCL is high during the 8th clock cycle of the command byte.
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
 * @brief Check whether the MCP4728 is ready (EEPROM write completed)
 * @param dev Pointer to the DAC device struct
 * @return bool true: ready, false: busy
 */
bool bsp_mcp4728_is_ready(const dac_dev_t *dev)
{
    uint8_t status_byte;
    // Use an I2C read to get the status. The MSB of the first byte read back from MCP4728 is RDY/BSY
    // 1 = Ready (not busy), 0 = Busy (writing EEPROM)
    if (HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->i2c_bus->handle,
                               dev->i2c_bus->dev_addr[dev->chip_index] | 0x01,
                               &status_byte, 1, 10) == HAL_OK)
    {
        return (status_byte & 0x80) != 0;
    }
    return false;
}

/**
 * @brief Wait until the MCP4728 is ready
 * @param dev Pointer to the DAC device struct
 * @param timeout_ms Timeout in milliseconds
 * @retval BSP_OK on success, BSP_ERROR on timeout or failure
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
        bsp_delay_ms(1); // Avoid a busy loop hogging too much CPU
    }
    return BSP_OK;
}

/*
*********************************************************************************************************
*	Function  : I2C_CTRL_init
*	Description: Initialize I2C communication for the on-board chips
*********************************************************************************************************
*/
void I2C_CTRL_init(void)
{
    bsp_InitI2C();

    DAC_gpio_init();
    
}

/*
*********************************************************************************************************
*	Function  : DAC_gpio_init
*	Description: Configure the DAC chip control pins
*	Parameter  : none
*	Return     : none
*********************************************************************************************************
*/
static void DAC_gpio_init(void)
{
    GPIO_InitTypeDef gpio_init;

    __HAL_RCC_GPIOA_CLK_ENABLE(); 
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;	         /* Push-pull output */
    gpio_init.Pull = GPIO_NOPULL;			             /* No pull-up/pull-down */
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;	       /* GPIO speed level */

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

    DAC_LDAC_1_H();                          // Set DAC_LDAC_1 high
    DAC_LDAC_2_H();                          // Set DAC_LDAC_2 high
    DAC_LDAC_3_H();                          // Set DAC_LDAC_3 high
    DAC_LDAC_4_H();                          // Set DAC_LDAC_4 high
    DAC_LDAC_5_H();                          // Set DAC_LDAC_5 high

}
