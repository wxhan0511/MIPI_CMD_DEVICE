/**
 * @file       bsp_ads1256_ctl.c
 * @brief      MIPI_CMD ADS1256 control module, including raw data queue management, calibration parameter selection, and sample data processing
 * @author     wxhan
 * @version    1.0.0
 * @date       2026-01-29
 * @copyright  Copyright (c) 2026 gcoreinc
 * @license    MIT License
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_ads1256_ctl.h"
#include <stdio.h>
#include <string.h>
#include "bsp_channel_sel.h"
#include "calibration_utils.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint8_t channel_num = 0;
static float offset, gain, IV_data = 0.0f;
float raw_data_queue[RAW_DATA_QUEUE_SIZE] __attribute__((section(".raw_data_queue"))) __attribute__((aligned(4))); // align 4B , size 4092
uint8_t raw_data_index_queue[RAW_DATA_INDEX_QUEUE_SIZE] __attribute__((section(".raw_data_index_queue")));
uint8_t raw_data_ch_sel_queue[RAW_DATA_INDEX_QUEUE_SIZE] __attribute__((section(".raw_data_index_queue")));
volatile uint16_t raw_data_queue_head = 0;
volatile float latest_sample_raw_data[8] = {0};
volatile uint8_t latest_sample_ch_sel[8] = {0};
volatile double raw_data = 0;
volatile float latest_sample_data[8] = {0};
volatile uint8_t latest_sample_index[8] = {0};
float cali_data = 0;
extern R_D_MODE r_d_mode;
extern TEST_R_D_RES_LEVEL r_level_selected;
volatile TEST_R_D_RES_LEVEL cal_r = OHM_NULL;
volatile uint8_t r_en = 0;
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

void raw_data_queue_push(float value, uint8_t index)
{

    raw_data_queue[raw_data_queue_head] = value;       // raw data
    raw_data_index_queue[raw_data_queue_head] = index; // channel index
    raw_data_ch_sel_queue[raw_data_queue_head] = index == 0 ? ch0_flag : index == 1 ? ch1_flag
                                                                     : index == 2   ? ch2_flag
                                                                                    : 0;
    latest_sample_raw_data[index] = value; // latest sample data for  8 channel
    latest_sample_ch_sel[index] = index == 0 ? ch0_flag : index == 1 ? ch1_flag
                                                      : index == 2   ? ch2_flag
                                                                     : 0; // latest sample channel sel for  8 channel

    raw_data_queue_head = (raw_data_queue_head + 1) % RAW_DATA_QUEUE_SIZE;

    sample_data_cali();
}

void sample_data_cali()
{
    float rt_value = 0xffffff;

    for (uint8_t i = 0; i < 8; i++)
    {
        sel_cali_param(i, latest_sample_ch_sel[i], &offset, &gain);

        if (r_d_mode == R_MODE && i == 2 && latest_sample_ch_sel[i] == 0)
        {

            
            // R=VoRt/(0.5-Vo)  mv
    // OHM_NULL = 0,
    // OHM_10_M,
    // OHM_1_M,
    // OHM_100_K,
    // OHM_10_K,
    // OHM_1_K,
    // OHM_100_OHM,
    // OHM_4_point_7_K,
            cal_r = r_level_selected;
            if (r_level_selected == OHM_10_M)
                rt_value = 10000.0f;
            else if (r_level_selected == OHM_1_M)
                rt_value = 1000.0f;
            else if (r_level_selected == OHM_100_K)
                rt_value = 100.0f;
            else if (r_level_selected == OHM_10_K)
                rt_value = 10.0f;
            else if (r_level_selected == OHM_1_K)
                rt_value = 1.0f;
            else if (r_level_selected == OHM_100_OHM)
                rt_value = 0.1f;
            else if (r_level_selected == OHM_4_point_7_K)
                rt_value = 4.7f;
            if(latest_sample_raw_data[i] >= 0.27160f)
            {
                //printf("Warning: raw data %f may be out of range for resistance calculation\r\n", latest_sample_raw_data[i]);
                latest_sample_data[i] = 999999;//防止除数为0或者负数
            }
            else
            {
                latest_sample_data[i] = (latest_sample_raw_data[i]*rt_value*1000)/(0.27-latest_sample_raw_data[i]);
                printf("cal_r: %d, rt_value: %f m, raw data: %f, resistance: %f m\r\n", cal_r, rt_value/1000, latest_sample_raw_data[i], latest_sample_data[i]/1000000);
                
            }
            cali_data = latest_sample_data[i] / 1000;//转换成k
            
            if(cali_data > 0.01 && cali_data <= 0.1 && r_level_selected != OHM_100_OHM)
            {
                M_SPI_DEBUG("change gear 100 ohm\r\n");
                bsp_rd_select_r_level(OHM_100_OHM);
            }
            else if(cali_data > 0.1 && cali_data <= 1 && r_level_selected != OHM_1_K)
            {
                M_SPI_DEBUG("change gear 1k ohm\r\n");
                bsp_rd_select_r_level(OHM_1_K);
            }
            else if(cali_data > 1 && cali_data <= 10 && r_level_selected != OHM_10_K)
            {
                M_SPI_DEBUG("change gear 10k ohm\r\n");
                bsp_rd_select_r_level(OHM_10_K);    
            }
            else if(cali_data > 10 && cali_data <= 100 && r_level_selected != OHM_100_K)
            {
                M_SPI_DEBUG("change gear 100k ohm\r\n");
                bsp_rd_select_r_level(OHM_100_K);
            }
            else if(cali_data > 100 && cali_data <= 1000 && r_level_selected != OHM_1_M)
            {
                M_SPI_DEBUG("change gear 1M ohm\r\n");
                bsp_rd_select_r_level(OHM_1_M);
            }
            else if(cali_data > 1000 && cali_data <= 10000 && r_level_selected != OHM_10_M)
            {
                M_SPI_DEBUG("change gear 10M ohm\r\n");
                bsp_rd_select_r_level(OHM_10_M);
            }
        }
        else
        {
            IV_data = latest_sample_raw_data[i] * gain + offset;
            latest_sample_data[i] = IV_data * 1000; 
        }
        
        //printf("channel %d, raw data %f, cali data %f\r\n", i, latest_sample_raw_data[i], latest_sample_data[i]);
    }
}
/**
 * @brief get the index value from the ring buffer
 * @param index
 * @return
 */
uint8_t raw_data_queue_get_index(uint16_t index)
{
    if (0 <= index && index < RAW_DATA_INDEX_QUEUE_SIZE)
    {
        return raw_data_index_queue[index];
    }
    else if (index < 0)
    {
        return raw_data_index_queue[RAW_DATA_INDEX_QUEUE_SIZE + index];
    }
    else
    {
        return 0;
    }
}

/**
 * @brief 获取环形队列中的数据
 * @param index 索引位置
 * @return 队列中指定索引位置的数据
 */
float raw_data_queue_get_data(uint16_t index)
{
    if (0 <= index && index < RAW_DATA_QUEUE_SIZE)
    {
        return raw_data_queue[index];
    }
    else if (index < 0)
    {
        return raw_data_queue[RAW_DATA_QUEUE_SIZE + index];
    }
    else
    {
        return 0.0f;
    }
}

/* Exported functions --------------------------------------------------------*/
