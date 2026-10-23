#include "bsp_power_en_sequence.h"
#include "bsp_d_trigger.h"
#include <string.h>

/* TIM5 is dedicated to this sequencer; no HAL/RTOS calls from its priority-1 ISR. */
#define SEQ_START_GUARD_US 100U
#define SEQ_MAX_LATENESS_US 5U
static const uint8_t latch_bits[8] = {7, 6, 5, 1, 4, 3, 2, 0};
typedef struct { uint32_t at_us; uint8_t latch; } seq_event_t;
static seq_event_t events[POWER_EN_SEQ_MAX_EVENTS];
static volatile uint8_t running, executed;
static uint8_t count;
static volatile power_en_seq_result_t outcome;
static volatile uint32_t max_lateness;

static uint32_t read_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

uint8_t bsp_power_en_user_to_latch(uint8_t mask)
{
    uint8_t data = 3U; /* negative rails active low */
    for (uint8_t i = 0; i < 8; ++i)
        if (mask & (1U << i)) data ^= 1U << latch_bits[i];
    return data;
}

uint8_t bsp_power_en_latch_to_user(uint8_t data)
{
    uint8_t mask = 0;
    data ^= 3U;
    for (uint8_t i = 0; i < 8; ++i)
        if (data & (1U << latch_bits[i])) mask |= 1U << i;
    return mask;
}

power_en_seq_result_t bsp_power_en_sequence_validate(const uint8_t *frame, uint16_t len)
{
    if (!frame || len < 4 || frame[0] != 0xA0 || frame[1] != 0x2B || frame[2] != 1)
        return POWER_EN_SEQ_BAD_FORMAT;
    uint8_t n = frame[3];
    if (!n || n > POWER_EN_SEQ_MAX_EVENTS) return POWER_EN_SEQ_BAD_COUNT;
    if (len < 4U + n * 5U) return POWER_EN_SEQ_BAD_FORMAT;
    uint32_t previous = 0;
    for (uint8_t i = 0; i < n; ++i) {
        uint32_t t = read_u32(&frame[4U + i * 5U]);
        if ((!i && t != 0) || t > POWER_EN_SEQ_MAX_DURATION_US)
            return POWER_EN_SEQ_BAD_TIME;
        if (i && (t <= previous || t - previous < POWER_EN_SEQ_MIN_INTERVAL_US))
            return POWER_EN_SEQ_BAD_INTERVAL;
        previous = t;
    }
    return POWER_EN_SEQ_OK;
}

static void timer_stop(void)
{
    TIM5->DIER = 0;
    TIM5->CR1 = 0;
    TIM5->SR = 0;
    HAL_NVIC_DisableIRQ(TIM5_IRQn);
    HAL_NVIC_ClearPendingIRQ(TIM5_IRQn);
}

static int timer_init(void)
{
    RCC_ClkInitTypeDef clocks;
    uint32_t latency;
    HAL_RCC_GetClockConfig(&clocks, &latency);
    uint32_t hz = HAL_RCC_GetPCLK1Freq();
    if (clocks.APB1CLKDivider != RCC_HCLK_DIV1) hz *= 2U;
    if (hz < 1000000U || hz % 1000000U) return 0;
    __HAL_RCC_TIM5_CLK_ENABLE();
    timer_stop();
    TIM5->PSC = hz / 1000000U - 1U;
    TIM5->ARR = 0xFFFFFFFFU;
    TIM5->CCMR1 = 0;
    TIM5->CCER = 0;
    TIM5->EGR = TIM_EGR_UG;
    TIM5->SR = 0;
    TIM5->CNT = 0;
    TIM5->CCR1 = SEQ_START_GUARD_US;
    HAL_NVIC_SetPriority(TIM5_IRQn, 1, 0);
    HAL_NVIC_ClearPendingIRQ(TIM5_IRQn);
    return 1;
}

power_en_seq_result_t bsp_power_en_sequence_execute(const uint8_t *frame, uint16_t len,
                                                   power_en_seq_report_t *report)
{
    if (!report) return POWER_EN_SEQ_BAD_FORMAT;
    memset(report, 0, sizeof(*report));
    power_en_seq_result_t result = bsp_power_en_sequence_validate(frame, len);
    if (result != POWER_EN_SEQ_OK) return result;
    /* Keep all other latch users off the parallel bus until the ISR is stopped. */
    uint32_t hz = osKernelGetTickFreq();
    if (!bsp_d_trigger_bus_acquire((hz + 9U) / 10U))
        return POWER_EN_SEQ_RESOURCE_BUSY;
    if (!timer_init()) {
        bsp_d_trigger_bus_release();
        return POWER_EN_SEQ_RESOURCE_BUSY;
    }
    count = frame[3];
    for (uint8_t i = 0; i < count; ++i) {
        events[i].at_us = read_u32(&frame[4U + i * 5U]);
        events[i].latch = bsp_power_en_user_to_latch(frame[8U + i * 5U]);
    }
    executed = 0;
    max_lateness = 0;
    outcome = POWER_EN_SEQ_OK;
    running = 1;
    __DMB();
    uint32_t start = osKernelGetTickCount();
    TIM5->DIER = TIM_DIER_CC1IE;
    HAL_NVIC_EnableIRQ(TIM5_IRQn);
    TIM5->CR1 = TIM_CR1_CEN;
    while (running) {
        if (osKernelGetTickCount() - start > 2U * hz) {
            /* Task context cannot run concurrently with the ISR on this core. */
            timer_stop();
            outcome = POWER_EN_SEQ_WAIT_TIMEOUT;
            running = 0;
            break;
        }
        osDelay(1);
    }
    timer_stop();
    __DMB();
    report->executed_count = executed;
    report->final_user_mask = bsp_power_en_latch_to_user(bsp_d_trigger_read_byte_owned(&d_3));
    report->final_state_valid = 1;
    report->max_lateness_us = max_lateness;
    result = outcome;
    bsp_d_trigger_bus_release();
    return result;
}

void bsp_power_en_sequence_irq_handler(void)
{
    if (!(TIM5->SR & TIM_SR_CC1IF)) return;
    TIM5->SR = ~TIM_SR_CC1IF;
    if (!running) return;
    /* Only mask interrupts for the short bus-write/latch transaction, never
     * for the whole sequence. This prevents preemption between data and CP. */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint32_t late = TIM5->CNT - TIM5->CCR1;
    if (late > max_lateness) max_lateness = late;
    if (late > SEQ_MAX_LATENESS_US) {
        outcome = POWER_EN_SEQ_DEADLINE_MISSED;
    } else {
        bsp_d_trigger_write_byte_owned(&d_3, events[executed].latch);
        ++executed;
    }
    if (outcome != POWER_EN_SEQ_OK || executed == count) {
        TIM5->DIER = 0;
        TIM5->CR1 = 0;
        __DMB();
        running = 0;
    } else {
        uint32_t next = SEQ_START_GUARD_US + events[executed].at_us;
        TIM5->CCR1 = next;
        if ((int32_t)(next - TIM5->CNT) <= 0) {
            TIM5->DIER = 0;
            TIM5->CR1 = 0;
            outcome = POWER_EN_SEQ_DEADLINE_MISSED;
            running = 0;
        }
    }
    __set_PRIMASK(primask);
}

void TIM5_IRQHandler(void)
{
    bsp_power_en_sequence_irq_handler();
}
