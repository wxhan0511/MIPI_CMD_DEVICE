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
#include <math.h>
#include "bsp_channel_sel.h"
#include "calibration_utils.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/* Legacy conversion coordinate, NOT the physical excitation voltage.
 * Paired with the reference instrument's EEPROM coefficients below.
 * Do not change V without refitting. */
#define RES_EXCITATION_V 0.5
#define RD_SETTLE_MS 120U
#define RD_RESISTANCE_TIME_MS 1000U
#define RD_DIODE_TIME_MS 500U
#define RD_TIMEOUT_MS 2500U
/* Temporary scope/ADC diagnostic: one GET_RESISTANCE holds its pins for 10 s.
 * Set to 0 to restore normal resistance acquisition. Diode is unaffected. */
#define RD_FIXED_10M_DIAGNOSTIC 0
#define RD_DIAGNOSTIC_MS 10000U
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
static uint8_t rd_measure_active = 0U;
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
    /* RDREF EEPROM capture, 2026-10-26; flag=0x8F, read_status=0.
     * Reference UID: 32303836 30345105 003A004C. Keep double precision. */
    {6.274250255595802e-07, 1.2589245023812747, 136358.5973519478}, // 10M
    {2.8042908213941092e-06, 1.6413799835431107, 14809.926608166192}, // 1M
    {2.9220855105576999e-05, 1.5918204657145729, 1975.0893950806058}, // 100K
    {0.00020244407732554568, 1.847439851373391, 29.657266917341076}, // 10K
    {0.0030143603007261807, 1.5615864357168867, 17.923538423852278}, // 1K
    {0.035958432343360935, 1.7784477806803523, -2.7183547154907899}, // 91
    {0, 0, 0}, // Unused diode path; reference NaN coefficients intentionally not imported
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
    if (rd_measure_active == 0U)
        printf("bsp_adc_r_convert: gear %d, input %f, cali_en %d\r\n", gear, input, cali_en);
    double output = 0;
    if (gear < OHM_10_M || gear > OHM_100_OHM || !isfinite(input) ||
        input < 0.0 || input >= RES_EXCITATION_V)
        return NAN;
    switch (gear)
    {
    case OHM_10_M:
        output = input * 10000 * 1000 / (RES_EXCITATION_V - input);
        break;
    case OHM_1_M:
        output = input * 1000 * 1000 / (RES_EXCITATION_V - input);
        break;
    case OHM_100_K:
        output = input * 100 * 1000 / (RES_EXCITATION_V - input);
        break;
    case OHM_10_K:
        output = input * 10 * 1000 / (RES_EXCITATION_V - input);
        break;
    case OHM_1_K:
        output = input * 1000 / (RES_EXCITATION_V - input);
        break;
    case OHM_100_OHM:
        output = input * 91 / (RES_EXCITATION_V - input);
        break;
    case OHM_4_point_7_K:
        output = input / (RES_EXCITATION_V - input);
        break;
    default:;
    }
    if (rd_measure_active == 0U)
        printf("raw resistance: %f ohm\r\n", output);
    if (cali_en == 1)
    {
        output = output * output * cali_r_value[gear - 1][0] + output * cali_r_value[gear - 1][1] + cali_r_value[gear - 1][2];
    }

    // output = output * 0.7346 + 22.908;
    // output  = output * output * 0.00009 + output*0.853 + 5.7802;
    return output;
}
static TEST_R_D_RES_LEVEL rd_target_gear(double resistance)
{
    if (resistance <= 100.0) return OHM_100_OHM;
    if (resistance <= 1000.0) return OHM_1_K;
    if (resistance <= 10000.0) return OHM_10_K;
    if (resistance <= 100000.0) return OHM_100_K;
    if (resistance <= 1000000.0) return OHM_1_M;
    return OHM_10_M;
}

