/**
 * @file       bsp_mcp4728_ctl.h
 * @brief
 * @author     wxhan
 * @version    1.0.0
 * @date       2026-01-28
 * @copyright  Copyright (c) 2026 gcoreinc
 * @license    MIT License
 */

#ifndef __BSP_MCP4728_CTL_H
#define __BSP_MCP4728_CTL_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "pin_defs.h"


    typedef enum
    {
        DAC_CHIP_1 = 0,
        DAC_CHIP_2,
        DAC_CHIP_3,
        DAC_CHIP_4,
        DAC_CHIP_5,
        DAC_CHIP_MAX
    } dac_chip_index_t;
    /* Exported types ------------------------------------------------------------*/
    typedef struct
    {
        float *last_voltage;   // Last voltage stored in calibration_manager
        float *offset;         // Corresponding calibration offset
        float *gain;           // Corresponding calibration gain
        dac_chip_index_t chip; // Which chip is used (DAC_CHIP_1 to 5)
        const char *name;      // Debug name
        uint8_t channel;       // Which channel of the chip (0-3)
        uint8_t inverse;       // Whether inversion is needed (e.g. ELVSS is a negative voltage, so its calibration logic may differ)
        uint8_t id;
        uint8_t reserve;                  // Reserved field
        void (*enable_func)(uint8_t id);  // Enable function pointer
        void (*disable_func)(uint8_t id); // Disable function pointer
        const char *name1;

    } dac_config_table_t;

    /* Exported constants --------------------------------------------------------*/
    extern dac_config_table_t dac_config_table[20];
    /* Exported macro ------------------------------------------------------------*/

#define DAC_LDAC_1_H() HAL_GPIO_WritePin(DAC_LDAC1_GPIO_Port, DAC_LDAC1_Pin, GPIO_PIN_SET)
#define DAC_LDAC_1_L() HAL_GPIO_WritePin(DAC_LDAC1_GPIO_Port, DAC_LDAC1_Pin, GPIO_PIN_RESET)
#define DAC_LDAC_2_H() HAL_GPIO_WritePin(DAC_LDAC2_GPIO_Port, DAC_LDAC2_Pin, GPIO_PIN_SET)
#define DAC_LDAC_2_L() HAL_GPIO_WritePin(DAC_LDAC2_GPIO_Port, DAC_LDAC2_Pin, GPIO_PIN_RESET)
#define DAC_LDAC_3_H() HAL_GPIO_WritePin(DAC_LDAC3_GPIO_Port, DAC_LDAC3_Pin, GPIO_PIN_SET)
#define DAC_LDAC_3_L() HAL_GPIO_WritePin(DAC_LDAC3_GPIO_Port, DAC_LDAC3_Pin, GPIO_PIN_RESET)
#define DAC_LDAC_4_H() HAL_GPIO_WritePin(DAC_LDAC4_GPIO_Port, DAC_LDAC4_Pin, GPIO_PIN_SET)
#define DAC_LDAC_4_L() HAL_GPIO_WritePin(DAC_LDAC4_GPIO_Port, DAC_LDAC4_Pin, GPIO_PIN_RESET)
#define DAC_LDAC_5_H() HAL_GPIO_WritePin(DAC_LDAC5_GPIO_Port, DAC_LDAC5_Pin, GPIO_PIN_SET)
#define DAC_LDAC_5_L() HAL_GPIO_WritePin(DAC_LDAC5_GPIO_Port, DAC_LDAC5_Pin, GPIO_PIN_RESET)
    /* Exported functions prototypes ---------------------------------------------*/
    void bsp_power_all_disable();
    void bsp_power_all_enable();
    void bsp_dac_init();
    void mcp4728_device_init();
    void bsp_cali_and_set_power(uint8_t power_id);
    void bsp_power_single_enable(uint8_t power_id);
    void bsp_power_single_disable(uint8_t power_id);
    void single_mcp4728_sync_update(uint8_t power_id);
    void all_mcp4728_sync_update();
#ifdef __cplusplus
}
#endif

#endif /* __BSP_MCP4728_CTL_H */
