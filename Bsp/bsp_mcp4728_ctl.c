/**
 * @file       bsp_mcp4728_ctl.c
 * @brief
 * @author     wxhan
 * @version    1.0.0
 * @date       2026-01-28
 * @copyright  Copyright (c) 2026 gcoreinc
 * @license    MIT License
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_mcp4728_ctl.h"
#include "bsp_channel_sel.h"
#include "bsp_calibration.h"
#include "bsp_power.h"
#include "bsp_i2c_gpio.h"

#include <stdio.h>
#include <string.h>

#include "calibration_utils.h"

#include "pin_defs.h"
#include "utils.h"
#include "bsp_channel_sel.h"

#include "bsp_mcp4728_ctl.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

dev_mcp4728_t dac_1 = {0xC0, DAC_LDAC1_GPIO_Port, DAC_LDAC1_Pin}; // 默认地址0xC0
dev_mcp4728_t dac_2 = {0xC0, DAC_LDAC2_GPIO_Port, DAC_LDAC2_Pin};
dev_mcp4728_t dac_3 = {0xC0, DAC_LDAC3_GPIO_Port, DAC_LDAC3_Pin};
dev_mcp4728_t dac_4 = {0xC0, DAC_LDAC4_GPIO_Port, DAC_LDAC4_Pin};
dev_mcp4728_t dac_5 = {0xC0, DAC_LDAC5_GPIO_Port, DAC_LDAC5_Pin};
// I2C总线1设备结构体，配置I2C句柄和MCP4728器件地址
i2c_dev_t i2c_bus_1 = {
    .handle = &hi2c1,
    .dev_addr = {DAC1_ADDR, DAC2_ADDR, DAC3_ADDR, DAC4_ADDR, DAC5_ADDR},
};
i2c_dev_t i2c_bus_2 = {
    .handle = &hi2c2,
    .dev_addr = {DAC1_ADDR, DAC2_ADDR, DAC3_ADDR, DAC4_ADDR, DAC5_ADDR},
};

//
dac_dev_t dac_chips[DAC_CHIP_MAX] = {
    {.i2c_bus = &i2c_bus_2, .chip_index = DAC_CHIP_1, .vref = {1}, .gain = {MCP4728_GAIN_2, MCP4728_GAIN_2, MCP4728_GAIN_2, MCP4728_GAIN_2}, .val = {1500, 1500, 1500, 1500}},
    {.i2c_bus = &i2c_bus_2, .chip_index = DAC_CHIP_2, .vref = {1}, .gain = {MCP4728_GAIN_2, MCP4728_GAIN_2, MCP4728_GAIN_2, MCP4728_GAIN_2}, .val = {1500, 1500, 1500, 1500}},
    {.i2c_bus = &i2c_bus_2, .chip_index = DAC_CHIP_3, .vref = {1}, .gain = {MCP4728_GAIN_2, MCP4728_GAIN_2, MCP4728_GAIN_2, MCP4728_GAIN_2}, .val = {1500, 1500, 1500, 1500}},
    {.i2c_bus = &i2c_bus_2, .chip_index = DAC_CHIP_4, .vref = {1}, .gain = {MCP4728_GAIN_2, MCP4728_GAIN_2, MCP4728_GAIN_2, MCP4728_GAIN_2}, .val = {1500, 1500, 1500, 1500}},
    {.i2c_bus = &i2c_bus_2, .chip_index = DAC_CHIP_5, .vref = {1}, .gain = {MCP4728_GAIN_2, MCP4728_GAIN_2, MCP4728_GAIN_2, MCP4728_GAIN_2}, .val = {1500, 1500, 1500, 1500}}};

// 1. 定义映射表
dac_config_table_t dac_config_table[20] = {
    //  上次电压地址                                      | 偏移地址                                               | 增益地址                                             | 芯片       | 名称         | 通道 | 反 | id | res | 使能                    | 禁能
    {&g_calibration_manager.data.vcc_last_voltage,       &g_calibration_manager.data.da_data.vcc_set_offset,      &g_calibration_manager.data.da_data.vcc_set_gain,       DAC_CHIP_1, "VCC",           0,   0,   0,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.iovcc_last_voltage,     &g_calibration_manager.data.da_data.iovcc_set_offset,    &g_calibration_manager.data.da_data.iovcc_set_gain,     DAC_CHIP_1, "IOVCC",         1,   0,   1,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.vsp_last_voltage,       &g_calibration_manager.data.da_data.vsp_set_offset,      &g_calibration_manager.data.da_data.vsp_set_gain,       DAC_CHIP_1, "VSP",           2,   0,   2,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.vsn_last_voltage,       &g_calibration_manager.data.da_data.vsn_set_offset,      &g_calibration_manager.data.da_data.vsn_set_gain,       DAC_CHIP_1, "VSN",           3,   1,   3,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.vcc_ref_last,           &g_calibration_manager.data.da_data.vcc_ref_offset,      &g_calibration_manager.data.da_data.vcc_ref_gain,       DAC_CHIP_2, "VCC_LimRef",    0,   0,   4,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.iovcc_ref_last,         &g_calibration_manager.data.da_data.iovcc_ref_offset,    &g_calibration_manager.data.da_data.iovcc_ref_gain,     DAC_CHIP_2, "IOVCC_LimRef",  1,   0,   5,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.vsp_ref_last,           &g_calibration_manager.data.da_data.vsp_ref_offset,      &g_calibration_manager.data.da_data.vsp_ref_gain,       DAC_CHIP_2, "VSP_LimRef",    2,   0,   6,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.vsn_ref_last,           &g_calibration_manager.data.da_data.vsn_ref_offset,      &g_calibration_manager.data.da_data.vsn_ref_gain,       DAC_CHIP_2, "VSN_LimRef",    3,   1,   7,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.avdd_last_voltage,      &g_calibration_manager.data.da_data.avdd_set_offset,     &g_calibration_manager.data.da_data.avdd_set_gain,      DAC_CHIP_3, "AVDD",          0,   0,   8,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.vdd_last_voltage,       &g_calibration_manager.data.da_data.vdd_set_offset,      &g_calibration_manager.data.da_data.vdd_set_gain,       DAC_CHIP_3, "VDD",           1,   0,   9,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.elvdd_last_voltage,     &g_calibration_manager.data.da_data.elvdd_set_offset,    &g_calibration_manager.data.da_data.elvdd_set_gain,     DAC_CHIP_3, "ELVDD",         2,   0,  10,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.elvss_last_voltage,     &g_calibration_manager.data.da_data.elvss_set_offset,    &g_calibration_manager.data.da_data.elvss_set_gain,     DAC_CHIP_3, "ELVSS",         3,   1,  11,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.avdd_ref_last,          &g_calibration_manager.data.da_data.avdd_ref_offset,     &g_calibration_manager.data.da_data.avdd_ref_gain,      DAC_CHIP_4, "AVDD_LimRef",   0,   0,  12,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.vdd_ref_last,           &g_calibration_manager.data.da_data.vdd_ref_offset,      &g_calibration_manager.data.da_data.vdd_ref_gain,       DAC_CHIP_4, "VDD_LimRef",    1,   0,  13,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.elvdd_ref_last,         &g_calibration_manager.data.da_data.elvdd_ref_offset,    &g_calibration_manager.data.da_data.elvdd_ref_gain,     DAC_CHIP_4, "ELVDD_LimRef",  2,   0,  14,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.elvss_ref_last,         &g_calibration_manager.data.da_data.elvss_ref_offset,    &g_calibration_manager.data.da_data.elvss_ref_gain,     DAC_CHIP_4, "ELVSS_LimRef",  3,   1,  15,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.v_level_shift_last,     &g_calibration_manager.data.da_data.v_level_shift_offset, &g_calibration_manager.data.da_data.v_level_shift_gain, DAC_CHIP_5, "V_LEVEL_SHIFT", 0,   0,  16,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.ref_freq_last,          &g_calibration_manager.data.da_data.ref_freq_offset,     &g_calibration_manager.data.da_data.ref_freq_gain,      DAC_CHIP_5, "REF_FREQ",       1,   0,  17,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.vadj_p_last,            &g_calibration_manager.data.da_data.vadj_p_offset,       &g_calibration_manager.data.da_data.vadj_p_gain,        DAC_CHIP_5, "VADJ_P",         2,   0,  18,  0,   bsp_power_single_enable, bsp_power_single_disable},
    {&g_calibration_manager.data.vadj_n_last,            &g_calibration_manager.data.da_data.vadj_n_offset,       &g_calibration_manager.data.da_data.vadj_n_gain,        DAC_CHIP_5, "VADJ_N",         3,   1,  19,  0,   bsp_power_single_enable, bsp_power_single_disable},
};
/* Private function prototypes -----------------------------------------------*/
static void DAC_gpio_init(void);
static void I2C_CTRL_init(void);
/* Private functions ---------------------------------------------------------*/

