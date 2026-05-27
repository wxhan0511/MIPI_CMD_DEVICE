/**
 * @brief
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "com_handle.h"
#include "task_sample.h"
#include "spi.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
extern uint8_t meter_tx_buf[SPI2_SLAVE_TX_LEN];
extern uint8_t meter_rx_buf[SPI2_SLAVE_RX_LEN];
extern SampleTask_S g_sample_task;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
BSP_STATUS
com_handle_spi(const uint8_t *rx, uint8_t *tx)
{

    // 通过帧首判断模式
    if (rx[0] == 0xA0)
    {
        task_sample_task_mutex_acquire(); // 获取采样任务的互斥锁，停止通信任务,采样任务处理完命令后恢复通信任务
        {
            // 处理接收的数据
            g_sample_task.frame_header = meter_rx_buf[0];
            g_sample_task.cmd_type = meter_rx_buf[1];
            g_sample_task.cmd_status = POWER_CMD_STATUS_SUCCESS;
            // 处理命令--------------------
            task_sample_suspend();
            task_sample_resume();
            task_com_suspend(); // 暂停通信任务，等待采样任务处理完命令后调用task_com_resume()恢复通信任务
            // 准备待发送数据
            meter_tx_buf[0] = g_sample_task.frame_header;
            meter_tx_buf[1] = g_sample_task.cmd_type;
            // 发送数据给上位机
            meter_com_flag = 0;
            SPI2_Slave_Send_IT(meter_tx_buf, SPI2_SLAVE_TX_LEN);
            M_INT_HIGH();
        }
        task_sample_task_mutex_release();
    }
    else
    {
        meter_tx_buf[0] = 0xFF;                    // 错误标志
        meter_tx_buf[0] = 0xFF;                    // 错误标志
        meter_tx_buf[0] = POWER_CMD_STATUS_FAILED; // 错误标志
        SPI2_Slave_Send_IT(meter_tx_buf, SPI2_SLAVE_TX_LEN);
        M_INT_HIGH();
        return BSP_ERROR;
    }
    return BSP_OK;
}
/* Exported functions --------------------------------------------------------*/
