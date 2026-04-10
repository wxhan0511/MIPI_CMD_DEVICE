//
// Created by 薛斌 on 24-8-19.
//

#ifndef BSP_D_TRIGGER_H
#define BSP_D_TRIGGER_H

#include "bsp.h"
#include "main.h"

typedef struct
{
    GPIO_TypeDef *d_clk_group;//CP Group
    uint16_t d_clk_pin;

    GPIO_TypeDef **d_group;//Latch Data Group
    uint16_t *d_pin;
}d_trigger_t;

extern const d_trigger_t d_1;
extern const d_trigger_t d_2;
extern const d_trigger_t d_3;
extern const d_trigger_t d_4;
extern const d_trigger_t d_5;
extern const d_trigger_t d_6;
extern const d_trigger_t d_7;
extern const d_trigger_t d_8;


void bsp_d_trigger_init(d_trigger_t cfg);
void bsp_d_trigger_set(uint8_t state);
void bsp_d_trigger_set_channel(const d_trigger_t *cfg, const uint8_t channel,uint8_t enable);
void test_d_trigger();
#endif //BSP_D_TRIGGER_H
