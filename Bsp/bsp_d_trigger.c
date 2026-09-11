/**
 * @file bsp_d_trigger.c
 * @brief D flip-flop control module
 *
 * Usage (recommended flow):
 * 1) First initialize a specific flip-flop (d_1 ~ d_8):
 *      bsp_d_trigger_init(d_1);
 * 2) Enable the master output (OE, note that this function writes inverted):
 *      bsp_d_trigger_set(1);   // 1=enable output, 0=disable output
 * 3) Set a channel state and latch it into the flip-flop:
 *      bsp_d_trigger_set_channel(&d_1, 0, 1); // Set d_1 CH0 high
 *      bsp_d_trigger_set_channel(&d_1, 3, 0); // Set d_1 CH3 low
 *
 * Parameter conventions:
 * - channel: 0~7 (8 channels in total)
 * - enable : 0/1 (low/high)
 *
 * Timing notes:
 * - Write the data pins first, then generate a rising-edge/falling-edge pulse on d_clk to complete the latch.
 */

#include "bsp_d_trigger.h"

#include "bsp_dwt.h" //for delay
#include "cmsis_os2.h"
#include "utils.h"
static osMutexId_t s_d_trigger_mutex = NULL;
static uint8_t s_d_trigger_mutex_ready = 0;

#define D_TRIGGER_CHANNEL_NUM 8
#define D_TRIGGER_DEVICE_NUM 8

/* 8-channel state cache per D flip-flop: bit0~bit7 correspond to channel 0~7 */
static volatile uint8_t s_d_trigger_shadow[D_TRIGGER_DEVICE_NUM] = {0};
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
void bsp_d_trigger_lock_init(void)
{
    if (s_d_trigger_mutex == NULL)
    {
        s_d_trigger_mutex = osMutexNew(NULL);
        if (s_d_trigger_mutex != NULL)
        {
            s_d_trigger_mutex_ready = 1;
        }
    }
}
/**
 * @brief Initialize all D flip-flop clock pins and 8 data pins, close the 24/40-pin channels to prevent leakage, initialize the mutex
 */
void bsp_all_d_trigger_init()
{
    bsp_d_trigger_init(d_1);
    bsp_d_trigger_init(d_2);
    bsp_d_trigger_init(d_3);
    bsp_d_trigger_init(d_4);
    bsp_d_trigger_init(d_5);
    bsp_d_trigger_init(d_6);
    bsp_d_trigger_init(d_7);
    bsp_d_trigger_init(d_8);
    bsp_d_trigger_set(enabled);
    bsp_close_24pin_channel();
    bsp_close_40pin_channel();
    bsp_d_trigger_lock_init();
}
/**
 * @brief Initialize one D flip-flop's clock pin and 8 data pins
 * @param cfg Flip-flop configuration (pass d_1 ~ d_8)
 */
void bsp_d_trigger_init(d_trigger_t cfg)
{

    // Clock pin initialization
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = cfg.d_clk_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(cfg.d_clk_group, &GPIO_InitStruct);
    // Data pin initialization
    for (uint8_t i = 0; i < D_TRIGGER_CHANNEL_NUM; i++)
    {
        GPIO_InitStruct.Pin = cfg.d_pin[i];
        HAL_GPIO_Init(cfg.d_group[i], &GPIO_InitStruct);
    }
}
/**
 * @brief Master enable control (OE)
 * @param state 1=enable output, 0=disable output
 * @note  The hardware OE is active-low; the function writes the pin inverted with !state
 */
void bsp_d_trigger_set(uint8_t state)
{
    HAL_GPIO_WritePin(OE_GPIO_Port, OE_Pin, !state);
}
static int8_t bsp_d_trigger_get_index(const d_trigger_t *cfg)
{
    if (cfg == &d_1)
        return 0;
    if (cfg == &d_2)
        return 1;
    if (cfg == &d_3)
        return 2;
    if (cfg == &d_4)
        return 3;
    if (cfg == &d_5)
        return 4;
    if (cfg == &d_6)
        return 5;
    if (cfg == &d_7)
        return 6;
    if (cfg == &d_8)
        return 7;
    return -1;
}
/**
 * @brief Set a single channel and latch it
 * @param cfg     Flip-flop configuration pointer (&d_1 ~ &d_8)
 * @param channel Channel number 0~7
 * @param enable  Level state 0/1
 *
 * Example:
 *    bsp_d_trigger_init(d_2);
 *    bsp_d_trigger_set(1);
 *    bsp_d_trigger_set_channel(&d_2, 5, 1); // Set d_2 CH5 output high
 */
void bsp_d_trigger_set_channel(const d_trigger_t *cfg, const uint8_t channel, const uint8_t enable)
{
    if (cfg == NULL || channel >= D_TRIGGER_CHANNEL_NUM)
    {
        return;
    }
    if (osMutexAcquire(s_d_trigger_mutex, osWaitForever) != osOK)
    {
        return;
    }

    /* 1) Update the software cache first */
    int8_t idx = bsp_d_trigger_get_index(cfg);
    if (idx >= 0)
    {
        if (enable)
            s_d_trigger_shadow[idx] |= (uint8_t)(1U << channel);
        else
            s_d_trigger_shadow[idx] &= (uint8_t)~(1U << channel);
    }
    /* 2) Replay all 8 data pins from the cache, so changing one channel does not lose the state of the others */
    uint8_t shadow = s_d_trigger_shadow[idx];
    for (uint8_t i = 0; i < D_TRIGGER_CHANNEL_NUM; i++)
    {
        GPIO_PinState st = ((shadow >> i) & 0x01U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
        HAL_GPIO_WritePin(cfg->d_group[i], cfg->d_pin[i], st);
    }

    HAL_GPIO_WritePin(cfg->d_clk_group, cfg->d_clk_pin, GPIO_PIN_SET);

    bsp_delay_us(20);

    HAL_GPIO_WritePin(cfg->d_clk_group, cfg->d_clk_pin, GPIO_PIN_RESET);
    bsp_delay_us(20);

    osMutexRelease(s_d_trigger_mutex);
}

uint8_t bsp_d_trigger_get_channel_state(const d_trigger_t *cfg, const uint8_t channel)
{
    uint8_t ret = 0;
    if (cfg == NULL || channel >= D_TRIGGER_CHANNEL_NUM)
    {
        return 0;
    }

    if (osMutexAcquire(s_d_trigger_mutex, osWaitForever) != osOK)
    {
        return 0;
    }

    int8_t idx = bsp_d_trigger_get_index(cfg);
    if (idx >= 0)
    {
        ret = (s_d_trigger_shadow[idx] >> channel) & 0x01U;
    }
    osMutexRelease(s_d_trigger_mutex);

    return ret;
}

void test_d_trigger()
{
    /**
     * Example test flow:
     * 1. Initialize one flip-flop
     * 2. Enable the master output
     * 3. Set each channel high then low
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

    for (uint8_t i = 0; i < 8; i++)
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

    for (uint8_t i = 0; i < 8; i++)
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
