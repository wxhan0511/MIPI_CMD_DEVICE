#ifndef __CALIBRATION_UTILS_H
#define __CALIBRATION_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/


/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void sel_cali_param(uint8_t ch, uint8_t type, uint8_t power, float *offset, float *gain);
int16_t float_to_int16_round(float value);
uint16_t float_to_uint16_round(float value);
uint8_t float_to_uint8_round(float value);
#ifdef __cplusplus
}
#endif

#endif /* __CALIBRATION_UTILS_H */
