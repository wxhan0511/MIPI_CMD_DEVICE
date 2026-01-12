//
// Created by 薛斌 on 24-8-21.
//

#include "bsp_mcp4728.h"

#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include "main.h"
#include "bsp_dwt.h"
#include "bsp_calibration.h"
#include "bsp_power.h"
// I2C总线1设备结构体，配置I2C句柄和MCP4728器件地址
i2c_dev_t i2c_bus_1 = {
    .handle = &hi2c1,
    .dev_addr = {P11_FLASH_ADDR, P2_FLASH_ADDR, P3_FLASH_ADDR, 0xc6},
};

// MCP4728 DAC设备结构体，包含I2C总线、掉电、参考电压、增益、初始值等参数
dac_dev_t dac_dev = {
    .i2c_bus = &i2c_bus_1,
    .pd = {0,0,0,0},// 掉电模式（Power-Down），每个通道独立设置，0=正常工作，1/2/3=不同掉电电阻
    .vref = {0x01,0x01,0x01,0x01},//vref: 1=外部VDD，0=内部2.048V基准
    .gain = {MCP4728_GAIN_2,MCP4728_GAIN_2,MCP4728_GAIN_2,MCP4728_GAIN_2},
	//OUT:  ELVDD+=18-5.733V,  ELVSS-=-(18-5.733Vi), 待定      ,Ilim_DA+*5/6A
	//IN:   ADJ_ELVDD+,        ADJ_ELVSS,            ADJ_VBAT,Ilim_DA+
    .val = {1500,1500,1500,1500}
};

/**
 * @brief DAC initialization, configure default voltage for 4 channels
 * @param dev Pointer to the DAC device structure
 * @note Set the default output voltage for 4 channels during initialization and pull the LDAC pin low to enable output
 */
void bsp_dac_init(dac_dev_t *dev)
{
    //恢复上次设置的电压
    BSP_STATUS status;
    VDD_DISABLE();
    VSN_DISABLE();
    ELVSS_DISABLE();
    ELVDD_DISABLE();
    VBAT_DISABLE();
    VCC_DISABLE();
    IOVCC_DISABLE();


    RA_POWEREX_INFO("Restore the voltage set last time\r\n");
    RA_POWEREX_INFO("ELVDD: %f mV\r\n",  g_calibration_manager.data.elvdd_last_voltage);
    RA_POWEREX_INFO("ELVSS: %f mV\r\n",  g_calibration_manager.data.elvss_last_voltage);
    RA_POWEREX_INFO("IOVCC: %f mV\r\n",  g_calibration_manager.data.iovcc_last_voltage);
    RA_POWEREX_INFO("VCC: %f mV\r\n",  g_calibration_manager.data.vcc_last_voltage);
    // 校准数据
    g_calibration_manager.data.elvdd_last_voltage = (g_calibration_manager.data.elvdd_last_voltage - g_calibration_manager.data.da_data.elvdd_set_offset) / (g_calibration_manager.data.da_data.elvdd_set_gain);
    dac_dev.val[0] = float_to_uint16_round(g_calibration_manager.data.elvdd_last_voltage);
    status = bsp_dac_single_voltage_set(&dac_dev, 0, dac_dev.val[0], 0);
    if (status != BSP_OK)
    {
        CDC_DEBUG("ELVDD set voltage failed\r\n");
    }
    g_calibration_manager.data.elvss_last_voltage = (-g_calibration_manager.data.elvss_last_voltage + g_calibration_manager.data.da_data.elvss_set_offset) / (g_calibration_manager.data.da_data.elvss_set_gain);
    CDC_DEBUG("ELVSS vi %f\r\n",g_calibration_manager.data.elvss_last_voltage);
    dac_dev.val[1] = float_to_uint16_round(g_calibration_manager.data.elvss_last_voltage);
    CDC_DEBUG("ELVSS vi %d\r\n",dac_dev.val[1]);
    status = bsp_dac_single_voltage_set(&dac_dev, 1, dac_dev.val[1], 0);
    if (status != BSP_OK)
    {
        CDC_DEBUG("ELVSS set voltage failed\r\n");
    }
    g_calibration_manager.data.iovcc_last_voltage = (g_calibration_manager.data.iovcc_last_voltage - g_calibration_manager.data.da_data.iovcc_set_offset) / (g_calibration_manager.data.da_data.iovcc_set_gain);
    dac_dev.val[3] = float_to_uint16_round(g_calibration_manager.data.iovcc_last_voltage);
    status = bsp_dac_single_voltage_set(&dac_dev, 3, dac_dev.val[3], 0);
    if (status != BSP_OK)
    {
        CDC_DEBUG("IOVCC set voltage failed\r\n");
    }
    g_calibration_manager.data.vcc_last_voltage = (g_calibration_manager.data.vcc_last_voltage - g_calibration_manager.data.da_data.vcc_set_offset) / (g_calibration_manager.data.da_data.vcc_set_gain);
    dac_dev.val[2] = float_to_uint16_round(g_calibration_manager.data.vcc_last_voltage);
    status = bsp_dac_single_voltage_set(&dac_dev, 2, dac_dev.val[2], 0);
    if (status != BSP_OK)
    {
        CDC_DEBUG("VCC set voltage failed\r\n");
    }
    //HAL_GPIO_WritePin(LDAC_Port, LDAC_Pin, GPIO_PIN_RESET);
   
    ELVDD_ENABLE();
    ELVSS_ENABLE();
    IOVCC_ENABLE();
    VCC_ENABLE();

    // dac_dev.val[0] = 1200; // Set default voltage for ELVDD+  7000 先不上电，cs hign 配7V
    // dac_dev.val[1] = 1500; // Set default voltage for ELVSS-    0
    // dac_dev.val[2] = -127; // Set default voltage for VCC    2700
    // dac_dev.val[3] = 1675; // Set default voltage for IOVCC  1800
    // bsp_dac_multi_voltage_set(&dac_dev); // 同时更新通道0-3的输出
    // HAL_GPIO_WritePin(LDAC_Port, LDAC_Pin, GPIO_PIN_RESET);
    // RA_POWEREX_INFO("DAC initialized , all voltage input set 1500mv\r\n");
}

