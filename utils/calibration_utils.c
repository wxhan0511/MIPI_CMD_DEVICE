/**
 * @file       calibration_utils.c
 * @brief      MIPI_CMD
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-10-11
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

/* Includes ------------------------------------------------------------------*/
#include "calibration_utils.h"
#include "main.h"
#include "bsp_calibration.h"
#include "bsp_channel_sel.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/**
 * @brief Convert float to int16 with rounding
 * @param value Float value to convert
 * @return Rounded int16 value
 */
int16_t float_to_int16_round(float value)
{
    if (value >= 0)
    {
        return (int16_t)(value + 0.5f);
    }
    else
    {
        return (int16_t)(value - 0.5f);
    }
}

uint16_t float_to_uint16_round(float value)
{
    if (value >= 0)
    {
        return (uint16_t)(value + 0.5f);
    }
    else
    {
        return (uint16_t)(value - 0.5f);
    }
}

float int32_to_float(int32_t value)
{
    return (float)value;
}

uint8_t float_to_uint8_round(float value)
{
    if (value >= 0)
    {
        return (uint8_t)(value + 0.5f);
    }
    else
    {
        return (uint8_t)(value - 0.5f);
    }
}

/*
 * @brief Select calibration parameters based on main_index and sub_index
 * @param main_index : Channel number (0-7)
 * @param sub_index : d trigger selection sub index (0-7)
 */
/**
 * @brief 根据通道选择校准参数
 * @param main_index 主通道索引 (0-7)
 * @param sub_index  子索引 (0-7)
 * @param offset     输出: 偏移参数指针
 * @param gain       输出: 增益参数指针
 */
void sel_cali_param(uint8_t main_index, uint8_t sub_index, float *offset, float *gain)
{
    // 1. 安全校验 (包含 sub_index 防越界)
    if (main_index > 7 || offset == NULL || gain == NULL)
    {
        if (offset)
            *offset = 0.0f;
        if (gain)
            *gain = 1.0f;
        return;
    }
    if (main_index >= 0 && main_index < 3 && sub_index > 7)
    {
        if (offset)
            *offset = 0.0f;
        if (gain)
            *gain = 1.0f;
        return;
    }
    calibration_data_t *cal = &g_calibration_manager.data;
    TEST_CUR_GEAR gear;
    // 2. 根据通道索引选择参数
    switch (main_index)
    {
    /* --- 非电流通道 (只有一套参数) --- */
    case 0:
        *offset = cal->ad_data.ch0_offset[sub_index];
        *gain = cal->ad_data.ch0_gain[sub_index];
        break;
    case 2:
        *offset = cal->ad_data.ch2_offset[sub_index];
        *gain = cal->ad_data.ch2_gain[sub_index];
        break;
    /* --- ch1: sub_index 为 2, 3, 7 时是电流通道，需单独判断挡位 --- */
    case 1:
        if (sub_index == 2)
        {
            gear = bsp_rly_gear_get(ELVDD_RLY);
            if (gear == GEAR_uA)
            {
                *offset = cal->ad_data.ch1_offset_ua[2];
                *gain = cal->ad_data.ch1_gain_ua[2];
            }
            else
            {
                *offset = cal->ad_data.ch1_offset[2];
                *gain = cal->ad_data.ch1_gain[2];
            }
        }
        else if (sub_index == 3)
        {
            gear = bsp_rly_gear_get(ELVSS_RLY);
            if (gear == GEAR_uA)
            {
                *offset = cal->ad_data.ch1_offset_ua[3];
                *gain = cal->ad_data.ch1_gain_ua[3];
            }
            else
            {
                *offset = cal->ad_data.ch1_offset[3];
                *gain = cal->ad_data.ch1_gain[3];
            }
        }
        else if (sub_index == 7)
        {
            gear = bsp_rly_gear_get(AVDD_RLY);
            if (gear == GEAR_uA)
            {
                *offset = cal->ad_data.ch1_offset_ua[7];
                *gain = cal->ad_data.ch1_gain_ua[7];
            }
            else
            {
                *offset = cal->ad_data.ch1_offset[7];
                *gain = cal->ad_data.ch1_gain[7];
            }
        }
        else
        {
            // 非电流通道，直接读取普通值
            *offset = cal->ad_data.ch1_offset[sub_index];
            *gain = cal->ad_data.ch1_gain[sub_index];
        }
        break;
    /* --- ch3~ch7: 标量结构，只要 main_index 匹配即为电流通道 --- */
    case 3:
        gear = bsp_rly_gear_get(VCC_RLY);
        if (gear == GEAR_uA)
        {
            *offset = cal->ad_data.ch3_offset_ua;
            *gain = cal->ad_data.ch3_gain_ua;
        }
        else
        {
            *offset = cal->ad_data.ch3_offset;
            *gain = cal->ad_data.ch3_gain;
        }
        break;
    case 4:
        gear = bsp_rly_gear_get(IOVCC_RLY);
        if (gear == GEAR_uA)
        {
            *offset = cal->ad_data.ch4_offset_ua;
            *gain = cal->ad_data.ch4_gain_ua;
        }
        else
        {
            *offset = cal->ad_data.ch4_offset;
            *gain = cal->ad_data.ch4_gain;
        }
        break;
    case 5:
        gear = bsp_rly_gear_get(VSP_RLY);
        if (gear == GEAR_uA)
        {
            *offset = cal->ad_data.ch5_offset_ua;
            *gain = cal->ad_data.ch5_gain_ua;
        }
        else
        {
            *offset = cal->ad_data.ch5_offset;
            *gain = cal->ad_data.ch5_gain;
        }
        break;
    case 6:
        gear = bsp_rly_gear_get(VSN_RLY);
        if (gear == GEAR_uA)
        {
            *offset = cal->ad_data.ch6_offset_ua;
            *gain = cal->ad_data.ch6_gain_ua;
        }
        else
        {
            *offset = cal->ad_data.ch6_offset;
            *gain = cal->ad_data.ch6_gain;
        }
        break;
    case 7:
        gear = bsp_rly_gear_get(VDD_RLY);
        if (gear == GEAR_uA)
        {
            *offset = cal->ad_data.ch7_offset_ua;
            *gain = cal->ad_data.ch7_gain_ua;
        }
        else
        {
            *offset = cal->ad_data.ch7_offset;
            *gain = cal->ad_data.ch7_gain;
        }
        break;
    default:
        // 理论上不会走到这里，因为前面已经拦截了 > 7 的情况
        *offset = 0.0f;
        *gain = 1.0f;
        break;
    }
}
