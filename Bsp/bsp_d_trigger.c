/**
 * @file bsp_d_trigger.c
 * @brief D 触发器控制模块
 *
 * 使用说明（推荐流程）：
 * 1) 先初始化具体一路触发器（d_1 ~ d_8）：
 *      bsp_d_trigger_init(d_1);
 * 2) 使能总输出（OE，注意本函数内部是反相写入）：
 *      bsp_d_trigger_set(1);   // 1=使能输出，0=关闭输出
 * 3) 设置某一路通道状态并锁存到触发器：
 *      bsp_d_trigger_set_channel(&d_1, 0, 1); // d_1 的 CH0 置高
 *      bsp_d_trigger_set_channel(&d_1, 3, 0); // d_1 的 CH3 置低
 *
 * 参数约定：
 * - channel: 0~7（共 8 路）
 * - enable : 0/1（低/高）
 *
 * 时序说明：
 * - 先写数据脚，再对 d_clk 产生一个上升沿-下降沿脉冲完成锁存。
 */

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
/**
 * @brief 初始化一路 D 触发器的时钟脚与 8 路数据脚
 * @param cfg 触发器配置（可传 d_1 ~ d_8）
 */
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
/**
 * @brief 总使能控制（OE）
 * @param state 1=使能输出，0=关闭输出
 * @note  硬件为低有效 OE，函数内部使用 !state 反相写引脚
 */
void bsp_d_trigger_set(uint8_t state)
{
    HAL_GPIO_WritePin(OE_GPIO_Port, OE_Pin, !state);
}
/**
 * @brief 设置单个通道并锁存
 * @param cfg     触发器配置指针（&d_1 ~ &d_8）
 * @param channel 通道号 0~7
 * @param enable  电平状态 0/1
 *
 * 示例：
 *    bsp_d_trigger_init(d_2);
 *    bsp_d_trigger_set(1);
 *    bsp_d_trigger_set_channel(&d_2, 5, 1); // d_2 CH5 输出高
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
}


void test_d_trigger()
{
    /**
     * 示例测试流程：
     * 1. 初始化一路触发器
     * 2. 打开总使能
     * 3. 逐通道置高再置低
     */
    bsp_d_trigger_init(d_1);
    bsp_d_trigger_init(d_2);
    bsp_d_trigger_init(d_3);
    bsp_d_trigger_init(d_4);
    bsp_d_trigger_init(d_5);
    bsp_d_trigger_init(d_6);
    bsp_d_trigger_init(d_7);
    bsp_d_trigger_init(d_8);
    bsp_d_trigger_set(1);

    for (uint8_t i=0;i<8;i++)
    {
        bsp_d_trigger_set_channel(&d_1, i, 0);
        bsp_d_trigger_set_channel(&d_2, i, 0);
        bsp_d_trigger_set_channel(&d_3, i, 0);
        bsp_d_trigger_set_channel(&d_4, i, 0);
        bsp_d_trigger_set_channel(&d_5, i, 0);
        bsp_d_trigger_set_channel(&d_6, i, 0);
        bsp_d_trigger_set_channel(&d_7, i, 0);
        bsp_d_trigger_set_channel(&d_8, i, 0);
    }

    for (uint8_t i=0;i<8;i++)
    {
        bsp_d_trigger_set_channel(&d_1, i, 1);
        bsp_d_trigger_set_channel(&d_2, i, 1);
        bsp_d_trigger_set_channel(&d_3, i, 1);
        bsp_d_trigger_set_channel(&d_4, i, 1);
        bsp_d_trigger_set_channel(&d_5, i, 1);
        bsp_d_trigger_set_channel(&d_6, i, 1);
        bsp_d_trigger_set_channel(&d_7, i, 1);
        bsp_d_trigger_set_channel(&d_8, i, 1);
    }
    bsp_d_trigger_set(0);
}
