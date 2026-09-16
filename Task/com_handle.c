/**
 * @brief Communication command handling module
 */

/* ==================== 1. Includes ==================== */
#include <stdio.h>
#include <string.h>
#include "com_handle.h"
#include "task_com.h"
#include "spi.h"
#include "boot_flag.h"
#include "tim.h"
#include "bsp_mcp4728_ctl.h"

/* ==================== 2. Macros ==================== */
/* None */

/* ==================== 3. Type definitions (structs, enums, aliases) ==================== */
/* None */

/* ==================== 4. External global variables ==================== */
/* Declared centrally in com_handle.h */
extern volatile uint32_t g_spi2_err_code; /* Captured in HAL_SPI_ErrorCallback (spi.c) */

/* ==================== 5. Static private variables ==================== */
/* None */

/* ==================== 6. Static function declarations ==================== */
/* None */

/* ==================== 7. Public function implementations ==================== */
BSP_STATUS com_handle_spi(const uint8_t *rx, uint8_t *tx)
{
    (void)tx;

    // Determine the mode from the frame header
    if (rx[0] == 0xA0)
    {
        const uint8_t request_cmd = rx[1];

        printf("COM enter: cmd=0x%02X\r\n", request_cmd);
        task_sample_task_mutex_acquire(); // Acquire the sample task mutex to stop the communication task; it resumes after the sample task finishes processing the command
        {
            // Process the received data
            g_sample_task.frame_header = rx[0];
            g_sample_task.cmd_type = request_cmd;
            printf("rx g_sample_task.cmd_type:%x\r\n", g_sample_task.cmd_type);
            g_sample_task.cmd_status = POWER_CMD_STATUS_SUCCESS;
            // Process the command --------------------
            printf("COM: waiting for sample task\r\n");
            task_com_suspend();
            printf("COM: sample task completed\r\n");
            // Prepare the data to send
            meter_tx_buf[0] = rx[0];
            meter_tx_buf[1] = request_cmd;
            printf("response cmd_type:%x\r\n", request_cmd);
            // Send the data to the host
            meter_com_flag = 0;
            spi2_enter_boot_pending = (request_cmd == CMD_ENTER_BOOT) ? 1U : 0U;
            HAL_StatusTypeDef status =
                SPI2_Slave_Send_IT(meter_tx_buf, SPI2_SLAVE_TX_LEN);

            printf("SPI TX start: status=%d state=%d error=0x%08lX\r\n",
                   status, hspi2.State, hspi2.ErrorCode);

            if (status == HAL_OK)
            {
                M_INT_HIGH();

                if (request_cmd == CMD_ENTER_BOOT)
                {
                    uint32_t ack_wait_start = osKernelGetTickCount();
                    while (!spi2_tx_complete_flag &&
                           (osKernelGetTickCount() - ack_wait_start < 1000U))
                    {
                        osDelay(1);
                    }
                    printf("BOOT reset: tx_done=%u\r\n", spi2_tx_complete_flag);

                    /* Keep the communication path alive until the host has
                       clocked out the ACK. Shut down only the DUT power rails;
                       the level shifter is still needed by the bootloader. */
                    disableTim1PWMOutput();
                    disableTim2PWMOutput();
                    bsp_power_all_disable();
                    osDelay(10);
                    Boot_RequestUpgrade();
                }
            }
            else
            {
                spi2_enter_boot_pending = 0;
            }
        }
        task_sample_task_mutex_release();
    }
    else
    {
        meter_tx_buf[0] = POWER_CMD_STATUS_FAILED; // Error flag
        SPI2_Slave_Send_IT(meter_tx_buf, SPI2_SLAVE_TX_LEN);
        printf("slave send IT start send M_INT_HIGH,not a0\r\n");
        M_INT_HIGH();
        return BSP_ERROR;
    }
    return BSP_OK;
}

/* ==================== 8. Static private function implementations ==================== */
/* None */
