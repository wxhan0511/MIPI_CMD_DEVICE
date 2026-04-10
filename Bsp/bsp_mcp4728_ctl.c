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
    {.i2c_bus = &i2c_bus_2, .chip_index = DAC_CHIP_1, .vref = {1,1,1,1}, .gain = {MCP4728_GAIN_1, MCP4728_GAIN_1, MCP4728_GAIN_1, MCP4728_GAIN_1}, .val = {1500, 1500, 1500, 1500}},
    {.i2c_bus = &i2c_bus_2, .chip_index = DAC_CHIP_2, .vref = {1,1,1,1}, .gain = {MCP4728_GAIN_1, MCP4728_GAIN_1, MCP4728_GAIN_1, MCP4728_GAIN_1}, .val = {1500, 1500, 1500, 1500}},
    {.i2c_bus = &i2c_bus_2, .chip_index = DAC_CHIP_3, .vref = {1,1,1,1}, .gain = {MCP4728_GAIN_1, MCP4728_GAIN_1, MCP4728_GAIN_1, MCP4728_GAIN_1}, .val = {1500, 1500, 1500, 1500}},
    {.i2c_bus = &i2c_bus_2, .chip_index = DAC_CHIP_4, .vref = {1,1,1,1}, .gain = {MCP4728_GAIN_1, MCP4728_GAIN_1, MCP4728_GAIN_1, MCP4728_GAIN_1}, .val = {1500, 1500, 1500, 1500}},
    {.i2c_bus = &i2c_bus_2, .chip_index = DAC_CHIP_5, .vref = {1,1,1,1}, .gain = {MCP4728_GAIN_1, MCP4728_GAIN_1, MCP4728_GAIN_1, MCP4728_GAIN_1}, .val = {1500, 1500, 1500, 1500}}};