/**
 * @brief 设置DAC单路输出电压
 * @param dev DAC设备结构体指针
 * @param channel 通道号（0-3）
 * @param voltage 输出电压（单位mV）
 * @param en 0: 立即生效，1: 重启生效
 * @retval BSP_OK 成功，BSP_ERROR 失败
 * @note 通过I2C发送3字节命令设置指定通道电压
 */
BSP_STATUS bsp_dac_single_voltage_set(dac_dev_t* dev, const uint8_t channel, const uint16_t voltage, const uint8_t en)
{
    uint8_t buf[3];
    dev->val[channel] = voltage;
    buf[0] = MCP4728_SINGLE_WRITE | (channel << 1) | en; // Command and channel
    buf[1] = dev->val[channel] >> 8 | dev->vref[channel] << 7 | dev->gain[channel] << 4 | dev->pd[channel] << 5;
    buf[2] = dev->val[channel] & 0xFF; // Lower 8 bits of the 12-bit DAC value
    const HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        dev->i2c_bus->handle, dev->i2c_bus->dev_addr[P11_FLASH], buf, 3,100);
        if (status != HAL_OK)
    {
        printf("I2C transmit failed %d \r\n", status);
        return BSP_ERROR;
    }
    bsp_delay_ms(10);
    return BSP_OK;
}

/**
 * @brief 一次性设置DAC全部4路输出电压
 * @param dev DAC设备结构体指针
 * @retval BSP_OK 成功，BSP_ERROR 失败
 * @note 依次填充4路通道的命令和数据，通过I2C一次性发送
 */
BSP_STATUS bsp_dac_multi_voltage_set(const dac_dev_t* dev) {
    uint8_t buf[12];
    uint8_t buf_index = 0;
    for (uint8_t i = 0; i < 4; i++) {
        const uint8_t pd = 0;
        buf[buf_index++] = MCP4728_MULTI_WRITE | (i << 1);
        buf[buf_index++] = dev->val[i] >> 8 | dev->vref[i] << 7 | dev->gain[i] << 4 | dev->pd[i] << 5;
        buf[buf_index++] = dev->val[i] & 0xFF; // Lower 8 bits of the 12-bit DAC value
    }

    const HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        dev->i2c_bus->handle, dev->i2c_bus->dev_addr[P11_FLASH], buf, buf_index, 1000);
    if (status != HAL_OK)
    {
        printf("I2C transmit failed %d \r\n", status);
        return BSP_ERROR;
    }
    bsp_delay_ms(10);
    return BSP_OK;
}
