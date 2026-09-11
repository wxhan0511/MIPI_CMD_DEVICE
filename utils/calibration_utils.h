#ifndef __CALIBRATION_UTILS_H
#define __CALIBRATION_UTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

    /* Exported types ------------------------------------------------------------*/

    /* Exported constants --------------------------------------------------------*/

    /* Exported macro ------------------------------------------------------------*/

    /* Exported functions prototypes ---------------------------------------------*/
    void sel_cali_param(uint8_t main_index, uint8_t sub_index, float *offset, float *gain);
    void set_cali_param(uint8_t main_index, uint8_t sub_index, float offset, float gain);
    uint16_t float_to_uint16_round(float value);
    uint8_t float_to_uint8_round(float value);
#ifdef __cplusplus
}
#endif

#endif /* __CALIBRATION_UTILS_H */