int bsp_rd_measure(uint8_t pin_p, uint8_t pin_n, bool diode, float *result)
{
    if (result == NULL) return -2;
    *result = NAN;
    if (pin_p < 1U || pin_p > 64U || pin_n < 1U || pin_n > 64U || pin_p == pin_n)
        return -2;

    const uint8_t saved_channels = dev_vol.channel_en;
    const bool diagnostic = RD_FIXED_10M_DIAGNOSTIC && !diode;
    const uint32_t duration = diode ? RD_DIODE_TIME_MS : RD_RESISTANCE_TIME_MS;
    int status = -1;
    rd_measure_active = 1U;
    dev_vol.channel_en = 0x04U;
    bsp_close_64pin_channel();
    bsp_ads1256_ch2_select(0);
    bsp_rd_select_r_level(diode ? OHM_4_point_7_K :
                          (diagnostic ? OHM_10_M : OHM_100_OHM));
    bsp_rd_select_mode(diode ? D_MODE : R_MODE);
    bsp_rd_select_pin(pin_p, pin_n, 1);

    const uint32_t started = HAL_GetTick();
    uint32_t changed_at = started;
    uint32_t seq = sample_update_seq[2];
    uint8_t settling = 1U;
    uint32_t range_samples = 0U;
    float range_min = 0.0f;
    float range_max = 0.0f;
    if (diagnostic)
    {
        /* Observe from connection onward, including startup. Do not use the
         * normal settling/ranging/calibration code in this diagnostic. */
        uint32_t count = 0U;
        uint32_t missed = 0U;
        float minimum = 0.0f, maximum = 0.0f;
        printf("RD10M BEGIN pins=%u,%u duration=%lu ms unit=V\r\n",
               pin_p, pin_n, (unsigned long)RD_DIAGNOSTIC_MS);
        printf("RD10M,t_ms,seq,delta,raw_V\r\n");
        while ((HAL_GetTick() - started) < RD_DIAGNOSTIC_MS)
        {
            osDelay(1);
            uint32_t next_seq;
            uint8_t mux;
            float voltage;
            taskENTER_CRITICAL();
            next_seq = sample_update_seq[2];
            mux = latest_sample_ch_sel[2];
            voltage = latest_sample_raw_data[2];
            taskEXIT_CRITICAL();
            if (next_seq == seq) continue;
            const uint32_t delta = next_seq - seq;
            seq = next_seq;
            if (mux != 0U || !isfinite(voltage)) continue;
            missed += delta - 1U;
            if (count == 0U) minimum = maximum = voltage;
            if (voltage < minimum) minimum = voltage;
            if (voltage > maximum) maximum = voltage;
            count++;
            printf("RD10M,%lu,%lu,%lu,%.9f\r\n",
                   (unsigned long)(HAL_GetTick() - started),
                   (unsigned long)seq, (unsigned long)delta, (double)voltage);
        }
        printf("RD10M END samples=%lu missed=%lu min=%.9f max=%.9f span=%.9f V\r\n",
               (unsigned long)count, (unsigned long)missed,
               (double)minimum, (double)maximum, (double)(maximum - minimum));
        /* Diagnostic has no resistance result. Never label volts as ohms. */
        status = -2;
        goto rd_cleanup;
    }
    while ((HAL_GetTick() - started) < RD_TIMEOUT_MS)
    {
        osDelay(1);
        if (settling != 0U)
        {
            if ((HAL_GetTick() - changed_at) < RD_SETTLE_MS) continue;
            /* Discard frames published during settling; require a later frame. */
            seq = sample_update_seq[2];
            settling = 0U;
            continue;
        }

        float voltage;
        uint8_t mux;
        uint32_t next_seq;
        taskENTER_CRITICAL();
        next_seq = sample_update_seq[2];
        voltage = latest_sample_raw_data[2];
        mux = latest_sample_ch_sel[2];
        taskEXIT_CRITICAL();
        if (next_seq == seq) continue;
        const uint32_t seq_delta = next_seq - seq;
        seq = next_seq;
        if (mux != 0U || !isfinite(voltage)) continue;

        const TEST_R_D_RES_LEVEL gear = dev_vol.sample_res_gear_rd;
        if (range_samples == 0U)
            range_min = range_max = voltage;
        if (voltage < range_min) range_min = voltage;
        if (voltage > range_max) range_max = voltage;
        range_samples++;
        /* Limit UART traffic; min/max still include every consumed sample. */
        if (range_samples <= 4U || range_samples % 16U == 0U)
            printf("RD sample: t=%lu seq=%lu delta=%lu gear=%u raw=%.9f\r\n",
                   (unsigned long)(HAL_GetTick() - started),
                   (unsigned long)seq, (unsigned long)seq_delta, gear, (double)voltage);
        const double value = diode ? voltage :
            bsp_adc_r_convert(gear, (double)voltage - 0.001660, dev_vol.res_cali_en);
        if (!isfinite(value) || value < 0.0)
        {
            /* Invalid voltage is not permission to enter the diode range. */
            continue;
        }

        if (!diode && dev_vol.res_cali_en != 0U)
        {
            const TEST_R_D_RES_LEVEL target = rd_target_gear(value);
            if (target != gear)
            {
                bsp_rd_select_r_level(target);
                changed_at = HAL_GetTick();
                settling = 1U;
                range_samples = 0U;
                continue;
            }
        }

        /* Match the reference's timed readout. A non-ohmic DUT need not
         * converge to a constant voltage. Still require a fresh, valid sample
         * after the last range's settling interval; never return stale data. */
        if ((HAL_GetTick() - started) >= duration)
        {
            *result = (float)value;
            latest_sample_data[2] = *result;
            printf("RD result: pins=%u,%u gear=%u raw=%.9f value=%.6f cali=%u elapsed=%lu ms\r\n",
                   pin_p, pin_n, gear, (double)voltage, value,
                   diode ? 0U : dev_vol.res_cali_en,
                   (unsigned long)(HAL_GetTick() - started));
            printf("RD range: samples=%lu min=%.9f max=%.9f span=%.9f V\r\n",
                   (unsigned long)range_samples, (double)range_min,
                   (double)range_max, (double)(range_max - range_min));
            status = 0;
            break;
        }
    }

rd_cleanup:
    bsp_close_64pin_channel();
    bsp_close_rd_select_channel();
    bsp_rd_select_mode(R_D_MODE_NULL);
    calibrated_sample_seq[2] = sample_update_seq[2];
    rd_measure_active = 0U;
    dev_vol.channel_en = saved_channels;
    if (status != 0 && !diagnostic)
        printf("RD acquisition timeout/no valid settled-range sample: pins=%u,%u gear=%u\r\n",
               pin_p, pin_n, dev_vol.sample_res_gear_rd);
    return status;
}

