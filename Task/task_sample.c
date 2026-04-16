/*
 * task_sample.c
 *
 *  Created on: Jun 30, 2025
 *      Author: Wenxiao Han
 */
/*主机发 CMD（写事务）
从机开始处理，READY=0（或 BUSY=1）
从机数据准备完成，置 READY=1（或触发中断脚）
主机检测到 READY 后，再发 READ_CMD + dummy 读回数据（读事务）
所有读写都带超时与重试*/
#include "task_sample.h"
#include "usart.h"
#include "i2c.h"
#include "spi.h"
#include "power_control.h"
#include "bsp_mcp4728_ctl.h"
#include "bsp_ads1256_ctl.h"
#include "spi.h"
#include "bsp_calibration.h"
#include "bsp_channel_sel.h"
#include "utils.h"

enum
{
    PROTO_GET_ID = 0,
    PROTO_GET_VERSION,
    PROTO_SET_VOLTAGE,
    PROTO_SET_CURRENT_LIM,
    PROTO_SET_ALL_POWER_EN,
    PROTO_SET_POWER_EN,
    PROTO_GET_VOLTAGE,
    PROTO_GET_CURRENT,
    PROTO_SET_ALL_POWER_VOLTAGE,
    PROTO_SET_ALL_POWER_CURRENT_LIM,
    PROTO_CMD_COUNT
};

static const protocol_header_t PROTOCOL_HEADERS[PROTO_CMD_COUNT] =
{
    [PROTO_GET_ID]                    = {0xA0, 0x10}, // 获取 ID
    [PROTO_GET_VERSION]               = {0xA0, 0x11}, // 获取软件版本号
    [PROTO_SET_VOLTAGE]               = {0xA0, 0x12}, // 电压配置
    [PROTO_SET_CURRENT_LIM]           = {0xA0, 0x13}, // 限流配置
    [PROTO_SET_ALL_POWER_EN]          = {0xA0, 0x14}, // 所有电源使能
    [PROTO_SET_POWER_EN]              = {0xA0, 0x15}, // 单路电源使能
    [PROTO_GET_VOLTAGE]               = {0xA0, 0x16}, // 单路电源电压获取
    [PROTO_GET_CURRENT]               = {0xA0, 0x17}, // 单路电源电流获取
    [PROTO_SET_ALL_POWER_VOLTAGE]     = {0xA0, 0x18}, // 所有电源电压配置
    [PROTO_SET_ALL_POWER_CURRENT_LIM] = {0xA0, 0x19}, // 所有电源限流配置
};

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

