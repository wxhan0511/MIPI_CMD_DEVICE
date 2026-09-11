#ifndef BSP_CD4051_H
#define BSP_CD4051_H

#include <stdint.h>

/* CD4051 channel selection definitions (corresponding to INOUT 0-7) */
typedef enum {
    MUX_CH_BIAS_I = 0, // AD_V_BIAS_I
    MUX_CH_BIAS_V = 1, // AD_V_BIAS_V
    MUX_CH_BL     = 2, // AD_V_BL
    MUX_CH_VDD    = 3, // AD_V_VDD
    MUX_CH_VBAT   = 4, // AD_V_VBAT
    MUX_CH_ELVDD  = 5, // AD_V_ELVDD
    MUX_CH_ELVSS  = 6, // AD_V_ELVSS
    MUX_CH_LS     = 7, // AD_V_LS
    MUX_CH_DISABLE = 0xFF // Disable all channels
} bsp_mux_channel_t;

/**
 * @brief Select the analog multiplexer channel
 * @param channel Target channel (0-7) or MUX_CH_DISABLE
 */
void bsp_mux_select_channel(bsp_mux_channel_t channel);

/**
 * @brief Control the power enable bits (Bit 4-7) on latch U1
 * @param vdd_en, elvdd_en, vbat_en, elvss_en Turned on when true
 */
void bsp_mux_power_ctrl(uint8_t vdd_en, uint8_t elvdd_en, uint8_t vbat_en, uint8_t elvss_en);

#endif