void sample_data_cali()
{
    for (uint8_t i = 0; i < 8; i++)
    {
        const uint32_t target_seq = sample_update_seq[i];
        /* The command owns CH2 conversion and ranging until acquisition ends. */
        if (i == 2U && rd_measure_active != 0U)
        {
            calibrated_sample_seq[i] = target_seq;
            continue;
        }
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
            // 1.660 mV,用途是补偿测阻通路的零点偏移
            // cali_data = bsp_adc_r_convert(dev_vol.sample_res_gear_rd, (raw_data - 1660) / 1000000, dev_vol.res_cali_en);
            cali_data = bsp_adc_r_convert(dev_vol.sample_res_gear_rd, (raw_data - 1660) / 1000000, 0);
            printf("raw data: %f, cali data: %f ohm, gear: %d, cali_en: %d\r\n", raw_sample, cali_data, dev_vol.sample_res_gear_rd, dev_vol.res_cali_en);
            if (cali_data > 10 && cali_data <= 100 && dev_vol.sample_res_gear_rd != OHM_100_OHM)
            {
                printf("change gear 100 ohm\r\n");
                bsp_rd_select_r_level(OHM_100_OHM);
            }
            else if (cali_data > 100 && cali_data <= 1000 && dev_vol.sample_res_gear_rd != OHM_1_K)
            {
                printf("change gear 1000 ohm\r\n");
                bsp_rd_select_r_level(OHM_1_K);
            }
            else if (cali_data > 1000 && cali_data <= 10000 && dev_vol.sample_res_gear_rd != OHM_10_K)
            {
                printf("change gear 10000 ohm\r\n");
                bsp_rd_select_r_level(OHM_10_K);
            }
            else if (cali_data > 10 * 1000 && cali_data <= 100 * 1000 && dev_vol.sample_res_gear_rd != OHM_100_K)
            {
                printf("change gear 100000 ohm\r\n");
                bsp_rd_select_r_level(OHM_100_K);
            }
            else if (cali_data > 100 * 1000 && cali_data <= 1000 * 1000 && dev_vol.sample_res_gear_rd != OHM_1_M)
            {
                printf("change gear 1000000 ohm\r\n");
                bsp_rd_select_r_level(OHM_1_M);
            }
            else if (cali_data > 1000 * 1000 && dev_vol.sample_res_gear_rd != OHM_10_M)
            {
                printf("change gear 10000000 ohm\r\n");
                bsp_rd_select_r_level(OHM_10_M);
            }
            else if (cali_data < 0)
            {
                dev_vol.sample_res_gear_rd += 1;
                bsp_rd_select_r_level(dev_vol.sample_res_gear_rd);
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
