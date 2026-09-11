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
#include "bsp_ads1256.h"
#include <stdio.h>
#include <string.h>
#include "bsp_channel_sel.h"
#include "calibration_utils.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
float offset, gain, IV_data = 0.0f;
static float raw_data_queue[RAW_DATA_QUEUE_SIZE] __attribute__((section(".raw_data_queue"))) __attribute__((aligned(4))); // align 4B , size 4092
static uint8_t raw_data_index_queue[RAW_DATA_INDEX_QUEUE_SIZE] __attribute__((section(".raw_data_index_queue")));
static uint8_t raw_data_ch_sel_queue[RAW_DATA_INDEX_QUEUE_SIZE] __attribute__((section(".raw_data_index_queue")));
volatile uint16_t raw_data_queue_head = 0;
volatile float latest_sample_raw_data[8] = {0};
volatile uint8_t latest_sample_ch_sel[8] = {0};
volatile double raw_data = 0;
volatile float latest_sample_data[8] = {0};
static volatile uint8_t latest_sample_index[8] = {0};
static double cali_data = 0;
extern R_D_MODE r_d_mode;
extern ads1256_dev_t dev_vol;

static __IO double cali_r_value[7][3] = {
    {0.000000621013436, 1.269563854760952, 128545.457705873996019}, // 10M
    {0.000002878590314, 1.612767830900360, 16471.438778921728954},  // 1M
    {0.000029345393638, 1.610968251555477, 1883.541990776575403},   // 100K
    {0.000213147484506, 1.838968199598394, 30.062618869735161},     // 10K
    {0.002992970571149, 1.597308304947589, 15.483768962910290},     // 1K
    {0.036816118668450, 1.779873763228085, -2.619710236117498},     // 91
    {0, 0, 0},                                                      // 1
};
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/* One round takes about 140 ms */
int wait_adc_one_round(uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();

    // 1) First wait for step_cnt to leave the "previous round end state (6)" to avoid a stale state
    while (dev_vol.step_cnt == 6)
    {
        if ((HAL_GetTick() - t0) >= timeout_ms)
            return -1;
        bsp_delay_ms(1);
    }

    // 2) Then wait for it to return to 6, which means one full round is complete
    while (dev_vol.step_cnt != 6)
    {
        if ((HAL_GetTick() - t0) >= timeout_ms)
            return -1;
        bsp_delay_ms(1);
    }

    return 0;
}
void raw_data_queue_push(float value, uint8_t index)
{

    raw_data_queue[raw_data_queue_head] = value;       // raw data
    raw_data_index_queue[raw_data_queue_head] = index; // channel index
    raw_data_ch_sel_queue[raw_data_queue_head] = index == 0 ? ch0_flag : index == 1 ? ch1_flag
                                                                     : index == 2   ? ch2_flag
                                                                                    : 0xff;
    latest_sample_raw_data[index] = value; // latest sample data for  8 channel
    latest_sample_ch_sel[index] = index == 0 ? ch0_flag : index == 1 ? ch1_flag
                                                      : index == 2   ? ch2_flag
                                                                     : 0xff; // latest sample channel sel for  8 channel

    raw_data_queue_head = (raw_data_queue_head + 1) % RAW_DATA_QUEUE_SIZE;

    sample_data_cali();
}

