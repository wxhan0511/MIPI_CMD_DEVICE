//
// Created by 17333 on 25-6-26.
//

#ifndef BSP_MCP4728_H
#define BSP_MCP4728_H

#include <bsp.h>
#include <bsp_i2c.h>
#include "bsp_mcp4728.h" //for da_calibration_data_t
#include "bsp_d_trigger.h"
#include <stdbool.h>

typedef enum
{
    DAC1_ADDR = 0xC0,
    DAC2_ADDR = 0xC2,
    DAC3_ADDR = 0xC4,
    DAC4_ADDR = 0xC6,
    DAC5_ADDR = 0xC8,
} dac_address_t;

typedef enum
{
    DAC_CHIP_1 = 0,
    DAC_CHIP_2,
    DAC_CHIP_3,
    DAC_CHIP_4,
    DAC_CHIP_5,
    DAC_CHIP_MAX
} dac_chip_index_t;

typedef struct
{
    void *handle;
    uint8_t dev_addr[DAC_CHIP_MAX];
} i2c_dev_t;

/* Commands and Modes */
#define MCP4728_GENERAL_RESET 0x06
#define MCP4728_GENERAL_WKUP 0x09
#define MCP4728_GENERAL_SOFTWARE_UPDATE 0x08
#define MCP4728_GENERAL_READ_ADDR 0x0C

#define MCP4728_FAST_WRITE 0x00
#define MCP4728_MULTI_WRITE 0x40
#define MCP4728_SEQ_WRITE 0x50
#define MCP4728_SINGLE_WRITE 0x58
#define MCP4728_ADDR_WRITE 0x60
#define MCP4728_VREF_WRITE 0x80
#define MCP4728_GAIN_WRITE 0xC0
#define MCP4728_PWRDOWN_WRITE 0xA0

#define MCP4728_BASE_ADDR (0x60 << 1) // 7-bit address shifted left

#define MCP4728_GAIN_1 0x0
#define MCP4728_GAIN_2 0x1

#define MCP4728_CHANNEL_A 0x0
#define MCP4728_CHANNEL_B 0x1
#define MCP4728_CHANNEL_C 0x2
#define MCP4728_CHANNEL_D 0x3

#define MCP4728_PWRDWN_NORMAL 0x0
#define MCP4728_PWRDWN_1 0x1
#define MCP4728_PWRDWN_2 0x2
#define MCP4728_PWRDWN_3 0x3

#define MCP4728_UDAC_UPLOAD 0x1
#define MCP4728_UDAC_NOLOAD 0x0

extern i2c_dev_t dac;
// 设备实例化
typedef struct
{
    uint8_t i2c_dev_address;
    GPIO_TypeDef *ldac_port;
    uint16_t ldac_pin;
} dev_mcp4728_t;

extern dev_mcp4728_t dac_1;
extern dev_mcp4728_t dac_2;
extern dev_mcp4728_t dac_3;
extern dev_mcp4728_t dac_4;
extern dev_mcp4728_t dac_5;

typedef struct
{
    i2c_dev_t *i2c_bus;
    dac_chip_index_t chip_index;
    uint8_t vref[4]; /* 4-bit reference voltage info: 1=2.048V, 0=VDD */
    uint8_t gain[4]; /* 4-bit gain info: 1=x2, 0=x1 */
    uint8_t pd[4];
    uint16_t val[4]; /* 12-bit numbers specifying outputs for channels A, B, C, D */
} dac_dev_t;

extern dac_dev_t dac_chips[DAC_CHIP_MAX];

BSP_STATUS bsp_dac_single_voltage_set(dac_dev_t *dev, const uint8_t channel, const uint16_t voltage, const uint8_t en);

BSP_STATUS bsp_dac_multi_voltage_set(const dac_dev_t *dev);

BSP_STATUS bsp_bias_output(const float v_bias, const uint16_t pin);

void bsp_mcp4728_init(dev_mcp4728_t *dev);
void bsp_mcp4728_change_address(dev_mcp4728_t *dev, uint8_t dev_address);
uint8_t bsp_mcp4728_read_address(dev_mcp4728_t *dev);
bool bsp_mcp4728_is_ready(const dac_dev_t *dev);
BSP_STATUS bsp_mcp4728_wait_ready(const dac_dev_t *dev, uint32_t timeout_ms);
void I2C_CTRL_init(void);
#endif // BSP_MCP4728_H
