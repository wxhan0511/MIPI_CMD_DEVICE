#include "bsp_cd4051.h"

/* Data image of latch U1 (Latch 3) */
extern uint8_t s_latch3_data; // INH=1 (disabled) by default

/* Bit offsets */
#define BIT_INH      0
#define BIT_A        1
#define BIT_B        2
#define BIT_C        3
#define BIT_ELVSS_EN 4
#define BIT_VBAT_EN  5
#define BIT_ELVDD_EN 6
#define BIT_VDD_EN   7

#define LATCH_ID_MUX 2 // Assume this is the ID of latch 3

// void bsp_mux_select_channel(bsp_mux_channel_t channel) {
//     if (channel == MUX_CH_DISABLE) {
//         s_latch3_data |= (1 << BIT_INH); // Set INH to 1, disconnect
//     } else {
//         s_latch3_data &= ~(1 << BIT_INH); // Clear INH to 0, select
//         /* Clear the A,B,C bits and set the new address */
//         s_latch3_data &= ~(0x07 << BIT_A);
//         s_latch3_data |= ((channel & 0x07) << BIT_A);
//     }
    
//     bsp_latch_write(LATCH_ID_MUX, s_latch3_data);
// }

// void bsp_mux_power_ctrl(uint8_t vdd_en, uint8_t elvdd_en, uint8_t vbat_en, uint8_t elvss_en) {
//     if (vdd_en)   s_latch3_data |= (1 << BIT_VDD_EN);   else s_latch3_data &= ~(1 << BIT_VDD_EN);
//     if (elvdd_en) s_latch3_data |= (1 << BIT_ELVDD_EN); else s_latch3_data &= ~(1 << BIT_ELVDD_EN);
//     if (vbat_en)  s_latch3_data |= (1 << BIT_VBAT_EN);  else s_latch3_data &= ~(1 << BIT_VBAT_EN);
//     if (elvss_en) s_latch3_data |= (1 << BIT_ELVSS_EN); else s_latch3_data &= ~(1 << BIT_ELVSS_EN);
    
//     bsp_latch_write(LATCH_ID_MUX, s_latch3_data);
// }