// 1. 定义映射表
dac_config_table_t dac_config_table[20] = {
    //  上次电压地址                                      | 偏移地址                                               | 增益地址                                             | 芯片       | 名称         | 通道 | 反 | id | res | 使能                    | 禁能
    {&g_calibration_manager.data.vcc_last_voltage,       &g_calibration_manager.data.da_data.vcc_set_offset,      &g_calibration_manager.data.da_data.vcc_set_gain,       DAC_CHIP_1, "VCC",           0,   0,   0,  0,   bsp_power_single_enable, bsp_power_single_disable,"CtrlV+1"},
    {&g_calibration_manager.data.iovcc_last_voltage,     &g_calibration_manager.data.da_data.iovcc_set_offset,    &g_calibration_manager.data.da_data.iovcc_set_gain,     DAC_CHIP_1, "IOVCC",         1,   0,   1,  0,   bsp_power_single_enable, bsp_power_single_disable,"CtrlV+2"},
    {&g_calibration_manager.data.vsp_last_voltage,       &g_calibration_manager.data.da_data.vsp_set_offset,      &g_calibration_manager.data.da_data.vsp_set_gain,       DAC_CHIP_1, "VSP",           2,   0,   2,  0,   bsp_power_single_enable, bsp_power_single_disable,"CtrlV+3"},
    {&g_calibration_manager.data.vsn_last_voltage,       &g_calibration_manager.data.da_data.vsn_set_offset,      &g_calibration_manager.data.da_data.vsn_set_gain,       DAC_CHIP_1, "VSN",           3,   1,   3,  0,   bsp_power_single_enable, bsp_power_single_disable,"CtrlV-1"},
    {&g_calibration_manager.data.vcc_ref_last,           &g_calibration_manager.data.da_data.vcc_ref_offset,      &g_calibration_manager.data.da_data.vcc_ref_gain,       DAC_CHIP_2, "VCC_LimRef",    0,   0,   4,  0,   bsp_power_single_enable, bsp_power_single_disable,"LIMREF1"},
    {&g_calibration_manager.data.iovcc_ref_last,         &g_calibration_manager.data.da_data.iovcc_ref_offset,    &g_calibration_manager.data.da_data.iovcc_ref_gain,     DAC_CHIP_2, "IOVCC_LimRef",  1,   0,   5,  0,   bsp_power_single_enable, bsp_power_single_disable,"LIMREF2"},
    {&g_calibration_manager.data.vsp_ref_last,           &g_calibration_manager.data.da_data.vsp_ref_offset,      &g_calibration_manager.data.da_data.vsp_ref_gain,       DAC_CHIP_2, "VSP_LimRef",    2,   0,   6,  0,   bsp_power_single_enable, bsp_power_single_disable,"LIMREF3"},
    {&g_calibration_manager.data.vsn_ref_last,           &g_calibration_manager.data.da_data.vsn_ref_offset,      &g_calibration_manager.data.da_data.vsn_ref_gain,       DAC_CHIP_2, "VSN_LimRef",    3,   1,   7,  0,   bsp_power_single_enable, bsp_power_single_disable,"LIMREF-1"},
    {&g_calibration_manager.data.avdd_last_voltage,      &g_calibration_manager.data.da_data.avdd_set_offset,     &g_calibration_manager.data.da_data.avdd_set_gain,      DAC_CHIP_3, "AVDD",          0,   0,   8,  0,   bsp_power_single_enable, bsp_power_single_disable,"CtrlV+4"},
    {&g_calibration_manager.data.vdd_last_voltage,       &g_calibration_manager.data.da_data.vdd_set_offset,      &g_calibration_manager.data.da_data.vdd_set_gain,       DAC_CHIP_3, "VDD",           1,   0,   9,  0,   bsp_power_single_enable, bsp_power_single_disable,"CtrlV+5"},
    {&g_calibration_manager.data.elvdd_last_voltage,     &g_calibration_manager.data.da_data.elvdd_set_offset,    &g_calibration_manager.data.da_data.elvdd_set_gain,     DAC_CHIP_3, "ELVDD",         2,   0,  10,  0,   bsp_power_single_enable, bsp_power_single_disable,"CtrlV+6"},
    {&g_calibration_manager.data.elvss_last_voltage,     &g_calibration_manager.data.da_data.elvss_set_offset,    &g_calibration_manager.data.da_data.elvss_set_gain,     DAC_CHIP_3, "ELVSS",         3,   1,  11,  0,   bsp_power_single_enable, bsp_power_single_disable,"CtrlV-2"},
    {&g_calibration_manager.data.avdd_ref_last,          &g_calibration_manager.data.da_data.avdd_ref_offset,     &g_calibration_manager.data.da_data.avdd_ref_gain,      DAC_CHIP_4, "AVDD_LimRef",   0,   0,  12,  0,   bsp_power_single_enable, bsp_power_single_disable,"LIMREF4"},
    {&g_calibration_manager.data.vdd_ref_last,           &g_calibration_manager.data.da_data.vdd_ref_offset,      &g_calibration_manager.data.da_data.vdd_ref_gain,       DAC_CHIP_4, "VDD_LimRef",    1,   0,  13,  0,   bsp_power_single_enable, bsp_power_single_disable,"LIMREF5"},
    {&g_calibration_manager.data.elvdd_ref_last,         &g_calibration_manager.data.da_data.elvdd_ref_offset,    &g_calibration_manager.data.da_data.elvdd_ref_gain,     DAC_CHIP_4, "ELVDD_LimRef",  2,   0,  14,  0,   bsp_power_single_enable, bsp_power_single_disable,"LIMREF6"},
    {&g_calibration_manager.data.elvss_ref_last,         &g_calibration_manager.data.da_data.elvss_ref_offset,    &g_calibration_manager.data.da_data.elvss_ref_gain,     DAC_CHIP_4, "ELVSS_LimRef",  3,   1,  15,  0,   bsp_power_single_enable, bsp_power_single_disable,"LIMREF-2"},
    {&g_calibration_manager.data.v_level_shift_last,     &g_calibration_manager.data.da_data.v_level_shift_offset, &g_calibration_manager.data.da_data.v_level_shift_gain, DAC_CHIP_5, "V_LEVEL_SHIFT", 0,   0,  16,  0,   bsp_power_single_enable, bsp_power_single_disable,"VLSHIFT"},
    {&g_calibration_manager.data.ref_freq_last,          &g_calibration_manager.data.da_data.ref_freq_offset,     &g_calibration_manager.data.da_data.ref_freq_gain,      DAC_CHIP_5, "REF_FREQ",       1,   0,  17,  0,   bsp_power_single_enable, bsp_power_single_disable,"Ctrl_REFFREQ"},
    {&g_calibration_manager.data.vadj_p_last,            &g_calibration_manager.data.da_data.vadj_p_offset,       &g_calibration_manager.data.da_data.vadj_p_gain,        DAC_CHIP_5, "VADJ_P",         2,   0,  18,  0,   bsp_power_single_enable, bsp_power_single_disable,"Ctrl_VADJP"},
    {&g_calibration_manager.data.vadj_n_last,            &g_calibration_manager.data.da_data.vadj_n_offset,       &g_calibration_manager.data.da_data.vadj_n_gain,        DAC_CHIP_5, "VADJ_N",         3,   1,  19,  0,   bsp_power_single_enable, bsp_power_single_disable,"Ctrl_VADJN"},
};
/* Private function prototypes -----------------------------------------------*/
static void DAC_gpio_init(void);

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

    val = (val - *(cfg->offset)) / (*(cfg->gain));
    MIPI_CMD_DEBUG("%s,%s: vi = %.2f mV (vo=%.2f mV, offset=%.2f, gain=%.2f)\r\n",
        cfg->name, cfg->name1, val, *(cfg->last_voltage), *(cfg->offset), *(cfg->gain));

    val = val * 2; // MCP4728默认增益为2，如果校准增益是基于增益=1的，需要除以2进行补偿
    // 最大输出电压 4.096V
    dac_chips[cfg->chip].val[cfg->channel] = float_to_uint16_round(val);
    // dac_chips[cfg->chip].val[cfg->channel] = 1000;
    // bsp_dac_single_voltage_set(&dac_chips[4],0, 2000, 0);//ADJ-  (3000,-18.441)(2000,-14.282)
    // bsp_dac_single_voltage_set(&dac_chips[4],1, 2400, 0);//ADJ+ (3000,5.134)(1000,11.324)(2500,8.526)(2400,9.198)
    // bsp_dac_single_voltage_set(&dac_chips[0],0, 1500, 0);//out+V1  (vi1000 v0=4.966)(1500,4.946) (vi=2000 vo=1.5055)(2500,0.0505)
    // bsp_dac_single_voltage_set(&dac_chips[0],1, 1300, 0);//(1500,2623)(1300,4003)
    // bsp_dac_single_voltage_set(&dac_chips[0],2, 1500, 0);//(1500,2792.5)(1200,5312)
    // bsp_dac_single_voltage_set(&dac_chips[0],3, 2000, 0);//(1500,-3219)(2000,-1235.7)
    // bsp_dac_single_voltage_set(&dac_chips[2],0, 1550, 0); //(1500,4894)(1550,4546)
    // bsp_dac_single_voltage_set(&dac_chips[2],1, 1400, 0);//(1500,2702)(1400,3535.7)
    //bsp_dac_single_voltage_set(&dac_chips[2],2, 1100, 0);//(1500,2637.6)(1100,6000.6)
    // bsp_dac_single_voltage_set(&dac_chips[2],3, 1100, 0);//(1500,-3115.0)(1100,-5796)
    // bsp_dac_single_voltage_set(&dac_chips[4],3, 1500, 0);//(1500,)       
    // MIPI_CMD_DEBUG("%s,%s: last_voltage=%.2f mV, offset=%.2f, gain=%.2f\r\n",
    //     cfg->name, cfg->name1, *(cfg->last_voltage), *(cfg->offset), *(cfg->gain));
    //if (bsp_dac_single_voltage_set(&dac_chips[2], 0, 1000, 0) != BSP_OK)
    // {
    //     MIPI_CMD_DEBUG("%s set voltage failed\r\n", cfg->name);
    // }
    if (bsp_dac_single_voltage_set(&dac_chips[cfg->chip], cfg->channel, dac_chips[cfg->chip].val[cfg->channel], 0) != BSP_OK)
    {
        MIPI_CMD_DEBUG("%s set voltage failed\r\n", cfg->name);
    }
    single_mcp4728_sync_update(power_id);

    
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
    I2C_CTRL_init();
    HAL_I2C_DeInit(&hi2c2);
    HAL_I2C_Init(&hi2c2);
    
    bsp_power_all_disable();
    bsp_limit_current_reset();
    g_calibration_manager.data.vsn_last_voltage = -5205.0f;
    g_calibration_manager.data.vsp_last_voltage = 5600.0f;
    g_calibration_manager.data.vcc_last_voltage = 1800.0f;
    g_calibration_manager.data.iovcc_last_voltage = 1900.0f;
    // 打印恢复电压信息
    MIPI_CMD_INFO("Restore the voltage set last time\r\n");
    ADS1256_DEBUG("VSN : %f mV\r\n", g_calibration_manager.data.vsn_last_voltage);
    ADS1256_DEBUG("ELVSS : %f mV\r\n", g_calibration_manager.data.elvss_last_voltage);
    ADS1256_DEBUG("VCC : %f mV\r\n", g_calibration_manager.data.vcc_last_voltage);
    ADS1256_DEBUG("IOVCC : %f mV\r\n", g_calibration_manager.data.iovcc_last_voltage);
    ADS1256_DEBUG("VSP : %f mV\r\n", g_calibration_manager.data.vsp_last_voltage);
    ADS1256_DEBUG("AVDD : %f mV\r\n", g_calibration_manager.data.avdd_last_voltage);
    ADS1256_DEBUG("VDD : %f mV\r\n", g_calibration_manager.data.vdd_last_voltage);
    ADS1256_DEBUG("ELVDD : %f mV\r\n", g_calibration_manager.data.elvdd_last_voltage);
    ADS1256_DEBUG("-----------------------------------\r\n");
    //---------------------------------------------------------------------------------------------------------
    ADS1256_DEBUG("VCC_LimCur : %f mA\r\n", g_calibration_manager.data.vcc_ref_last);
    ADS1256_DEBUG("IOVCC_LimCur : %f mA\r\n", g_calibration_manager.data.iovcc_ref_last);
    ADS1256_DEBUG("VSP_LimCur : %f mA\r\n", g_calibration_manager.data.vsp_ref_last);
    ADS1256_DEBUG("VSN_LimCur : %f mA\r\n", g_calibration_manager.data.vsn_ref_last);
    ADS1256_DEBUG("AVDD_LimCur : %f mA\r\n", g_calibration_manager.data.avdd_ref_last);
    ADS1256_DEBUG("VDD_LimCur : %f mA\r\n", g_calibration_manager.data.vdd_ref_last);
    ADS1256_DEBUG("ELVSS_LimCur : %f mA\r\n", g_calibration_manager.data.elvss_ref_last);
    ADS1256_DEBUG("ELVDD_LimCur : %f mA\r\n", g_calibration_manager.data.elvdd_ref_last);
    ADS1256_DEBUG("-----------------------------------\r\n");
    //---------------------------------------------------------------------------------------------------------
    ADS1256_DEBUG("V_LEVEL_SHIFT vi: %f mV\r\n", g_calibration_manager.data.v_level_shift_last);
    ADS1256_DEBUG("VADJ_P: %f mV\r\n", g_calibration_manager.data.vadj_p_last);
    ADS1256_DEBUG("VADJ_N: %f mV\r\n", g_calibration_manager.data.vadj_n_last);
    ADS1256_DEBUG("-----------------------------------\r\n");

    // 循环一次性处理所有通道
    for (int i = 0; i < sizeof(dac_config_table) / sizeof(dac_config_table_t); i++)
    {
        bsp_cali_and_set_power(i);
    }
    HAL_Delay(5000); // 等待DAC输出稳定
    for( uint8_t i =0;i<20;i++)
    {
       bsp_power_single_enable(i);
    }
    
    LEVEL_SHIFT_ENABLE();
    bsp_rly_gear_set(GEAR_uA, VSN_RLY);
    bsp_rly_gear_set(GEAR_uA, ELVSS_RLY);
    bsp_rly_gear_set(GEAR_uA, VCC_RLY);
    bsp_rly_gear_set(GEAR_uA, IOVCC_RLY);
    bsp_rly_gear_set(GEAR_uA, VSP_RLY);
    bsp_rly_gear_set(GEAR_uA, AVDD_RLY);
    bsp_rly_gear_set(GEAR_uA, VDD_RLY);
    bsp_rly_gear_set(GEAR_uA, ELVDD_RLY);
}

void mcp4728_device_init()
{
    bsp_mcp4728_change_address(&dac_1, DAC1_ADDR);
    printf("DAC1 initialized with address 0x%x\r\n", bsp_mcp4728_read_address(&dac_1));
    bsp_mcp4728_change_address(&dac_2, DAC2_ADDR);
    printf("DAC2 initialized with address 0x%x\r\n", bsp_mcp4728_read_address(&dac_2));
    bsp_mcp4728_change_address(&dac_3, DAC3_ADDR);
    printf("DAC3 initialized with address 0x%x\r\n", bsp_mcp4728_read_address(&dac_3));
    bsp_mcp4728_change_address(&dac_4, DAC4_ADDR);
    printf("DAC4 initialized with address 0x%x\r\n", bsp_mcp4728_read_address(&dac_4));
    bsp_mcp4728_change_address(&dac_5, DAC5_ADDR);
    printf("DAC5 initialized with address 0x%x\r\n", bsp_mcp4728_read_address(&dac_5));
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
        bsp_limit_current_reset();
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
        bsp_limit_current_reset();
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
