/**
 * @file       calibration_utils.c
 * @brief      RA_PowerEX
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
  * @brief Select calibration parameters based on channel and type
  * @param ch Channel number (0-7)
  * @param type 0 for set parameters, 1 for read parameters
  * @param power 0 for voltage, 1 for current
  * @param offset Pointer to store the selected offset
  * @param gain Pointer to store the selected gain  
*/
void sel_cali_param(uint8_t ch, uint8_t type, uint8_t power, float *offset, float *gain)
{
  if (ch > 7 || type > 1 || power > 1)
  {
    *offset = 0.0f;
    *gain = 1.0f;
    RA_POWEREX_DEBUG("Invalid channel (%d), type (%d) or power(%d) for calibration selection\r\n", ch, type, power);
    return;
  }

  calibration_data_t *cal = &g_calibration_manager.data;

  if (type == 0)
  { // Set parameters
    switch (ch)
    {
    case 0:
      *offset = cal->ch0_set_v_offset;
      *gain = cal->ch0_set_v_gain;
      break;
    case 1:
      *offset = cal->ch1_set_v_offset;
      *gain = cal->ch1_set_v_gain;
      break;
    case 2:
      *offset = cal->ch2_set_v_offset;
      *gain = cal->ch2_set_v_gain;
      break;
    case 3:
      *offset = cal->ch3_set_v_offset;
      *gain = cal->ch3_set_v_gain;
      break;
    case 4:
      *offset = cal->ch4_set_v_offset;
      *gain = cal->ch4_set_v_gain;
      break;
    case 5:
      *offset = cal->ch5_set_v_offset;
      *gain = cal->ch5_set_v_gain;
      break;
    case 6:
      *offset = cal->ch6_set_v_offset;
      *gain = cal->ch6_set_v_gain;
      break;
    case 7:
      *offset = cal->ch7_set_v_offset;
      *gain = cal->ch7_set_v_gain;
      break;
    default:
      *offset = 0.0f;
      *gain = 1.0f;
      break;
    }
  }
  else
  { // Read parameters
    switch (ch)
    {
    case 0:
      if (power == 0)
      {
        *offset = cal->ch0_read_v_offset;
        *gain = cal->ch0_read_v_gain;
      }
      else
      {
        *offset = cal->ch0_read_c_offset;
        *gain = cal->ch0_read_c_gain;
      }
      break;
    case 1:
      if (power == 0)
      {
        *offset = cal->ch1_read_v_offset;
        *gain = cal->ch1_read_v_gain;
      }
      else
      {
        *offset = cal->ch1_read_c_offset;
        *gain = cal->ch1_read_c_gain;
      }
      break;
    case 2:
      if (power == 0)
      {
        *offset = cal->ch2_read_v_offset;
        *gain = cal->ch2_read_v_gain;
      }
      else
      {
        *offset = cal->ch2_read_c_offset;
        *gain = cal->ch2_read_c_gain;
      }
      break;
    case 3:
      if (power == 0)
      {
        *offset = cal->ch3_read_v_offset;
        *gain = cal->ch3_read_v_gain;
      }
      else
      {
        *offset = cal->ch3_read_c_offset;
        *gain = cal->ch3_read_c_gain;
      }
      break;
    case 4:
      if (power == 0)
      {
        *offset = cal->ch4_read_v_offset;
        *gain = cal->ch4_read_v_gain;
      }
      else
      {
        *offset = cal->ch4_read_c_offset;
        *gain = cal->ch4_read_c_gain;
      }
      break;
    case 5:
      if (power == 0)
      {
        *offset = cal->ch5_read_v_offset;
        *gain = cal->ch5_read_v_gain;
    }
      else
      {
        *offset = cal->ch5_read_c_offset;
        *gain = cal->ch5_read_c_gain;
      }
      break;
    case 6:
      if (power == 0)
      {
        *offset = cal->ch6_read_v_offset;
        *gain = cal->ch6_read_v_gain;
      }
      else
      {
        *offset = cal->ch6_read_c_offset;
        *gain = cal->ch6_read_c_gain;
      }
      break;
    case 7:
      if (power == 0)
      {
        *offset = cal->ch7_read_v_offset;
        *gain = cal->ch7_read_v_gain;
      }
      else
      {
        *offset = cal->ch7_read_c_offset;
        *gain = cal->ch7_read_c_gain;
      }
      break;
    default:
      *offset = 0.0f;
      *gain = 1.0f;
      RA_POWEREX_DEBUG("Invalid channel (%d) for calibration selection\r\n", ch);
      break;
    }
  }
}
