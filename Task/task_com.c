/**
 * @brief  任务层：通信任务
 */

/* ==================== 1. 头文件包含 ==================== */
#include "task_com.h"
#include "task_manage.h"
#include "com_handle.h"
#include <stdio.h>
#include <string.h>
#include "spi.h"

/* ==================== 2. 宏定义 ==================== */
/* 无 */

/* ==================== 3. 类型定义（结构体、枚举、别名） ==================== */
/* 无 */

/* ==================== 4. 外部全局变量 ==================== */
extern uint8_t meter_rx_buf[SPI2_SLAVE_RX_LEN];
extern uint8_t meter_tx_buf[SPI2_SLAVE_TX_LEN];

/* ==================== 5. 静态私有变量 ==================== */
volatile uint8_t meter_com_flag = 0;
osThreadId_t task_com_handle;
const osThreadAttr_t task_com_attributes = {
    .name = "task_com_task",
    .stack_size = 4096,
    .priority = (osPriority_t)osPriorityHigh,
};

/* ==================== 6. 静态函数声明 ==================== */
static void task_com_run(void *arg);

/* ==================== 7. 外部可调用函数实现 ==================== */
void task_com_suspend(void)
{
    osThreadSuspend(task_com_handle);
}

void task_com_resume(void)
{
    osThreadResume(task_com_handle);
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

/* ==================== 8. 静态私有函数实现 ==================== */
static void task_com_run(void *arg)
{
    (void)arg;
    osDelay(1000);
    MX_SPI2_Init();
    SPI2_Slave_StartRx_IT(); // 启动SPI2从机64字节接收
    M_INT_HIGH();
    while (1)
    {

        if (meter_com_flag == 1) //
        {
            meter_com_flag = 0;
            com_handle_spi(meter_rx_buf, meter_tx_buf);
        }
        osDelay(10);
    }
}
