#ifndef BSP_POWER_EN_SEQUENCE_H
#define BSP_POWER_EN_SEQUENCE_H
#include <stdint.h>

#define POWER_EN_SEQ_MAX_EVENTS 8U
#define POWER_EN_SEQ_MIN_INTERVAL_US 50U
#define POWER_EN_SEQ_MAX_DURATION_US 1000000U

typedef enum {
    POWER_EN_SEQ_OK = 0,
    POWER_EN_SEQ_BAD_FORMAT,
    POWER_EN_SEQ_BAD_COUNT,
    POWER_EN_SEQ_BAD_TIME,
    POWER_EN_SEQ_BAD_INTERVAL,
    POWER_EN_SEQ_RESOURCE_BUSY,
    POWER_EN_SEQ_DEADLINE_MISSED,
    POWER_EN_SEQ_WAIT_TIMEOUT
} power_en_seq_result_t;

typedef struct {
    uint8_t executed_count;
    uint8_t final_user_mask;
    uint8_t final_state_valid;
    uint32_t max_lateness_us;
} power_en_seq_report_t;

uint8_t bsp_power_en_user_to_latch(uint8_t mask);
uint8_t bsp_power_en_latch_to_user(uint8_t data);
power_en_seq_result_t bsp_power_en_sequence_validate(const uint8_t *frame, uint16_t len);
power_en_seq_result_t bsp_power_en_sequence_execute(const uint8_t *frame, uint16_t len,
                                                   power_en_seq_report_t *report);
void bsp_power_en_sequence_irq_handler(void);
#endif