uint8_t sample_cur_map[11][2] = {
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
uint8_t sample_vol_map[15][2] = {
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


    SPI2_Slave_StartRx_IT();   // 启动SPI2从机64字节接收
    M_INT_HIGH();
    uint32_t t0=0;
    uint8_t power_id = 0;
    uint8_t en =0;
    uint8_t sample_vol_id= 0;
    uint8_t sample_cur_id= 0;
    uint8_t ads1256_ch_index = 0;
    uint8_t d_trigger_ch_index = 0;
    float vol =0;
    float cur=0;
    float ref_freq_vol = 3300;
    float_bytes_t float_bytes = {0};
    uint8_t pin_n=0;
    uint8_t pin_p=0;
    uint8_t pin_num=0;
    R_D_MODE rd_mode = R_D_MODE_NULL;
    TEST_R_D_RES_LEVEL r_level = OHM_NULL;
    static const uint8_t power_order[8] = {0, 1, 2, 3, 8, 9, 10, 11};
    for (;;)
    {
         if (spi_rx_flag == 1)
        {
            memcpy(rx_buf, g_spi2_rx_buf, sizeof(rx_buf));
//-------------------------------------------------------------------------------------------------            
            memset(tx_buf, 0, sizeof(tx_buf));
            // 打印收到的数据
            printf("receive\r\n");
            for(int i = 0; i < SPI2_SLAVE_RX_LEN; i++) {
                M_SPI_DEBUG("%02X ", rx_buf[i]);
                if ((i + 1) % 16 == 0) {
                    M_SPI_DEBUG("\r\n");
                }
            }
            // 解析收到的数据
            if (rx_buf[0] != 0xA0) {  //SPI溢出错误,没有去读SPI发的数据
                g_spi2_tx_buf[0] = 0xFF; // 错误标志
                g_spi2_tx_buf[0] = 0xFF; // 错误标志
                g_spi2_tx_buf[0] = POWER_CMD_STATUS_FAILED; // 错误标志
                SPI2_Slave_Send_IT(g_spi2_tx_buf, SPI2_SLAVE_TX_LEN);
                M_INT_HIGH();
                break;
            }

            g_sample_task.frame_header = rx_buf[0];
            g_sample_task.cmd_type = rx_buf[1];


            // tx和rx头一样
            tx_buf[0] = g_sample_task.frame_header;
            tx_buf[1] = g_sample_task.cmd_type;

            // 解析header
            if (g_sample_task.frame_header != 0xA0)
            {
                M_SPI_ERROR("erro,receive data header: %x\r\n", g_sample_task.frame_header);
                
            }
            static const uint8_t power_order[8] = {0, 1, 2, 3, 8, 9, 10, 11};
            g_sample_task.cmd_status = POWER_CMD_STATUS_SUCCESS;
            switch (g_sample_task.cmd_type)
            {
            case GET_ID:
                tx_buf[3] = id; // 1,2,3,4:bit1~4
                break;
            case GET_SW_VERSION:
                memcpy(&tx_buf[3], sw_version, 4);
                break;
            case VOL_SET:
            case LIM_SET:
                g_sample_task.set_power_data_frame.frame_header = rx_buf[0];
                g_sample_task.set_power_data_frame.cmd_type = rx_buf[1];
                g_sample_task.set_power_data_frame.power_id = rx_buf[2];
                memcpy(&float_bytes, &rx_buf[3], sizeof(float_bytes));
                memcpy(g_sample_task.set_power_data_frame.value.bytes, float_bytes.b, sizeof(float_bytes.b));
                g_sample_task.set_power_data_frame.power_id = power_order[g_sample_task.set_power_data_frame.power_id];
                M_SPI_DEBUG("set_power_data_frame.power_id:%x\r\n", g_sample_task.set_power_data_frame.power_id);
                M_SPI_DEBUG("set_power_data_frame.value.bytes:%02X %02X %02X %02X\r\n",
                            float_bytes.b[0], float_bytes.b[1], float_bytes.b[2], float_bytes.b[3]);
                M_SPI_DEBUG("set_power_data_frame.value.float_value:%f\r\n", float_bytes.f);

                *(dac_config_table[g_sample_task.set_power_data_frame.power_id].last_voltage) = float_bytes.f;
                
                bsp_cali_and_set_power(g_sample_task.set_power_data_frame.power_id);
                //calibration_save();
                tx_buf[2] = g_sample_task.cmd_status;
                break;
            case ALL_POWER_EN:
                memcpy(g_sample_task.power_switch, &rx_buf[2], 8);
                for(uint8_t i=0;i<8;i++)
                {
                    M_SPI_DEBUG("power_switch[%d]: %d\r\n", i, g_sample_task.power_switch[i]);
                }
                
                for (uint8_t n = 0; n < 8; n++)
                {
                    uint8_t power_id = power_order[n];

                    if (g_sample_task.power_switch[n] == 0x01)
                        bsp_power_single_enable(power_id);
                    else
                        bsp_power_single_disable(power_id);
                }
                break;
            case SINGLE_POWER_EN:
                power_id= power_order[rx_buf[2]];

                en = rx_buf[3];
                MIPI_CMD_DEBUG("power_id:%d, en:%d\r\n", power_id, en);
                
                power_id = power_order[power_id];
                if (en == 0x01)
                    bsp_power_single_enable(power_id);
                else
                    bsp_power_single_disable(power_id);
                break;
            case SINGLE_VOL_GET:
                M_SPI_DEBUG("SINGLE_VOL_GET\r\n");
                sample_vol_id= rx_buf[2];

                ads1256_ch_index = sample_vol_map[sample_vol_id][0];
                d_trigger_ch_index = sample_vol_map[sample_vol_id][1];
                if (ads1256_ch_index == 0 && d_trigger_ch_index != 0xff) 
                    bsp_ads1256_ch0_select(d_trigger_ch_index);
                else if (ads1256_ch_index == 1 && d_trigger_ch_index != 0xff)
                    bsp_ads1256_ch1_select(d_trigger_ch_index);
                else if (ads1256_ch_index == 2 && d_trigger_ch_index != 0xff)
                    bsp_ads1256_ch2_select(d_trigger_ch_index);
                if(ads1256_ch_index < 3 && d_trigger_ch_index != 0xff)
                {
                    t0 = HAL_GetTick();
                    while (latest_sample_ch_sel[ads1256_ch_index] != d_trigger_ch_index)
                    {
                        if ((HAL_GetTick() - t0) >= 1000U)  // 最多等待1s
                        {
                            // 可按你的状态定义改成超时状态
                            g_sample_task.cmd_status = POWER_CMD_STATUS_TIMEOUT;
                            M_SPI_INFO("SINGLE_VOL_GET timeout\r\n");
                            break;
                        }
                        osDelay(1);
                    }
                }
                M_SPI_INFO("ads1256_ch_index: %d, d_trigger_ch_index: %d, latest_sample_ch_sel: %d\r\n", ads1256_ch_index, d_trigger_ch_index, latest_sample_ch_sel[ads1256_ch_index]);
                memcpy(&tx_buf[3], (const void *)&latest_sample_data[ads1256_ch_index], sizeof(float));
                M_SPI_DEBUG( "SINGLE_VOL_GET: channel %d, voltage %f\r\n", ads1256_ch_index, latest_sample_data[ads1256_ch_index]);
                M_SPI_DEBUG("latest_sample_raw_data: %f\r\n", latest_sample_raw_data[ads1256_ch_index]);
                break;
            case SINGLE_CUR_GET:
                sample_cur_id= rx_buf[2];

                ads1256_ch_index = sample_cur_map[sample_cur_id][0];
                d_trigger_ch_index = sample_cur_map[sample_cur_id][1];
                if (ads1256_ch_index == 0 && d_trigger_ch_index != 0xff) 
                    bsp_ads1256_ch0_select(d_trigger_ch_index);
                else if (ads1256_ch_index == 1 && d_trigger_ch_index != 0xff)
                    bsp_ads1256_ch1_select(d_trigger_ch_index);
                else if (ads1256_ch_index == 2 && d_trigger_ch_index != 0xff)
                    bsp_ads1256_ch2_select(d_trigger_ch_index);
                if(ads1256_ch_index < 3 && d_trigger_ch_index != 0xff)
                {
                    t0 = HAL_GetTick();
                    while (latest_sample_ch_sel[ads1256_ch_index] != d_trigger_ch_index)
                    {
                        if ((HAL_GetTick() - t0) >= 1000U)  // 最多等待1s
                        {
                            // 可按你的状态定义改成超时状态
                            g_sample_task.cmd_status = POWER_CMD_STATUS_TIMEOUT;
                            M_SPI_INFO("SINGLE_CUR_GET timeout\r\n");
                            break;
                        }
                        osDelay(1);
                    }
                }
                M_SPI_INFO("ads1256_ch_index: %d, d_trigger_ch_index: %d, latest_sample_ch_sel: %d\r\n", ads1256_ch_index, d_trigger_ch_index, latest_sample_ch_sel[ads1256_ch_index]);
                memcpy(&tx_buf[3], (const void *)&latest_sample_data[ads1256_ch_index], sizeof(float));
                M_SPI_DEBUG( "SINGLE_CUR_GET: channel %d, current %f\r\n", ads1256_ch_index, latest_sample_data[ads1256_ch_index]);
                M_SPI_DEBUG("latest_sample_raw_data: %f\r\n", latest_sample_raw_data[ads1256_ch_index]);
                break;
            case GET_FREQUENCY:
                pin_num = rx_buf[2];
                memcpy(&ref_freq_vol,&rx_buf[3],sizeof(float));

                bsp_select_24pin_channel(pin_num);
                g_calibration_manager.data.ref_freq_last = ref_freq_vol/2;
                bsp_cali_and_set_power(17);
                test_ccp();
                break;
            case GET_RESISTANCE:
                pin_p = rx_buf[2];
                pin_n = rx_buf[3];
                r_level = rx_buf[4];
                bsp_rd_select_r_level(r_level);
                bsp_rd_select_pin(pin_p, pin_n);
                bsp_rd_select_mode(R_MODE);
                break;
            case GET_DIODE:
                pin_p = rx_buf[2];
                pin_n = rx_buf[3];
                r_level = rx_buf[4];
                bsp_rd_select_r_level(r_level);
                bsp_rd_select_pin(pin_p, pin_n);
                bsp_rd_select_mode(D_MODE);
                break;
            case SET_RESISTANCE:
                r_level = rx_buf[2];
                bsp_rd_select_r_level(r_level);
                break;
            case SET_FREQUENCY:
                
                break;
            case GET_24PIN_VOLTAGE:
                M_SPI_DEBUG("GET_24PIN_VOLTAGE\r\n");
                pin_num = rx_buf[2];
                bsp_select_24pin_channel(pin_num);
                ads1256_ch_index = 2;
                d_trigger_ch_index = 3;
                if (ads1256_ch_index == 0 && d_trigger_ch_index != 0xff) 
                    bsp_ads1256_ch0_select(d_trigger_ch_index);
                else if (ads1256_ch_index == 1 && d_trigger_ch_index != 0xff)
                    bsp_ads1256_ch1_select(d_trigger_ch_index);
                else if (ads1256_ch_index == 2 && d_trigger_ch_index != 0xff)
                    bsp_ads1256_ch2_select(d_trigger_ch_index);
                if(ads1256_ch_index < 3 && d_trigger_ch_index != 0xff)
                {
                    t0 = HAL_GetTick();
                    while (latest_sample_ch_sel[ads1256_ch_index] != d_trigger_ch_index)
                    {
                        if ((HAL_GetTick() - t0) >= 1000U)  // 最多等待1s
                        {
                            // 可按你的状态定义改成超时状态
                            g_sample_task.cmd_status = POWER_CMD_STATUS_TIMEOUT;
                            M_SPI_INFO("SINGLE_VOL_GET timeout\r\n");
                            break;
                        }
                        osDelay(1);
                    }
                }
                M_SPI_INFO("ads1256_ch_index: %d, d_trigger_ch_index: %d, latest_sample_ch_sel: %d\r\n", ads1256_ch_index, d_trigger_ch_index, latest_sample_ch_sel[ads1256_ch_index]);
                memcpy(&tx_buf[3], (const void *)&latest_sample_data[ads1256_ch_index], sizeof(float));

                M_SPI_DEBUG( "SINGLE_VOL_GET: channel %d, voltage %f\r\n", ads1256_ch_index, latest_sample_data[ads1256_ch_index]);
                break;
            case SEL_PIN_24:
                pin_num = rx_buf[2];
                bsp_select_24pin_channel(pin_num);
                break;
            case SEL_PIN_PN:
                pin_p = rx_buf[2];
                pin_n = rx_buf[3];
                bsp_rd_select_pin(pin_p, pin_n);
                break;
            default:
                M_SPI_DEBUG("unknow command\r\n");
                break;
            }
            printf("send\r\n");
            
            // float mock_vol = 3.1415926; 
            // memcpy(&tx_buf[3], &mock_vol, sizeof(mock_vol));
            // g_sample_task.cmd_status = POWER_CMD_STATUS_SUCCESS;

            tx_buf[2] = g_sample_task.cmd_status;
            for(uint32_t i = 0; i < sizeof(tx_buf); i++)
            {
                M_SPI_DEBUG("%02X ", tx_buf[i]);
                if ((i + 1) % 16 == 0)
                {
                    M_SPI_DEBUG("\r\n");
                }
            }


//---------------------------------------------------------------------------------------------------
            memset(rx_buf, 0, sizeof(rx_buf));
            memcpy(g_spi2_tx_buf, tx_buf, sizeof(tx_buf));
            spi_rx_flag = 0;
            SPI2_Slave_Send_IT(g_spi2_tx_buf, SPI2_SLAVE_TX_LEN);
            M_INT_HIGH();
        }
        else
        {
            osDelay(10);
            continue;
        }
        osDelay(100);
    }
}

void task_sample_init(void)
{
    task_sample_handle = osThreadNew(task_sample_run, NULL, &task_sample_attributes);
}
