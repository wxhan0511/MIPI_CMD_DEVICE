//
// Created by 薛斌 on 24-8-19.
//

#include "bsp_d_trigger.h"

#include "bsp_dwt.h"//for delay

#define D_TRIGGER_CHANNEL_NUM 8

GPIO_TypeDef *latch_group[8] = {
    LATCH_0_GPIO_Port,
    LATCH_1_GPIO_Port,
    LATCH_2_GPIO_Port,
    LATCH_3_GPIO_Port,
    LATCH_4_GPIO_Port,
    LATCH_5_GPIO_Port,
    LATCH_6_GPIO_Port,
    LATCH_7_GPIO_Port,
};

uint16_t latch_pin[8] = {
    LATCH_0_Pin,
    LATCH_1_Pin,
    LATCH_2_Pin,
    LATCH_3_Pin,
    LATCH_4_Pin,
    LATCH_5_Pin,
    LATCH_6_Pin,
    LATCH_7_Pin,
};

uint16_t latch_pin_2[8] = {
    LATCH_7_Pin,
    LATCH_6_Pin,
    LATCH_5_Pin,
    LATCH_4_Pin,
    LATCH_3_Pin,
    LATCH_2_Pin,
    LATCH_1_Pin,
    LATCH_0_Pin,
};

const d_trigger_t d_1 = {
    .d_clk_group = D_CLK_1_GPIO_Port,
    .d_clk_pin = D_CLK_1_Pin,
    .d_group = latch_group,
    .d_pin = latch_pin,
};

const d_trigger_t d_2 = {
    .d_clk_group = D_CLK_2_GPIO_Port,
    .d_clk_pin = D_CLK_2_Pin,
    .d_group = latch_group,
    .d_pin = latch_pin,
};

const d_trigger_t d_3 = {
    .d_clk_group = D_CLK_3_GPIO_Port,
    .d_clk_pin = D_CLK_3_Pin,
    .d_group = latch_group,
    .d_pin = latch_pin,
};

const d_trigger_t d_4 = {
    .d_clk_group = D_CLK_4_GPIO_Port,
    .d_clk_pin = D_CLK_4_Pin,
    .d_group = latch_group,
    .d_pin = latch_pin,
};

const d_trigger_t d_5 = {
    .d_clk_group = D_CLK_5_GPIO_Port,
    .d_clk_pin = D_CLK_5_Pin,
    .d_group = latch_group,
    .d_pin = latch_pin,
};

const d_trigger_t d_6 = {
    .d_clk_group = D_CLK_6_GPIO_Port,
    .d_clk_pin = D_CLK_6_Pin,
    .d_group = latch_group,
    .d_pin = latch_pin,
};

const d_trigger_t d_7 = {
    .d_clk_group = D_CLK_7_GPIO_Port,
    .d_clk_pin = D_CLK_7_Pin,
    .d_group = latch_group,
    .d_pin = latch_pin,
};

const d_trigger_t d_8 = {
    .d_clk_group = D_CLK_8_GPIO_Port,
    .d_clk_pin = D_CLK_8_Pin,
    .d_group = latch_group,
    .d_pin = latch_pin,
};

void bsp_d_trigger_init( d_trigger_t cfg)
{

    //时钟引脚初始化
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = cfg.d_clk_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(cfg.d_clk_group, &GPIO_InitStruct);
    //数据引脚初始化
    for(uint8_t i=0;i<D_TRIGGER_CHANNEL_NUM;i++)
    {
        GPIO_InitStruct.Pin = cfg.d_pin[i];
        HAL_GPIO_Init(cfg.d_group[i], &GPIO_InitStruct);
    }

}
void bsp_d_trigger_set(uint8_t state)
{
    HAL_GPIO_WritePin(OE_GPIO_Port, OE_Pin, !state);
}
/*Set a channel of the D trigger
    @param cfg : D trigger configuration d1~d8
    @param channel : Channel number 0-7
    @param enable :  state 0/1
*/ 
void bsp_d_trigger_set_channel(const d_trigger_t *cfg, const uint8_t channel, const uint8_t enable)
{
    if(cfg == NULL || channel >= D_TRIGGER_CHANNEL_NUM)
    {
        return;
    }
    HAL_GPIO_WritePin(cfg->d_group[channel],cfg->d_pin[channel],enable);
    //bsp_delay_us(20);

    HAL_GPIO_WritePin(cfg->d_clk_group,cfg->d_clk_pin,GPIO_PIN_SET);

    bsp_delay_us(20);

    HAL_GPIO_WritePin(cfg->d_clk_group,cfg->d_clk_pin,GPIO_PIN_RESET);
    bsp_delay_us(20);
    //osDelay(1);
}
