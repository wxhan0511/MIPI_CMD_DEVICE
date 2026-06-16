/**
 * @brief 通信命令处理模块
 */

/* ==================== 1. 头文件包含 ==================== */
#include <stdio.h>
#include <string.h>
#include "com_handle.h"
#include "task_com.h"
#include "spi.h"

/* ==================== 2. 宏定义 ==================== */
/* 无 */

/* ==================== 3. 类型定义（结构体、枚举、别名） ==================== */
/* 无 */

/* ==================== 4. 外部全局变量 ==================== */
/* 由 com_handle.h 统一声明 */

/* ==================== 5. 静态私有变量 ==================== */
/* 无 */

/* ==================== 6. 静态函数声明 ==================== */
/* 无 */

/* ==================== 7. 外部可调用函数实现 ==================== */
BSP_STATUS com_handle_spi(const uint8_t *rx, uint8_t *tx)
{
    (void)tx;

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

/* ==================== 8. 静态私有函数实现 ==================== */
/* 无 */
