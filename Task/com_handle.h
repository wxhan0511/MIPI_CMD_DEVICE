/* ==================== 1. 头文件保护 ==================== */
#ifndef COM_HANDLE_H
#define COM_HANDLE_H

/* ==================== 2. 头文件包含 ==================== */
#include <stdint.h>
#include "task_manage.h"
#include "bsp.h"
#include "task_sample.h"

/* ==================== 3. 宏定义 ==================== */
/* 无 */

/* ==================== 4. 类型定义 ==================== */
typedef enum
{
    METER_OK = 0,
    METER_ERROR = 1,
    METER_TIMEOUT = 2,
} METER_STATE;

/* ==================== 5. 外部全局变量声明 ==================== */
extern uint8_t meter_tx_buf[];
extern uint8_t meter_rx_buf[];
extern SampleTask_S g_sample_task;

/* ==================== 6. 外部函数声明 ==================== */
void com_handle_i2c(const uint8_t *rx, uint8_t *tx);
void com_handle_i2c_init(void);
BSP_STATUS com_handle_spi(const uint8_t *rx, uint8_t *tx);

/* ==================== 7. 结束头文件保护 ==================== */
#endif /* COM_HANDLE_H */
