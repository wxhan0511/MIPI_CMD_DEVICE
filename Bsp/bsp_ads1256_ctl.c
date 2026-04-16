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
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

void raw_data_queue_push(float value, uint8_t index)
{

    raw_data_queue[raw_data_queue_head] = value; //raw data
    raw_data_index_queue[raw_data_queue_head] = index; //channel index
    raw_data_ch_sel_queue[raw_data_queue_head] = index == 0 ? ch0_flag : index == 1 ? ch1_flag : index == 2 ? ch2_flag : 0;
    latest_sample_raw_data[index] = value;//latest sample data for  8 channel
    latest_sample_ch_sel[index] = index == 0 ? ch0_flag : index == 1 ? ch1_flag : index == 2 ? ch2_flag : 0;//latest sample channel sel for  8 channel   
    if(ch0_flag == 8 && ch1_flag ==8 && ch2_flag ==8) MIPI_CMD_ERROR("ads1256 ch0~2 not selected , not connected!\r\n");
    
    raw_data_queue_head = (raw_data_queue_head + 1) % RAW_DATA_QUEUE_SIZE;

    sample_data_cali();
}

void sample_data_cali()
{

    for (uint8_t i = 0; i < 8; i++)
    {
      sel_cali_param(i , latest_sample_ch_sel[i], &offset, &gain);

      //ch0(all,2-,3-),ch1(0,5),ch2(0,2,5,6,7-)
      //VOL
      IV_data = latest_sample_raw_data[i] * gain + offset; 
          
      latest_sample_data[i] = IV_data * 1000;
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
