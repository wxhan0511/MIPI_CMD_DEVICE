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
volatile uint32_t sample_update_seq[8] = {0};
volatile uint32_t adc_sample_update_seq = 0;
static uint32_t calibrated_sample_seq[8] = {0};
#if ADC_SAMPLE_TRACE_ENABLE
static float trace_last_raw_data[8] = {0};
static float trace_last_cali_data[8] = {0};
static uint8_t trace_sample_valid[8] = {0};
#endif
extern R_D_MODE r_d_mode;
extern ads1256_dev_t dev_vol;
extern osThreadId_t task_sample_handle;
extern volatile uint8_t g_adc_sample_notify_enabled;

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
/* Wait for one new ADC result. At 50 SPS this normally takes at most 20 ms. */
int wait_adc_one_round(uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    const uint32_t start_seq = adc_sample_update_seq;

    while (adc_sample_update_seq == start_seq)
    {
        if ((HAL_GetTick() - t0) >= timeout_ms)
            return -1;
        bsp_delay_ms(1);
    }

    sample_data_cali();
    return 0;
}
void raw_data_queue_push(float value, uint8_t index)
{
    if (index >= 8U)
        return;

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
    sample_update_seq[index]++;
    adc_sample_update_seq++;

    if (g_adc_sample_notify_enabled != 0U && task_sample_handle != NULL)
        (void)osThreadFlagsSet(task_sample_handle, ADC_SAMPLE_READY_FLAG);
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
        const uint32_t target_seq = sample_update_seq[i];
        if (target_seq == calibrated_sample_seq[i])
            continue;

        const uint32_t update_count = target_seq - calibrated_sample_seq[i];
        const float raw_sample = latest_sample_raw_data[i];
        const uint8_t sample_ch_sel = latest_sample_ch_sel[i];

        sel_cali_param(i, sample_ch_sel, &offset, &gain);

        if (r_d_mode == R_MODE && i == 2 && sample_ch_sel == 0)
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
            const double raw_data = raw_sample * 1000000.0;
            cali_data = bsp_adc_r_convert(dev_vol.sample_res_gear_rd, (raw_data - 1660) / 1000000, dev_vol.res_cali_en);
            printf("raw data: %f, cali data: %f ohm, gear: %d, cali_en: %d\r\n", raw_sample, cali_data, dev_vol.sample_res_gear_rd, dev_vol.res_cali_en);
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
            IV_data = raw_sample * gain + offset;
            latest_sample_data[i] = IV_data;
        }

#if ADC_SAMPLE_TRACE_ENABLE
        {
            const float calibrated_sample = latest_sample_data[i];
            const float raw_delta = raw_sample - trace_last_raw_data[i];
            const float cali_delta = calibrated_sample - trace_last_cali_data[i];
            const float raw_delta_abs = raw_delta < 0.0f ? -raw_delta : raw_delta;
            const float last_raw_abs = trace_last_raw_data[i] < 0.0f ? -trace_last_raw_data[i] : trace_last_raw_data[i];
            const bool is_jump = trace_sample_valid[i] != 0U &&
                                 raw_delta_abs >= ADC_SAMPLE_TRACE_MIN_RAW_DELTA &&
                                 (last_raw_abs < ADC_SAMPLE_TRACE_MIN_RAW_DELTA ||
                                  raw_delta_abs * 100.0f >= last_raw_abs * ADC_SAMPLE_TRACE_JUMP_PERCENT);
            const double adc_code_value = raw_sample / (ADC_RATIO * 0.000001);
            const int32_t adc_code = (int32_t)(adc_code_value + (adc_code_value >= 0.0 ? 0.5 : -0.5));

            printf("[ADC%s] t=%lu seq=%lu(+%lu) ch=%u mux=%u code=%ld raw=%.9f cal=%.6f d_raw=%+.9f d_cal=%+.6f\r\n",
                   is_jump ? " JUMP" : (trace_sample_valid[i] != 0U ? "" : " INIT"),
                   (unsigned long)HAL_GetTick(),
                   (unsigned long)target_seq,
                   (unsigned long)update_count,
                   (unsigned int)i,
                   (unsigned int)sample_ch_sel,
                   (long)adc_code,
                   (double)raw_sample,
                   (double)calibrated_sample,
                   (double)raw_delta,
                   (double)cali_delta);

            trace_last_raw_data[i] = raw_sample;
            trace_last_cali_data[i] = calibrated_sample;
            trace_sample_valid[i] = 1U;
        }
#endif
        calibrated_sample_seq[i] = target_seq;
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
