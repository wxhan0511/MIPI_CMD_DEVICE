//
// Created by 17333 on 25-6-26.
//

#ifndef BSP_MCP4728_H
#define BSP_MCP4728_H

#include <bsp.h>
#include <bsp_i2c.h>
#include "bsp_mcp4728.h" //for da_calibration_data_t

/* Commands and Modes */
#define MCP4728_GENERAL_RESET           0x06
#define MCP4728_GENERAL_WKUP            0x09
#define MCP4728_GENERAL_SOFTWARE_UPDATE 0x08
#define MCP4728_GENERAL_READ_ADDR       0x0C

#define MCP4728_FAST_WRITE              0x00
#define MCP4728_MULTI_WRITE             0x40
#define MCP4728_SEQ_WRITE               0x50
#define MCP4728_SINGLE_WRITE            0x58
#define MCP4728_ADDR_WRITE              0x60
#define MCP4728_VREF_WRITE              0x80
#define MCP4728_GAIN_WRITE              0xC0
#define MCP4728_PWRDOWN_WRITE           0xA0

#define MCP4728_BASE_ADDR               (0x60 << 1) // 7-bit address shifted left

#define MCP4728_GAIN_1                  0x0
#define MCP4728_GAIN_2                  0x1

#define MCP4728_CHANNEL_A               0x0
#define MCP4728_CHANNEL_B               0x1
#define MCP4728_CHANNEL_C               0x2
#define MCP4728_CHANNEL_D               0x3

#define MCP4728_PWRDWN_NORMAL           0x0
#define MCP4728_PWRDWN_1                0x1
#define MCP4728_PWRDWN_2                0x2
#define MCP4728_PWRDWN_3                0x3

#define MCP4728_UDAC_UPLOAD             0x1
#define MCP4728_UDAC_NOLOAD             0x0

extern i2c_dev_t dac;

typedef struct
{
    // VCC校准参数
    float vcc_set_offset;          
    float vcc_set_gain;           
    // ELVDD校准参数
    float elvdd_set_offset;       
    float elvdd_set_gain; 
    
    // ELVSS校准参数
    float elvss_set_offset;         
    float elvss_set_gain;          

    // IOVCC校准参数
    float iovcc_set_offset;         
    float iovcc_set_gain;
    

} __attribute__((packed)) da_calibration_data_t;

typedef struct
{
    i2c_dev_t *i2c_bus;
    uint8_t vref[4];      /* 4-bit reference voltage info: 1=2.048V, 0=VDD */
    uint8_t gain[4];      /* 4-bit gain info: 1=x2, 0=x1 */
    uint8_t pd[4];
    uint16_t val[4];   /* 12-bit numbers specifying outputs for channels A, B, C, D */
}dac_dev_t;

extern dac_dev_t dac_dev;
void bsp_dac_init(dac_dev_t *dev);

BSP_STATUS bsp_dac_single_voltage_set(dac_dev_t* dev, const uint8_t channel, const uint16_t voltage, const uint8_t en);

BSP_STATUS bsp_dac_multi_voltage_set(const dac_dev_t* dev);

BSP_STATUS bsp_bias_output(const float v_bias, const uint16_t pin);


#endif //BSP_MCP4728_H
