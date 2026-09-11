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

/**
 * @brief Select calibration parameters based on channel
 * @param main_index Main channel index (0-7)
 * @param sub_index  Sub index (0-7)
 * @param offset     Output: pointer to the offset parameter
 * @param gain       Output: pointer to the gain parameter
 */
void sel_cali_param(uint8_t main_index, uint8_t sub_index, float *offset, float *gain)
{
    // 1. Safety checks (main_index and sub_index range, output pointers)
    if (main_index > 7 || sub_index > 7 || offset == NULL || gain == NULL)
    {
        if (offset)
            *offset = 0.0f;
        if (gain)
            *gain = 1.0f;
        return;
    }
    calibration_data_t *cal = &g_calibration_manager.data;
    TEST_CUR_GEAR gear;
    int16_t last_vol; // Holds the last sampled voltage
    // 2. Select parameters based on channel index
    switch (main_index)
    {
    /* --- Non-current channels (single parameter set) --- */
    case 0:
        *offset = cal->ad_data.ch0_offset[sub_index];
        *gain = cal->ad_data.ch0_gain[sub_index];
        break;
    case 2:
        *offset = cal->ad_data.ch2_offset[sub_index];
        *gain = cal->ad_data.ch2_gain[sub_index];
        break;
    /* --- ch1: sub_index 2, 3 and 7 are current channels, gear must be checked separately --- */
    case 1:
        if (sub_index == 2)
        {
            gear = bsp_rly_get_gear_isr(ELVDD_RLY);
            if (gear == GEAR_uA)
            {
                last_vol = bsp_rly_get_last_voltage_isr(ELVDD_RLY);
                if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
                {
                    *offset = cal->ad_data.ch1_offset_ua1[2];
                    *gain = cal->ad_data.ch1_gain_ua1[2];
                }
                else
                {
                    *offset = cal->ad_data.ch1_offset_ua[2];
                    *gain = cal->ad_data.ch1_gain_ua[2];
                }
            }
            else
            {
                *offset = cal->ad_data.ch1_offset[2];
                *gain = cal->ad_data.ch1_gain[2];
            }
        }
        else if (sub_index == 3)
        {
            gear = bsp_rly_get_gear_isr(ELVSS_RLY);
            if (gear == GEAR_uA)
            {
                last_vol = bsp_rly_get_last_voltage_isr(ELVSS_RLY);
                if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
                {
                    *offset = cal->ad_data.ch1_offset_ua1[3];
                    *gain = cal->ad_data.ch1_gain_ua1[3];
                }
                else
                {
                    *offset = cal->ad_data.ch1_offset_ua[3];
                    *gain = cal->ad_data.ch1_gain_ua[3];
                }
            }
            else
            {
                *offset = cal->ad_data.ch1_offset[3];
                *gain = cal->ad_data.ch1_gain[3];
            }
        }
        else if (sub_index == 7)
        {
            gear = bsp_rly_get_gear_isr(AVDD_RLY);

            if (gear == GEAR_uA)
            {
                last_vol = bsp_rly_get_last_voltage_isr(AVDD_RLY);
                if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
                {
                    *offset = cal->ad_data.ch1_offset_ua1[7];
                    *gain = cal->ad_data.ch1_gain_ua1[7];
                }
                else
                {
                    *offset = cal->ad_data.ch1_offset_ua[7];
                    *gain = cal->ad_data.ch1_gain_ua[7];
                }
            }
            else
            {
                *offset = cal->ad_data.ch1_offset[7];
                *gain = cal->ad_data.ch1_gain[7];
            }
        }
        else
        {
            // Non-current channel, read the normal values directly
            *offset = cal->ad_data.ch1_offset[sub_index];
            *gain = cal->ad_data.ch1_gain[sub_index];
        }
        break;
    /* --- ch3~ch7: scalar members; any matching main_index is a current channel --- */
    case 3:
        gear = bsp_rly_get_gear_isr(VCC_RLY);
        if (gear == GEAR_uA)
        {
            last_vol = bsp_rly_get_last_voltage_isr(VCC_RLY);
            if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
            {
                *offset = cal->ad_data.ch3_offset_ua1;
                *gain = cal->ad_data.ch3_gain_ua1;
            }
            else
            {
                *offset = cal->ad_data.ch3_offset_ua;
                *gain = cal->ad_data.ch3_gain_ua;
            }
        }
        else
        {
            *offset = cal->ad_data.ch3_offset;
            *gain = cal->ad_data.ch3_gain;
        }
        break;
    case 4:
        gear = bsp_rly_get_gear_isr(IOVCC_RLY);
        if (gear == GEAR_uA)
        {
            last_vol = bsp_rly_get_last_voltage_isr(IOVCC_RLY);
            if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
            {
                *offset = cal->ad_data.ch4_offset_ua1;
                *gain = cal->ad_data.ch4_gain_ua1;
            }
            else
            {
                *offset = cal->ad_data.ch4_offset_ua;
                *gain = cal->ad_data.ch4_gain_ua;
            }
        }
        else
        {
            *offset = cal->ad_data.ch4_offset;
            *gain = cal->ad_data.ch4_gain;
        }
        break;
    case 5:
        gear = bsp_rly_get_gear_isr(VSP_RLY);
        if (gear == GEAR_uA)
        {
            last_vol = bsp_rly_get_last_voltage_isr(VSP_RLY);
            if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
            {
                *offset = cal->ad_data.ch5_offset_ua1;
                *gain = cal->ad_data.ch5_gain_ua1;
            }
            else
            {
                *offset = cal->ad_data.ch5_offset_ua;
                *gain = cal->ad_data.ch5_gain_ua;
            }
        }
        else
        {
            *offset = cal->ad_data.ch5_offset;
            *gain = cal->ad_data.ch5_gain;
        }
        break;
    case 6:
        gear = bsp_rly_get_gear_isr(VSN_RLY);
        if (gear == GEAR_uA)
        {
            last_vol = bsp_rly_get_last_voltage_isr(VSN_RLY);
            if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
            {
                *offset = cal->ad_data.ch6_offset_ua1;
                *gain = cal->ad_data.ch6_gain_ua1;
            }
            else
            {
                *offset = cal->ad_data.ch6_offset_ua;
                *gain = cal->ad_data.ch6_gain_ua;
            }
        }
        else
        {
            *offset = cal->ad_data.ch6_offset;
            *gain = cal->ad_data.ch6_gain;
        }
        break;
    case 7:
        gear = bsp_rly_get_gear_isr(VDD_RLY);
        if (gear == GEAR_uA)
        {
            last_vol = bsp_rly_get_last_voltage_isr(VDD_RLY);
            if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
            {
                *offset = cal->ad_data.ch7_offset_ua1;
                *gain = cal->ad_data.ch7_gain_ua1;
            }
            else
            {
                *offset = cal->ad_data.ch7_offset_ua;
                *gain = cal->ad_data.ch7_gain_ua;
            }
        }
        else
        {
            *offset = cal->ad_data.ch7_offset;
            *gain = cal->ad_data.ch7_gain;
        }
        break;
    default:
        // Should never be reached: main_index > 7 is rejected above
        *offset = 0.0f;
        *gain = 1.0f;
        break;
    }
}

