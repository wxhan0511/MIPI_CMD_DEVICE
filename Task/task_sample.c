/*
 * task_sample.c
 *
 *  Created on: Jun 30, 2025
 *      Author: Wenxiao Han
 */

#include "task_sample.h"
#include "usart.h"
#include "i2c.h"
#include "spi.h"
#include "power_control.h"
#include "bsp_mcp4728_ctl.h"
#include "bsp_ads1256_ctl.h"

osMutexId_t sample_mutex;
osStaticMutexDef_t sample_mutex_control_block;
const osMutexAttr_t sample_mutex_attributes = {
    .name = "show_mutex",
    .cb_mem = &sample_mutex_control_block,
    .cb_size = sizeof(sample_mutex_control_block),
};
static uint8_t rx_buf[64] = {0};
static uint8_t tx_buf[64] = {0};

SampleTask_S g_sample_task = {0};

uint8_t sample_vol_map[11][2] = {
        {3, 0xff},        //power:0,VCC → ch_index、d_trigger_ch_index       
        {4, 0xff}, // power:1,IOVCC → ch_index、d_trigger_ch_index
        {5, 0xff}, // power:2,VSP → ch_index、d_trigger_ch_index
        {6, 0xff}, // power:3,VSN → ch_index、d_trigger_ch_index
        {1, 7},       // power:4,AVDD → ch_index、d_trigger_ch_index
        {7, 0xff}, // power:5,VDD → ch_index、d_trigger_ch_index
        {1, 2},       // power:6,ELVDD → ch_index、d_trigger_ch_index
        {1, 3},        // power:7,ELVSS → ch_index、d_trigger_ch_index
        {1, 4},        // power:8,AD_I_BLAS_I → ch_index、d_trigger_ch_index
        {1, 5},        // power:9,AD_I_BLAS_V → ch_index、d_trigger_ch_index
        {1, 6},        // power:10,AD_I_BL → ch_index、d_trigger_ch_index
};
uint8_t sample_cur_map[15][2] = {
        {0, 0},  //power:0,VCC → ch_index、d_trigger_ch_index       
        {0, 1}, // power:1,IOVCC → ch_index、d_trigger_ch_index
        {0, 7}, // power:2,VSP → ch_index、d_trigger_ch_index
        {0, 3}, // power:3,VSN → ch_index、d_trigger_ch_index
        {0, 6}, // power:4,AVDD → ch_index、d_trigger_ch_index
        {0, 4},  // power:5,VDD → ch_index、d_trigger_ch_index
        {0, 5},  // power:6,ELVDD → ch_index、d_trigger_ch_index
        {0, 2},  // power:7,ELVSS → ch_index、d_trigger_ch_index
        {1,0},  // power:8,AD_V_V+ADJ → ch_index、d_trigger_ch_index
        {2,0},  // power:9,AD_R&D → ch_index、d_trigger_ch_index
        {2,2},  // power:10,AD_24PinV → ch_index、d_trigger_ch_index
        {2,4},  // power:11,AD_V_BLAS_I → ch_index、d_trigger_ch_index
        {2,5},  // power:12,BLAS_V → ch_index、d_trigger_ch_index
        {2,6},  // power:13,AD_V_BL → ch_index、d_trigger_ch_index
        {2,7},  // power:14,AD_V_V-ADJ → ch_index、d_trigger_ch_index    
};
void task_sample_run()
{
    //启动SPI2作为从机接收数据
    SPI2_SlaveFixed_InitAndStart();
    for (;;)
    {

        if (spi_rx_flag == 1)
        {
            spi_rx_flag = 0;
            HAL_NVIC_DisableIRQ(SPI2_IRQn);
            memcpy(rx_buf, spi2_rx_buf, sizeof(spi2_rx_buf));
            HAL_NVIC_EnableIRQ(SPI2_IRQn);
        }
        else
        {
            osDelay(10);
            continue;
        }
        memset(tx_buf, 0, sizeof(tx_buf));
        // 打印收到的数据

        for (uint32_t i = 0; i < sizeof(rx_buf); i++)
        {
            I2C_DEBUG("%02X ", rx_buf[i]);
            if ((i + 1) % 16 == 0)
            {
                I2C_DEBUG("\r\n");
            }
        }

        // 解析收到的数据

        g_sample_task.frame_header = rx_buf[0];
        g_sample_task.cmd_type = rx_buf[1];

        // tx和rx头一样
        tx_buf[0] = g_sample_task.frame_header;
        tx_buf[1] = g_sample_task.cmd_type;

        // 解析header
        if (g_sample_task.frame_header != 0xA0)
        {
            I2C_ERROR("receive data header: %x\r\n", g_sample_task.frame_header);
            break;
        }

        g_sample_task.cmd_status = POWER_CMD_STATUS_SUCCESS;
        switch (g_sample_task.cmd_type)
        {
        case GET_ID:
            tx_buf[2] = id; // 1,2,3,4:bit1~4
            break;
        case GET_SW_VERSION:
            tx_buf[2] = id;
            memcpy(&tx_buf[3], sw_version, 4);
            break;
        case VOL_SET:
        case LIM_SET:
            memcpy(&g_sample_task.set_power_data_frame, rx_buf, sizeof(g_sample_task.set_power_data_frame));
            I2C_DEBUG("set_power_data_frame.power_id:%x\r\n", g_sample_task.set_power_data_frame.power_id);

             *(dac_config_table[g_sample_task.set_power_data_frame.power_id].last_voltage) = g_sample_task.set_power_data_frame.value.float_value;
            
            bsp_cali_and_set_power(g_sample_task.set_power_data_frame.power_id);
            calibration_save();
        
            tx_buf[2] = id;
            tx_buf[3] = g_sample_task.cmd_status;
            break;
        case ALL_POWER_EN:
            memcpy(g_sample_task.power_switch, &rx_buf[2], 8);
            for(int i = 0; i < 8; i++)
            {
                if (g_sample_task.power_switch[i] == 0x01)
                    bsp_power_single_enable(i);
                else
                    bsp_power_single_disable(i);
            }
            tx_buf[2] = id;
            tx_buf[3] = BSP_OK;
            break;
        case SINGLE_POWER_EN:
            uint8_t power_id= rx_buf[2];
            uint8_t en = rx_buf[3];
            MIPI_CMD_DEBUG("power_id:%d, en:%d\r\n", power_id, en);
             if (power_id < 8)
            if (en == 0x01)
                bsp_power_single_enable(power_id);
            else
                bsp_power_single_disable(power_id);
            tx_buf[2] = id;
            tx_buf[3] = BSP_OK;
            break;
        case SINGLE_VOL_GET:
            uint8_t power_name = rx_buf[2];
            uint8_t ch_index = sample_vol_map[power_name][0];
            uint8_t d_trigger_ch_index = sample_vol_map[power_name][1];
            if (ch_index == 0) 
                bsp_ads1256_ch0_select(d_trigger_ch_index);
            else if (ch_index == 1)
                bsp_ads1256_ch1_select(d_trigger_ch_index);
            else if (ch_index == 2)
                bsp_ads1256_ch2_select(d_trigger_ch_index);
            while(latest_sample_ch_sel[ch_index] != d_trigger_ch_index) osDelay(1); // 等待通道切换完成(采样也完成)
            memcpy(&tx_buf[3], &latest_sample_data[ch_index], 4);
            tx_buf[2] = id;
            break;
        case SINGLE_CUR_GET:
            uint8_t power_name = rx_buf[2];
            uint8_t ch_index = sample_cur_map[power_name][0];
            uint8_t d_trigger_ch_index = sample_cur_map[power_name][1];
            if (ch_index == 0) 
                bsp_ads1256_ch0_select(d_trigger_ch_index);
            else if (ch_index == 1)
                bsp_ads1256_ch1_select(d_trigger_ch_index);
            else if (ch_index == 2)
                bsp_ads1256_ch2_select(d_trigger_ch_index);
            while(latest_sample_ch_sel[ch_index] != d_trigger_ch_index) osDelay(1); // 等待通道切换完成(采样也完成)
            memcpy(&tx_buf[3], &latest_sample_data[ch_index], 4);
            tx_buf[2] = id;
            break;
        default:
            I2C_DEBUG("unknow command\r\n");
            break;
        }
        // 打印发送的数据
#ifdef I2C_DEBUG_ENABLE
        for (uint32_t i = 0; i < sizeof(tx_buf); i++)
        {
            printf("%02X ", tx_buf[i]);
            if ((i + 1) % 16 == 0)
            {
                printf("\r\n");
            }
        }
#endif
        osDelay(100);
    }
}

void task_sample_init(void)
{
    task_sample_handle = osThreadNew(task_sample_run, NULL, &task_sample_attributes);
    if (task_sample_handle == NULL)
    {
    }
}