double bsp_adc_r_convert(const TEST_R_D_RES_LEVEL gear, const double input, const uint8_t cali_en)
{
    printf("bsp_adc_r_convert: gear %d, input %f, cali_en %d\r\n", gear, input, cali_en);
    double output = 0;
    switch (gear)
    {
    case OHM_10_M:
        output = input * 10000 * 1000 / (0.5 - input);
        break;
    case OHM_1_M:
        output = input * 1000 * 1000 / (0.5 - input);
        break;
    case OHM_100_K:
        output = input * 100 * 1000 / (0.5 - input);
        break;
    case OHM_10_K:
        output = input * 10 * 1000 / (0.5 - input);
        break;
    case OHM_1_K:
        output = input * 1000 / (0.5 - input);
        break;
    case OHM_100_OHM:
        output = input * 91 / (0.5 - input);
        break;
    case OHM_4_point_7_K:
        output = input / (0.5 - input);
        break;
    default:;
    }
    printf("raw resistance: %f ohm\r\n", output);
    if (cali_en == 1)
    {
        output = output * output * cali_r_value[gear - 1][0] + output * cali_r_value[gear - 1][1] + cali_r_value[gear - 1][2];
    }

    // output = output * 0.7346 + 22.908;
    // output  = output * output * 0.00009 + output*0.853 + 5.7802;
    return output;
}
void sample_data_cali()
{
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
            const double raw_data = latest_sample_raw_data[i] * 1000000.0;
            cali_data = bsp_adc_r_convert(dev_vol.sample_res_gear_rd, (raw_data - 1660) / 1000000, dev_vol.res_cali_en);
            printf("raw data: %f, cali data: %f ohm, gear: %d, cali_en: %d\r\n", latest_sample_raw_data[i], cali_data, dev_vol.sample_res_gear_rd, dev_vol.res_cali_en);
            if (cali_data > 10 && cali_data <= 100 && dev_cur.sample_res_gear_rd != OHM_100_OHM)
            {
                printf("change gear 100 ohm\r\n");
                bsp_rd_select_r_level(OHM_100_OHM);
                dev_cur.sample_res_gear_rd = OHM_100_OHM;
            }
            else if (cali_data > 100 && cali_data <= 1000 && dev_cur.sample_res_gear_rd != OHM_1_K)
            {
                printf("change gear 1000 ohm\r\n");
                bsp_rd_select_r_level(OHM_1_K);
                dev_cur.sample_res_gear_rd = OHM_1_K;
            }
            else if (cali_data > 1000 && cali_data <= 10000 && dev_cur.sample_res_gear_rd != OHM_10_K)
            {
                printf("change gear 10000 ohm\r\n");
                bsp_rd_select_r_level(OHM_10_K);
                dev_cur.sample_res_gear_rd = OHM_10_K;
            }
            else if (cali_data > 10 * 1000 && cali_data <= 100 * 1000 && dev_cur.sample_res_gear_rd != OHM_100_K)
            {
                printf("change gear 100000 ohm\r\n");
                bsp_rd_select_r_level(OHM_100_K);
                dev_cur.sample_res_gear_rd = OHM_100_K;
            }
            else if (cali_data > 100 * 1000 && cali_data <= 1000 * 1000 && dev_cur.sample_res_gear_rd != OHM_1_M)
            {
                printf("change gear 1000000 ohm\r\n");
                bsp_rd_select_r_level(OHM_1_M);
                dev_cur.sample_res_gear_rd = OHM_1_M;
            }
            else if (cali_data > 1000 * 1000 && cali_data <= 10000 * 1000 && dev_cur.sample_res_gear_rd != OHM_10_M)
            {
                printf("change gear 10000000 ohm\r\n");
                bsp_rd_select_r_level(OHM_10_M);
                dev_cur.sample_res_gear_rd = OHM_10_M;
            }
            else if (cali_data < 0)
            {
                dev_cur.sample_res_gear_rd += 1;
                bsp_rd_select_r_level(dev_cur.sample_res_gear_rd);
            }
            else
            {
                printf("OL\r\n");
            }
            latest_sample_data[i] = cali_data;
        }
        else
        {
            IV_data = latest_sample_raw_data[i] * gain + offset;
            latest_sample_data[i] = IV_data;
        }
    }
}
/**
 * @brief Get the index value from the ring buffer
 * @param index Index position
 * @return The channel index at the specified position in the queue
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
 * @brief Get the data from the ring buffer
 * @param index Index position
 * @return The data at the specified index in the queue
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