/**
 * @brief Write calibration parameters into the global g_calibration_manager.data structure
 * @param main_index Main channel index (0-7)
 * @param sub_index  Sub channel index
 * @param offset     Offset value to write
 * @param gain       Gain value to write
 */
void set_cali_param(uint8_t main_index, uint8_t sub_index, float offset, float gain)
{
    // 1. Safety check
    if (main_index > 7 || sub_index > 7)
    {
        return; // Invalid channel, ignore
    }
    calibration_data_t *cal = &g_calibration_manager.data;
    TEST_CUR_GEAR gear;
    int16_t last_vol;
    // 2. Select the target by channel index and write
    switch (main_index)
    {
    /* --- Non-current channels (single parameter set) --- */
    case 0:
        cal->ad_data.ch0_offset[sub_index] = offset;
        cal->ad_data.ch0_gain[sub_index] = gain;
        break;
    case 2:
        cal->ad_data.ch2_offset[sub_index] = offset;
        cal->ad_data.ch2_gain[sub_index] = gain;
        break;
    /* --- ch1: sub_index 2, 3 and 7 are current channels, gear must be checked separately --- */
    case 1:
        if (sub_index == 2)
        {
            gear = bsp_rly_get_gear_isr(ELVDD_RLY);
            if (gear == GEAR_uA)
            {
                last_vol = bsp_rly_get_last_voltage_isr(ELVDD_RLY);
                if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
                {
                    cal->ad_data.ch1_offset_ua1[2] = offset;
                    cal->ad_data.ch1_gain_ua1[2] = gain;
                }
                else
                {
                    cal->ad_data.ch1_offset_ua[2] = offset;
                    cal->ad_data.ch1_gain_ua[2] = gain;
                }
            }
            else
            {
                cal->ad_data.ch1_offset[2] = offset;
                cal->ad_data.ch1_gain[2] = gain;
            }
        }
        else if (sub_index == 3)
        {
            gear = bsp_rly_get_gear_isr(ELVSS_RLY);
            if (gear == GEAR_uA)
            {
                last_vol = bsp_rly_get_last_voltage_isr(ELVSS_RLY);
                if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
                {
                    cal->ad_data.ch1_offset_ua1[3] = offset;
                    cal->ad_data.ch1_gain_ua1[3] = gain;
                }
                else
                {
                    cal->ad_data.ch1_offset_ua[3] = offset;
                    cal->ad_data.ch1_gain_ua[3] = gain;
                }
            }
            else
            {
                cal->ad_data.ch1_offset[3] = offset;
                cal->ad_data.ch1_gain[3] = gain;
            }
        }
        else if (sub_index == 7)
        {
            gear = bsp_rly_get_gear_isr(AVDD_RLY);
            if (gear == GEAR_uA)
            {
                last_vol = bsp_rly_get_last_voltage_isr(AVDD_RLY);
                if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
                {
                    cal->ad_data.ch1_offset_ua1[7] = offset;
                    cal->ad_data.ch1_gain_ua1[7] = gain;
                }
                else
                {
                    cal->ad_data.ch1_offset_ua[7] = offset;
                    cal->ad_data.ch1_gain_ua[7] = gain;
                }
            }
            else
            {
                cal->ad_data.ch1_offset[7] = offset;
                cal->ad_data.ch1_gain[7] = gain;
            }
        }
        else
        {
            // Non-current channel, write the normal values directly
            cal->ad_data.ch1_offset[sub_index] = offset;
            cal->ad_data.ch1_gain[sub_index] = gain;
        }
        break;
    /* --- ch3~ch7: scalar members; any matching main_index is a current channel --- */
    case 3:
        gear = bsp_rly_get_gear_isr(VCC_RLY);
        if (gear == GEAR_uA)
        {
            last_vol = bsp_rly_get_last_voltage_isr(VCC_RLY);
            if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
            {
                cal->ad_data.ch3_offset_ua1 = offset;
                cal->ad_data.ch3_gain_ua1 = gain;
            }
            else
            {
                cal->ad_data.ch3_offset_ua = offset;
                cal->ad_data.ch3_gain_ua = gain;
            }
        }
        else
        {
            cal->ad_data.ch3_offset = offset;
            cal->ad_data.ch3_gain = gain;
        }
        break;
    case 4:
        gear = bsp_rly_get_gear_isr(IOVCC_RLY);
        if (gear == GEAR_uA)
        {
            last_vol = bsp_rly_get_last_voltage_isr(IOVCC_RLY);
            if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
            {
                cal->ad_data.ch4_offset_ua1 = offset;
                cal->ad_data.ch4_gain_ua1 = gain;
            }
            else
            {
                cal->ad_data.ch4_offset_ua = offset;
                cal->ad_data.ch4_gain_ua = gain;
            }
        }
        else
        {
            cal->ad_data.ch4_offset = offset;
            cal->ad_data.ch4_gain = gain;
        }
        break;
    case 5:
        gear = bsp_rly_get_gear_isr(VSP_RLY);
        if (gear == GEAR_uA)
        {
            last_vol = bsp_rly_get_last_voltage_isr(VSP_RLY);
            if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
            {
                cal->ad_data.ch5_offset_ua1 = offset;
                cal->ad_data.ch5_gain_ua1 = gain;
            }
            else
            {
                cal->ad_data.ch5_offset_ua = offset;
                cal->ad_data.ch5_gain_ua = gain;
            }
        }
        else
        {
            cal->ad_data.ch5_offset = offset;
            cal->ad_data.ch5_gain = gain;
        }
        break;
    case 6:
        gear = bsp_rly_get_gear_isr(VSN_RLY);
        if (gear == GEAR_uA)
        {
            last_vol = bsp_rly_get_last_voltage_isr(VSN_RLY);
            if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
            {
                cal->ad_data.ch6_offset_ua1 = offset;
                cal->ad_data.ch6_gain_ua1 = gain;
            }
            else
            {
                cal->ad_data.ch6_offset_ua = offset;
                cal->ad_data.ch6_gain_ua = gain;
            }
        }
        else
        {
            cal->ad_data.ch6_offset = offset;
            cal->ad_data.ch6_gain = gain;
        }
        break;
    case 7:
        gear = bsp_rly_get_gear_isr(VDD_RLY);
        if (gear == GEAR_uA)
        {
            last_vol = bsp_rly_get_last_voltage_isr(VDD_RLY);
            if (last_vol > 2250 || last_vol < -2250) // absolute value greater than 2250
            {
                cal->ad_data.ch7_offset_ua1 = offset;
                cal->ad_data.ch7_gain_ua1 = gain;
            }
            else
            {
                cal->ad_data.ch7_offset_ua = offset;
                cal->ad_data.ch7_gain_ua = gain;
            }
        }
        else
        {
            cal->ad_data.ch7_offset = offset;
            cal->ad_data.ch7_gain = gain;
        }
        break;
    default:
        // Should never be reached
        break;
    }
}
