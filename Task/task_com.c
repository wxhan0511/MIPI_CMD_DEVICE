/**
 * @brief  Task layer: communication task
 */

/* ==================== 1. Includes ==================== */
#include "task_com.h"
#include "task_manage.h"
#include "com_handle.h"
#include <stdio.h>
#include <string.h>
#include "spi.h"

/* ==================== 2. Macros ==================== */
#define TASK_COM_RESPONSE_READY_FLAG (1UL << 0)

/* ==================== 3. Type definitions (structs, enums, aliases) ==================== */
/* None */

/* ==================== 4. External global variables ==================== */
extern uint8_t meter_rx_buf[SPI2_SLAVE_RX_LEN];
extern uint8_t meter_tx_buf[SPI2_SLAVE_TX_LEN];

/* ==================== 5. Static private variables ==================== */
volatile uint8_t meter_com_flag = 0;
osThreadId_t task_com_handle;
const osThreadAttr_t task_com_attributes = {
    .name = "task_com_task",
    .stack_size = 5120,
    .priority = (osPriority_t)osPriorityHigh,
};

/* ==================== 6. Static function declarations ==================== */
static void task_com_run(void *arg);

/* ==================== 7. Public function implementations ==================== */
void task_com_suspend(void)
{
    osThreadFlagsWait(TASK_COM_RESPONSE_READY_FLAG, osFlagsWaitAny, osWaitForever);
}

void task_com_resume(void)
{
    g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
    osThreadFlagsSet(task_com_handle, TASK_COM_RESPONSE_READY_FLAG);
}

void task_com_init(void)
{
    task_com_handle = osThreadNew(task_com_run, NULL, &task_com_attributes);
    if (task_com_handle == NULL)
    {
        LOG_ERROR("task_com_handle is NULL");
    }
    else
    {
        LOG_INFO("task_com_handle is OK");
    }
}

/* ==================== 8. Static private function implementations ==================== */
static void task_com_run(void *arg)
{
    (void)arg;
    osDelay(1000);
    MX_SPI2_Init();
    SPI2_Slave_StartRx_IT(); // Start SPI2 slave 64-byte reception
    M_INT_HIGH();
    printf("init status M_INT_HIGH\r\n");
    while (1)
    {

        if (meter_com_flag == 1) //
        {
            meter_com_flag = 0;
            com_handle_spi(meter_rx_buf, meter_tx_buf);
        }
        osDelay(2);
    }
}
