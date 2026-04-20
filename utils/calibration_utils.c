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
int16_t float_to_int16_round(float value) {
    if (value >= 0) {
        return (int16_t)(value + 0.5f);
    } else {
        return (int16_t)(value - 0.5f);
    }
}

uint16_t float_to_uint16_round(float value) {
    if (value >= 0) {
        return (uint16_t)(value + 0.5f);
    } else {
        return (uint16_t)(value - 0.5f);
    }
}

float int32_to_float(int32_t value) {
    return (float)value;
}

uint8_t float_to_uint8_round(float value) {
    if (value >= 0) {
        return (uint8_t)(value + 0.5f);
    } else {
        return (uint8_t)(value - 0.5f);
    }
}


/*
  * @brief Select calibration parameters based on main_index and sub_index
  * @param main_index : Channel number (0-7)
  * @param sub_index : d trigger selection sub index (0-7)
*/
void sel_cali_param(uint8_t main_index,uint8_t sub_index, float *offset, float *gain)
{
  if (main_index > 7 || sub_index > 7 || offset == NULL || gain == NULL)
  {
    *offset = 0.0f;
    *gain = 1.0f;
  }

  calibration_data_t *cal = &g_calibration_manager.data;
  *offset= main_index == 0 ? cal->ad_data.ch0_offset[sub_index] :
          main_index == 1 ? cal->ad_data.ch1_offset[sub_index] :
          main_index == 2 ? cal->ad_data.ch2_offset[sub_index] :
          main_index == 3 ? cal->ad_data.ch3_offset:
            main_index == 4 ? cal->ad_data.ch4_offset:
            main_index == 5 ? cal->ad_data.ch5_offset:
          main_index == 6 ? cal->ad_data.ch6_offset:
                            cal->ad_data.ch7_offset;

  *gain = main_index == 0 ? cal->ad_data.ch0_gain[sub_index] :
         main_index == 1 ? cal->ad_data.ch1_gain[sub_index] :
         main_index == 2 ? cal->ad_data.ch2_gain[sub_index] :
         main_index == 3 ? cal->ad_data.ch3_gain:
         main_index == 4 ? cal->ad_data.ch4_gain:
         main_index == 5 ? cal->ad_data.ch5_gain:
         main_index == 6 ? cal->ad_data.ch6_gain:
                           cal->ad_data.ch7_gain;

}