void bsp_power_all_disable()
{
    VSN_DISABLE_POWEREN_N_1();
    ELVSS_DISABLE_POWEREN_N_2();
    VCC_DISABLE_POWEREN_P_1();
    IOVCC_DISABLE_POWEREN_P_2();
    VSP_DISABLE_POWEREN_P_3();
    AVDD_DISABLE_POWEREN_P_4();
    VDD_DISABLE_POWEREN_P_5();
    ELVDD_DISABLE_POWEREN_P_6();
    // ADJ+ ADJ- no need to enable
}
void bsp_power_all_enable()
{
    VSN_ENABLE_POWEREN_N_1();
    ELVSS_ENABLE_POWEREN_N_2();
    VCC_ENABLE_POWEREN_P_1();
    IOVCC_ENABLE_POWEREN_P_2();
    VSP_ENABLE_POWEREN_P_3();
    AVDD_ENABLE_POWEREN_P_4();
    VDD_ENABLE_POWEREN_P_5();
    ELVDD_ENABLE_POWEREN_P_6();
    // ADJ+ ADJ- no need to disable
}

void bsp_cali_and_set_power(uint8_t power_id)
{
    bsp_power_single_disable(power_id);

    dac_config_table_t *cfg = &dac_config_table[power_id];
    float val = *(cfg->last_voltage);

    // 如果是 ELVSS 等负压，根据你的逻辑进行取反处理
    if (cfg->inverse)
    {
        val = (-val + *(cfg->offset)) / (*(cfg->gain));
    }
    else
    {
        val = (val - *(cfg->offset)) / (*(cfg->gain));
    }

    val = val / 2; // MCP4728默认增益为2，如果校准增益是基于增益=1的，需要除以2进行补偿
    // 最大输出电压 4.096V
    dac_chips[cfg->chip].val[cfg->channel] = float_to_uint16_round(val);

    if (bsp_dac_single_voltage_set(&dac_chips[cfg->chip], cfg->channel, dac_chips[cfg->chip].val[cfg->channel], 0) != BSP_OK)
    {
        MIPI_CMD_DEBUG("%s set voltage failed\r\n", cfg->name);
    }
    single_mcp4728_sync_update(power_id);

    bsp_power_single_enable(power_id);
}
void all_mcp4728_sync_update()
{
    DAC_LDAC_1_L();
    DAC_LDAC_2_L();
    DAC_LDAC_3_L();
    DAC_LDAC_4_L();
    DAC_LDAC_5_L();
}
void single_mcp4728_sync_update(uint8_t power_id)
{
    // 非法值直接返回
    if (power_id > 19) return;

    // 按范围直接判断，减少冗余case
    if (power_id < 4)      DAC_LDAC_1_L();
    else if (power_id < 8) DAC_LDAC_2_L();
    else if (power_id < 12) DAC_LDAC_3_L();
    else if (power_id < 16) DAC_LDAC_4_L();
    else                   DAC_LDAC_5_L(); // 16-19
}
void bsp_dac_init()
{
    mcp4728_device_init(); // Initialize 5 DAC addresses
    bsp_power_all_disable();
    bsp_limit_current_set(disabled);

    // 打印恢复电压信息
    MIPI_CMD_INFO("Restore the voltage set last time\r\n");
    MIPI_CMD_DEBUG("VSN : %f mV\r\n", g_calibration_manager.data.vsn_last_voltage);
    MIPI_CMD_DEBUG("ELVSS : %f mV\r\n", g_calibration_manager.data.elvss_last_voltage);
    MIPI_CMD_DEBUG("VCC : %f mV\r\n", g_calibration_manager.data.vcc_last_voltage);
    MIPI_CMD_DEBUG("IOVCC : %f mV\r\n", g_calibration_manager.data.iovcc_last_voltage);
    MIPI_CMD_DEBUG("VSP : %f mV\r\n", g_calibration_manager.data.vsp_last_voltage);
    MIPI_CMD_DEBUG("AVDD : %f mV\r\n", g_calibration_manager.data.avdd_last_voltage);
    MIPI_CMD_DEBUG("VDD : %f mV\r\n", g_calibration_manager.data.vdd_last_voltage);
    MIPI_CMD_DEBUG("ELVDD : %f mV\r\n", g_calibration_manager.data.elvdd_last_voltage);
    MIPI_CMD_DEBUG("-----------------------------------\r\n");
    //---------------------------------------------------------------------------------------------------------
    MIPI_CMD_DEBUG("VCC_LimCur : %f mA\r\n", g_calibration_manager.data.vcc_ref_last);
    MIPI_CMD_DEBUG("IOVCC_LimCur : %f mA\r\n", g_calibration_manager.data.iovcc_ref_last);
    MIPI_CMD_DEBUG("VSP_LimCur : %f mA\r\n", g_calibration_manager.data.vsp_ref_last);
    MIPI_CMD_DEBUG("VSN_LimCur : %f mA\r\n", g_calibration_manager.data.vsn_ref_last);
    MIPI_CMD_DEBUG("AVDD_LimCur : %f mA\r\n", g_calibration_manager.data.avdd_ref_last);
    MIPI_CMD_DEBUG("VDD_LimCur : %f mA\r\n", g_calibration_manager.data.vdd_ref_last);
    MIPI_CMD_DEBUG("ELVSS_LimCur : %f mA\r\n", g_calibration_manager.data.elvss_ref_last);
    MIPI_CMD_DEBUG("ELVDD_LimCur : %f mA\r\n", g_calibration_manager.data.elvdd_ref_last);
    MIPI_CMD_DEBUG("-----------------------------------\r\n");
    //---------------------------------------------------------------------------------------------------------
    MIPI_CMD_DEBUG("V_LEVEL_SHIFT vi: %f mV\r\n", g_calibration_manager.data.v_level_shift_last);
    MIPI_CMD_DEBUG("VADJ_P: %f mV\r\n", g_calibration_manager.data.vadj_p_last);
    MIPI_CMD_DEBUG("VADJ_N: %f mV\r\n", g_calibration_manager.data.vadj_n_last);
    MIPI_CMD_DEBUG("-----------------------------------\r\n");

    // 循环一次性处理所有通道
    for (int i = 0; i < sizeof(dac_config_table) / sizeof(dac_config_table_t); i++)
    {
        bsp_cali_and_set_power(i);
    }
    LEVEL_SHIFT_ENABLE();
    bsp_limit_current_set(enabled);
    bsp_rly_gear_set(GEAR_mA, VSN_RLY);
    bsp_rly_gear_set(GEAR_mA, ELVSS_RLY);
    bsp_rly_gear_set(GEAR_mA, VCC_RLY);
    bsp_rly_gear_set(GEAR_mA, IOVCC_RLY);
    bsp_rly_gear_set(GEAR_mA, VSP_RLY);
    bsp_rly_gear_set(GEAR_mA, AVDD_RLY);
    bsp_rly_gear_set(GEAR_mA, VDD_RLY);
    bsp_rly_gear_set(GEAR_mA, ELVDD_RLY);
}

