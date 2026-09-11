/* ==================== 1. Header guard ==================== */
#ifndef COM_HANDLE_H
#define COM_HANDLE_H

/* ==================== 2. Includes ==================== */
#include <stdint.h>
#include "task_manage.h"
#include "bsp.h"
#include "task_sample.h"

/* ==================== 3. Macros ==================== */
/* None */

/* ==================== 4. Type definitions ==================== */
typedef enum
{
    METER_OK = 0,
    METER_ERROR = 1,
    METER_TIMEOUT = 2,
} METER_STATE;

/* ==================== 5. External global variable declarations ==================== */
extern uint8_t meter_tx_buf[];
extern uint8_t meter_rx_buf[];
extern SampleTask_S g_sample_task;

/* ==================== 6. External function declarations ==================== */
BSP_STATUS com_handle_spi(const uint8_t *rx, uint8_t *tx);

/* ==================== 7. End of header guard ==================== */
#endif /* COM_HANDLE_H */
