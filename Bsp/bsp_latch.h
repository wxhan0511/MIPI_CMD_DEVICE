#ifndef BSP_LATCH_H
#define BSP_LATCH_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#include "pin_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_LATCH_MAX_COUNT  3U

/* Latch 1 (U28): Relay and Core Power Enable */
typedef union {
    uint8_t val;
    struct {
        uint8_t rly1       : 1; /* Q0: RLY1 */
        uint8_t rly2       : 1; /* Q1: RLY2 */
        uint8_t rly3       : 1; /* Q2: RLY3 */
        uint8_t rly4       : 1; /* Q3: RLY4 */
        uint8_t iovcc_en   : 1; /* Q4: IOVCC_EN */
        uint8_t vcc_en     : 1; /* Q5: VCC_EN */
        uint8_t vsp_en     : 1; /* Q6: VSP_EN */
        uint8_t vsn_en     : 1; /* Q7: VSN_EN */
    } bits;
} latch1_t;

/* Latch 0 (U27): Current limiting and ADC channel switching */
typedef union {
    uint8_t val;
    struct {
        uint8_t ilim_rst   : 1; /* Q0: Ilim_RST */
        uint8_t adc_inh1   : 1; /* Q1: ADC_INH1 */
        uint8_t adc_c      : 1; /* Q2: ADC_C */
        uint8_t adc_b      : 1; /* Q3: ADC_B */
        uint8_t adc_a      : 1; /* Q4: ADC_A */
        uint8_t adc_inh2   : 1; /* Q5: ADC_INH2 */
        uint8_t adc_inh3   : 1; /* Q6: ADC_INH3 */
        uint8_t cp0        : 1; /* Q7: CP0 */
    } bits;
} latch0_t;

/* Latch 2 (U1): Voltage measurement channel and screen power control */
typedef union {
    uint8_t val;
    struct {
        uint8_t adc_inh    : 1; /* Q0: ADC_INH */
        uint8_t adc_a      : 1; /* Q1: ADC_A */
        uint8_t adc_b      : 1; /* Q2: ADC_B */
        uint8_t adc_c      : 1; /* Q3: ADC_C */
        uint8_t elvss_en   : 1; /* Q4: ELVSS_EN */
        uint8_t vbat_en    : 1; /* Q5: VBAT_EN */
        uint8_t elvdd_en   : 1; /* Q6: ELVDD_EN */
        uint8_t vdd_en     : 1; /* Q7: VDD_EN */
    } bits;
} latch2_t;

/* Q-state structure of three latches */
typedef struct {
    latch0_t s_latch0_qdata; // ID 0
    latch1_t s_latch1_qdata; // ID 1
    latch2_t s_latch2_qdata; // ID 2
} bsp_latch_qstatus_t;

typedef struct
{
    GPIO_TypeDef *cp_port;  /* latch clock (CP) */
    GPIO_TypeDef *oe_port;  /* output enable, active low */
    uint16_t      cp_pin;
    uint16_t      oe_pin;
} bsp_latch_cfg_t;



extern bsp_latch_cfg_t s_latch_cfg[BSP_LATCH_MAX_COUNT];
extern bsp_latch_qstatus_t g_bsp_latch_qstatus;

/**
 * @brief Initialize shared data bus (D0..D7) and per-latch CP/OE pins.
 */
void bsp_latch_init();

/**
 * @brief Drive one latch with an 8-bit value (latched on CP rising edge).
 * @param qstatus Pointer to the latch status structure.
 * @param latch_id Latch index (0..count-1).
 */
void bsp_latch_write(bsp_latch_qstatus_t* qstatus, uint8_t latch_id);

/**
 * @brief Set the output enable (OE#) of the specified latch.
 * @param id     Latch index (0..count-1).
 * @param enable true to enable outputs, false to tri-state.
 */
void bsp_latch_set_output(uint8_t id, bool enable);
#ifdef __cplusplus
}
#endif

#endif /* BSP_LATCH_H */