void mcp4728_device_init()
{
    bsp_mcp4728_change_address(&dac_1, DAC1_ADDR);
    printf("DAC1 initialized with address 0x%x\r\n", DAC1_ADDR);
    bsp_mcp4728_change_address(&dac_2, DAC2_ADDR);
    printf("DAC2 initialized with address 0x%x\r\n", DAC2_ADDR);
    bsp_mcp4728_change_address(&dac_3, DAC3_ADDR);
    printf("DAC3 initialized with address 0x%x\r\n", DAC3_ADDR);
    bsp_mcp4728_change_address(&dac_4, DAC4_ADDR);
    printf("DAC4 initialized with address 0x%x\r\n", DAC4_ADDR);
    bsp_mcp4728_change_address(&dac_5, DAC5_ADDR);
    printf("DAC5 initialized with address 0x%x\r\n", DAC5_ADDR);
}

void bsp_power_single_enable(uint8_t power_id)
{
    switch (power_id)
    {
    case 0:
        VCC_ENABLE_POWEREN_P_1();
        break;
    case 1:
        IOVCC_ENABLE_POWEREN_P_2();
        break;
    case 2:
        VSP_ENABLE_POWEREN_P_3();
        break;
    case 3:
        VSN_ENABLE_POWEREN_N_1();
        break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 12:
    case 13:
    case 14:
    case 15:
        bsp_limit_current_set(ENABLE);
        break;
    case 8:
        AVDD_ENABLE_POWEREN_P_4();
        break;
    case 9:
        VDD_ENABLE_POWEREN_P_5();
        break;
    case 10:
        ELVDD_ENABLE_POWEREN_P_6();
        break;
    case 11:
        ELVSS_ENABLE_POWEREN_N_2();
        break;
    case 16:
        LEVEL_SHIFT_ENABLE();
        break;
    default:
        MIPI_CMD_DEBUG("other power_id: %d\r\n no need to handle", power_id);
        break;
    }
}

void bsp_power_single_disable(uint8_t power_id)
{
    switch (power_id)
    {
    case 0:
        VCC_DISABLE_POWEREN_P_1();
        break;
    case 1:
        IOVCC_DISABLE_POWEREN_P_2();
        break;
    case 2:
        VSP_DISABLE_POWEREN_P_3();
        break;
    case 3:
        VSN_DISABLE_POWEREN_N_1();
        break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 12:
    case 13:
    case 14:
    case 15:
        bsp_limit_current_set(DISABLE);
        break;
    case 8:
        AVDD_DISABLE_POWEREN_P_4();
        break;
    case 9:
        VDD_DISABLE_POWEREN_P_5();
        break;
    case 10:
        ELVDD_DISABLE_POWEREN_P_6();
        break;
    case 11:
        ELVSS_DISABLE_POWEREN_N_2();
        break;
    case 16:
        LEVEL_SHIFT_DISABLE();
        break;
    default:
        MIPI_CMD_DEBUG("other power_id: %d\r\n no need to handle", power_id);
        break;
    }
}
void test_mcp4728()
{
    mcp4728_device_init();

    bsp_dac_init();
}
