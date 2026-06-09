/**
 * @brief  任务层：通信任务
 */

/* Includes ------------------------------------------------------------------*/
#include "task_com.h"
#include "task_manage.h"
#include <stdio.h>
#include <string.h>
#include "spi.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile uint8_t meter_com_flag = 0;
extern uint8_t meter_rx_buf[SPI2_SLAVE_RX_LEN];
extern uint8_t meter_tx_buf[SPI2_SLAVE_TX_LEN];
osThreadId_t task_sample_handle;
const osThreadAttr_t task_sample_attributes = {
    .name = "task_sample_task",
    .stack_size = 4096,
    .priority = (osPriority_t)osPriorityHigh,
};

osThreadId_t task_com_handle;
const osThreadAttr_t task_com_attributes = {
    .name = "task_com_task",
    .stack_size = 4096,
    .priority = (osPriority_t)osPriorityHigh,
};
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static void task_com_run(void *arg)
{
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

void task_com_suspend()
{
    osThreadSuspend(task_com_handle);
}

void task_com_resume()
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
/* Exported functions --------------------------------------------------------*/
