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
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "bsp_mcp4728.h"
#include "pin_defs.h"

/* Exported types ------------------------------------------------------------*/
typedef struct {
    float *last_voltage;    // 存放在 calibration_manager 中的上次电压
    float *offset;          // 对应校准偏移
    float *gain;            // 对应校准增益
    dac_chip_index_t chip;  // 使用哪颗芯片 (DAC_CHIP_1 到 5)
    const char *name;       // 调试名称
    uint8_t channel;        // 芯片的哪个通道 (0-3)
    uint8_t inverse;        // 是否需要取反（如 ELVSS 是负电压，校准逻辑可能不同）
    uint8_t id;
    uint8_t reserve; // 保留字段
    void (*enable_func)(uint8_t id); // 使能函数指针
    void (*disable_func)(uint8_t id); // 禁能函数指针

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
