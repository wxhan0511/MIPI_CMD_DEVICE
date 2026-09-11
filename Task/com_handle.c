/**
 * @brief Communication command handling module
 */

/* ==================== 1. Includes ==================== */
#include <stdio.h>
#include <string.h>
#include "com_handle.h"
#include "task_com.h"
#include "spi.h"

/* ==================== 2. Macros ==================== */
/* None */

/* ==================== 3. Type definitions (structs, enums, aliases) ==================== */
/* None */

/* ==================== 4. External global variables ==================== */
/* Declared centrally in com_handle.h */

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
        task_sample_task_mutex_acquire(); // Acquire the sample task mutex to stop the communication task; it resumes after the sample task finishes processing the command
        {
            // Process the received data
            g_sample_task.frame_header = meter_rx_buf[0];
            g_sample_task.cmd_type = meter_rx_buf[1];
            g_sample_task.cmd_status = POWER_CMD_STATUS_SUCCESS;
            // Process the command --------------------
            task_sample_suspend();
            task_sample_resume();
            task_com_suspend(); // Suspend the communication task; task_com_resume() is called to resume it once the sample task has finished processing the command
            // Prepare the data to send
            meter_tx_buf[0] = g_sample_task.frame_header;
            meter_tx_buf[1] = g_sample_task.cmd_type;
            // Send the data to the host
            meter_com_flag = 0;
            SPI2_Slave_Send_IT(meter_tx_buf, SPI2_SLAVE_TX_LEN);
            M_INT_HIGH();
        }
        task_sample_task_mutex_release();
    }
    else
    {
        meter_tx_buf[0] = POWER_CMD_STATUS_FAILED; // Error flag
        SPI2_Slave_Send_IT(meter_tx_buf, SPI2_SLAVE_TX_LEN);
        M_INT_HIGH();
        return BSP_ERROR;
    }
    return BSP_OK;
}

/* ==================== 8. Static private function implementations ==================== */
/* None */
