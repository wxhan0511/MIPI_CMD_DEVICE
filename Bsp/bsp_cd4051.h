#ifndef BSP_CD4051_H
#define BSP_CD4051_H

#include <stdint.h>

/* CD4051 选通通道定义 (对应 INOUT 0-7) */
typedef enum {
    MUX_CH_BIAS_I = 0, // AD_V_BIAS_I
    MUX_CH_BIAS_V = 1, // AD_V_BIAS_V
    MUX_CH_BL     = 2, // AD_V_BL
    MUX_CH_VDD    = 3, // AD_V_VDD
    MUX_CH_VBAT   = 4, // AD_V_VBAT
    MUX_CH_ELVDD  = 5, // AD_V_ELVDD
    MUX_CH_ELVSS  = 6, // AD_V_ELVSS
    MUX_CH_LS     = 7, // AD_V_LS
    MUX_CH_DISABLE = 0xFF // 禁止所有通道
} bsp_mux_channel_t;

/**
 * @brief 选择模拟切换通道
 * @param channel 目标通道 (0-7) 或 MUX_CH_DISABLE
 */
void bsp_mux_select_channel(bsp_mux_channel_t channel);

/**
 * @brief 控制锁存器 U1 上的电源使能位 (Bit 4-7)
 * @param vdd_en, elvdd_en, vbat_en, elvss_en 为 true 时打开
 */
void bsp_mux_power_ctrl(uint8_t vdd_en, uint8_t elvdd_en, uint8_t vbat_en, uint8_t elvss_en);

#endif