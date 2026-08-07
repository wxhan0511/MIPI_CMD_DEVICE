/*
 * task_sample.c
 *
 *  Created on: Jun 30, 2025
 *      Author: Wenxiao Han
 */
/* ==================== 1. 头文件包含 ==================== */
#include "task_sample.h"
#include "usart.h"
#include "i2c.h"
#include "spi.h"
#include "power_control.h"
#include "utils.h"
#include "bsp.h"
#include "widget_func.h"
#include "task_com.h"
#include "tim.h"
#include "math.h"

/* ==================== 2. 宏定义 ==================== */
// 一轮完整的采样流程：
#define WAIT_ADC_1_IDLE           \
    while (dev_vol.step_cnt != 6) \
    {                             \
        osDelay(10);              \
    }

/* ==================== 3. 类型定义（结构体、枚举、别名） ==================== */

typedef int (*power_status_func_t)(void);
typedef int (*lim_status_func_t)(void);

/* ==================== 4. 外部全局变量 ==================== */
extern volatile uint8_t meter_com_flag;
extern uint8_t meter_rx_buf[SPI2_SLAVE_RX_LEN];
extern uint8_t meter_tx_buf[SPI2_SLAVE_TX_LEN];
extern volatile TEST_R_D_RES_LEVEL r_level_selected;
extern ads1256_dev_t dev_vol;
extern __IO uint32_t uwFrequency;
extern uint8_t get_freq_flag;
extern __IO uint32_t uwDutyCycle;
extern lcd_show_t lcd_show;

osThreadId_t task_sample_handle;
const osThreadAttr_t task_sample_attributes = {
    .name = "task_sample_task",
    .stack_size = 4096,
    .priority = (osPriority_t)osPriorityHigh,
};

osMutexId_t sample_mutex;
osStaticMutexDef_t sample_mutex_control_block;
const osMutexAttr_t sample_mutex_attributes = {
    .name = "show_mutex",
    .cb_mem = &sample_mutex_control_block,
    .cb_size = sizeof(sample_mutex_control_block),
};
extern void task_com_resume(void);
extern void disableTim1PWMOutput(void);
extern void disableTim2PWMOutput(void);
extern void enableTim1PWMOutput(void);
extern int Measure_Frequency_Adaptive(void);
sample_data_t sample_data;
SampleTask_S g_sample_task = {0};
uint8_t sample_vol_id = 0;
uint8_t sample_cur_id = 0;
uint8_t ads1256_ch_index = 0;
uint8_t d_trigger_ch_index = 0;

/* ==================== 6. 静态函数声明 ==================== */
static int find_sample_vol_map_index(uint8_t chip_index, uint8_t d_trigger_ch_index);
static int find_sample_cur_map_index(uint8_t chip_index, uint8_t d_trigger_ch_index);
static inline int get_VSN_status(void) { return !bsp_d_trigger_get_channel_state(&d_3, 0); }
static inline int get_ELVSS_status(void) { return !bsp_d_trigger_get_channel_state(&d_3, 1); }
static inline int get_ELVDD_status(void) { return bsp_d_trigger_get_channel_state(&d_3, 2); }
static inline int get_VDD_status(void) { return bsp_d_trigger_get_channel_state(&d_3, 3); }
static inline int get_AVDD_status(void) { return bsp_d_trigger_get_channel_state(&d_3, 4); }
static inline int get_VSP_status(void) { return bsp_d_trigger_get_channel_state(&d_3, 5); }
static inline int get_IOVCC_status(void) { return bsp_d_trigger_get_channel_state(&d_3, 6); }
static inline int get_VCC_status(void) { return bsp_d_trigger_get_channel_state(&d_3, 7); }

static inline int get_VCC_lim_status(void) { return bsp_d_trigger_get_channel_state(&d_2, VCC_RLY); }
static inline int get_IOVCC_lim_status(void) { return bsp_d_trigger_get_channel_state(&d_2, IOVCC_RLY); }
static inline int get_VSP_lim_status(void) { return bsp_d_trigger_get_channel_state(&d_2, VSP_RLY); }
static inline int get_VSN_lim_status(void) { return bsp_d_trigger_get_channel_state(&d_2, VSN_RLY); }
static inline int get_AVDD_lim_status(void) { return bsp_d_trigger_get_channel_state(&d_2, AVDD_RLY); }
static inline int get_VDD_lim_status(void) { return bsp_d_trigger_get_channel_state(&d_2, VDD_RLY); }
static inline int get_ELVDD_lim_status(void) { return bsp_d_trigger_get_channel_state(&d_2, ELVDD_RLY); }
static inline int get_ELVSS_lim_status(void) { return bsp_d_trigger_get_channel_state(&d_2, ELVSS_RLY); }

power_status_func_t power_enable_status[8] = {
    get_VCC_status,
    get_IOVCC_status,
    get_VSP_status,
    get_VSN_status,
    get_AVDD_status,
    get_VDD_status,
    get_ELVDD_status,
    get_ELVSS_status};
lim_status_func_t lim_gear_status[8] = {
    get_VCC_lim_status,
    get_IOVCC_lim_status,
    get_VSP_lim_status,
    get_VSN_lim_status,
    get_AVDD_lim_status,
    get_VDD_lim_status,
    get_ELVDD_lim_status,
    get_ELVSS_lim_status};

// 0xff:不需要dtrigger次级选通
uint8_t sample_cur_map[11][2] = {
    {3, 0xff}, // power:0,VCC → ch_index、d_trigger_ch_index
    {4, 0xff}, // power:1,IOVCC → ch_index、d_trigger_ch_index
    {5, 0xff}, // power:2,VSP → ch_index、d_trigger_ch_index
    {6, 0xff}, // power:3,VSN → ch_index、d_trigger_ch_index
    {1, 7},    // power:4,AVDD → ch_index、d_trigger_ch_index
    {7, 0xff}, // power:5,VDD → ch_index、d_trigger_ch_index
    {1, 2},    // power:6,ELVDD → ch_index、d_trigger_ch_index
    {1, 3},    // power:7,ELVSS → ch_index、d_trigger_ch_index
    {1, 4},    // power:8,AD_I_BLAS_I → ch_index、d_trigger_ch_index
    {1, 5},    // power:9,AD_I_BLAS_V → ch_index、d_trigger_ch_index
    {1, 6},    // power:10,AD_I_BL → ch_index、d_trigger_ch_index
};
uint8_t sample_vol_map[15][2] = {
    {0, 0}, // power:0,VCC → ch_index、d_trigger_ch_index
    {0, 1}, // power:1,IOVCC → ch_index、d_trigger_ch_index
    {0, 7}, // power:2,VSP → ch_index、d_trigger_ch_index
    {0, 3}, // power:3,VSN → ch_index、d_trigger_ch_index
    {0, 6}, // power:4,AVDD → ch_index、d_trigger_ch_index
    {0, 4}, // power:5,VDD → ch_index、d_trigger_ch_index
    {0, 5}, // power:6,ELVDD → ch_index、d_trigger_ch_index
    {0, 2}, // power:7,ELVSS → ch_index、d_trigger_ch_index
    {1, 0}, // power:8,AD_V_V+ADJ → ch_index、d_trigger_ch_index
    {2, 0}, // power:9,AD_R&D → ch_index、d_trigger_ch_index
    {2, 2}, // power:10,AD_24PinV → ch_index、d_trigger_ch_index
    {2, 4}, // power:11,AD_V_BLAS_I → ch_index、d_trigger_ch_index
    {2, 5}, // power:12,BLAS_V → ch_index、d_trigger_ch_index
    {2, 6}, // power:13,AD_V_BL → ch_index、d_trigger_ch_index
    {2, 7}, // power:14,AD_V_V-ADJ → ch_index、d_trigger_ch_index
};
static int find_sample_vol_map_index(uint8_t chip_index, uint8_t d_trigger_ch_index)
{
    for (int j = 0; j < 15; j++)
    {
        if (sample_vol_map[j][0] == chip_index && sample_vol_map[j][1] == d_trigger_ch_index)
        {
            return j; // 找到匹配的索引
        }
    }
    return -1; // 未找到
}
static int find_sample_cur_map_index(uint8_t chip_index, uint8_t d_trigger_ch_index)
{
    for (int j = 0; j < 11; j++)
    {
        if (sample_cur_map[j][0] == chip_index && sample_cur_map[j][1] == d_trigger_ch_index)
        {
            return j; // 找到匹配的索引
        }
    }
    return -1; // 未找到
}

/* ==================== 7. 外部可调用函数实现 ==================== */
void task_sample_run(void *argument)
{
    (void)argument;

    uint32_t t0 = 0;
    uint32_t t_temp_start = 0;
    uint32_t t_temp_end = 0;
    uint8_t power_id = 0;
    uint8_t en = 0;
    float vol = 0;
    float cur = 0;
    float ref_freq_vol = 3300;
    float_bytes_t float_bytes = {0};
    uint8_t pin_n = 0;
    uint8_t pin_p = 0;
    uint8_t pin_num = 0;
    R_D_MODE rd_mode = R_D_MODE_NULL;
    TEST_R_D_RES_LEVEL r_level = OHM_10_M;
    uint8_t usr_idx = 0;
    // 用户下发的设置电压index 到 dac通道index的映射 5个dac共20路
    static const uint8_t power_order[20] = {0, 1, 2, 3, 8, 9, 10, 11, 4, 5, 6, 7, 12, 13, 14, 15, 16, 17, 18, 19};

    for (;;)
    {
        switch (g_sample_task.cmd_type)
        {
        case NORMAL_LOOP_EVENT:
            task_sample_task_mutex_acquire(); // 通信时无法获取互斥锁,空闲时更新采样数据到显示屏上
            for (uint8_t i = 0; i < 8; i++)
            {
                // M_SPI_DEBUG("chip_index: %d, d_trigger_ch_index: %d\n", i, latest_sample_ch_sel[sample_vol_map[i][0]]);
                if (i <= 2 && latest_sample_ch_sel[i] == 0xff) //
                {
                    continue;
                }
                if (i > 2 && latest_sample_ch_sel[i] != 0xff) //
                {
                    continue;
                }
                int idx_vol = find_sample_vol_map_index(i, latest_sample_ch_sel[i]);
                int idx_cur = find_sample_cur_map_index(i, latest_sample_ch_sel[i]);
                // M_SPI_DEBUG("latest_sample_ch_sel[%d]: %d, idx_vol: %d, idx_cur: %d\n", i, latest_sample_ch_sel[i], idx_vol, idx_cur);

                if (idx_vol != -1)
                {
                    // for (uint8_t k = 0; k < 8; k++)
                    // {
                    //     if (i == 0 && k != idx_vol)
                    //         lcd_show.voltage[k] = NAN; // 其他通道显示nan,未选中
                    // }
                    // for (uint8_t k = 9; k < 15; k++)
                    // {
                    //     if (i == 2 && k != idx_vol)
                    //         lcd_show.voltage[k] = NAN; // 其他通道显示nan,未选中
                    // }
                    bool enable = (idx_vol < 8) ? power_enable_status[idx_vol]() : 1;
                    if (enable)
                    {
                        lcd_show.voltage[idx_vol] = latest_sample_data[i];
                        // M_SPI_DEBUG("idx_vol: %d, voltage: %f\n", idx_vol, latest_sample_data[i]);
                    }
                    else
                        lcd_show.voltage[idx_vol] = 0; // 未使能
                }
                if (idx_cur != -1)
                {
                    // for (uint8_t k = 4; k < 11; k++)
                    // {
                    //     if (i == 1 && k != idx_cur && k != 1)
                    //         lcd_show.current[k] = NAN; // 其他通道显示nan,未选中
                    // }
                    bool enable = (idx_cur < 8) ? power_enable_status[idx_cur]() : 1;
                    if (enable)
                    {
                        lcd_show.current[idx_cur] = latest_sample_data[i];
                    }
                    else
                        lcd_show.current[idx_cur] = 0; // 未使能
                }
                if (idx_vol == -1 && idx_cur == -1)
                {
                    // M_SPI_DEBUG("no idx found for chip_index: %d, d_trigger_ch_index: %d\n", i, latest_sample_ch_sel[sample_vol_map[i][0]]);
                    continue;
                }
            }
            task_sample_task_mutex_release();
            osDelay(5);
            break;
        case GET_ID:
            meter_rx_buf[2] = id; // 1,2,3,4:bit1~4
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case enable_lim:
            uint8_t lim_status = meter_rx_buf[2];
            bsp_lim_rst_set(lim_status);
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case GET_SW_VERSION:
            memcpy(&meter_tx_buf[3], sw_version, 4);
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case VOL_SET:
        case LIM_SET:
            // 解析数据
            g_sample_task.set_power_data_frame.frame_header = meter_rx_buf[0];
            g_sample_task.set_power_data_frame.cmd_type = meter_rx_buf[1];
            g_sample_task.set_power_data_frame.power_id = meter_rx_buf[2];
            // 解析浮点数
            memcpy(&float_bytes, &meter_rx_buf[3], sizeof(float_bytes));
            memcpy(g_sample_task.set_power_data_frame.value.bytes, float_bytes.b, sizeof(float_bytes.b));
            // 用户下发的index 到 dac通道index的映射
            g_sample_task.set_power_data_frame.power_id = power_order[g_sample_task.set_power_data_frame.power_id];
            // 用户下发的 index 到 dac通道index的映射
            uint8_t lim_idx = 0;
            if (3 < g_sample_task.set_power_data_frame.power_id && g_sample_task.set_power_data_frame.power_id < 8)
            {
                lim_idx = g_sample_task.set_power_data_frame.power_id - 4;
                lcd_show.threshold[lim_idx] = float_bytes.f;
            }
            if (11 < g_sample_task.set_power_data_frame.power_id && g_sample_task.set_power_data_frame.power_id < 16)
            {
                lim_idx = g_sample_task.set_power_data_frame.power_id - 8;
                lcd_show.threshold[lim_idx] = float_bytes.f;
            }

            M_SPI_DEBUG("set_power_data_frame.power_id:%x\r\n", g_sample_task.set_power_data_frame.power_id);
            M_SPI_DEBUG("set_power_data_frame.value.bytes:%02X %02X %02X %02X\r\n",
                        float_bytes.b[0], float_bytes.b[1], float_bytes.b[2], float_bytes.b[3]);
            M_SPI_DEBUG("set_power_data_frame.value.float_value:%f\r\n", float_bytes.f);

            *(dac_config_table[g_sample_task.set_power_data_frame.power_id].last_voltage) = float_bytes.f;

            bsp_cali_and_set_power(g_sample_task.set_power_data_frame.power_id);
            calibration_save();
            meter_tx_buf[2] = g_sample_task.cmd_status;
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case SET_ALL_POWER_VOLTAGE:
            g_sample_task.set_power_data_frame.frame_header = meter_rx_buf[0];
            g_sample_task.set_power_data_frame.cmd_type = meter_rx_buf[1];
            for (uint8_t i = 0; i < 8; i++)
            {
                g_sample_task.set_power_data_frame.power_id = i;
                memcpy(&float_bytes, &meter_rx_buf[2 + i * sizeof(float_bytes)], sizeof(float_bytes));
                memcpy(g_sample_task.set_power_data_frame.value.bytes, float_bytes.b, sizeof(float_bytes.b));
                g_sample_task.set_power_data_frame.power_id = power_order[g_sample_task.set_power_data_frame.power_id];
                // M_SPI_DEBUG("set_power_data_frame.power_id:%x\r\n", g_sample_task.set_power_data_frame.power_id);
                // M_SPI_DEBUG("set_power_data_frame.value.bytes:%02X %02X %02X %02X\r\n",
                //        float_bytes.b[0], float_bytes.b[1], float_bytes.b[2], float_bytes.b[3]);
                // M_SPI_DEBUG("set_power_data_frame.value.float_value:%f\r\n", float_bytes.f);

                *(dac_config_table[g_sample_task.set_power_data_frame.power_id].last_voltage) = float_bytes.f;

                bsp_cali_and_set_power(g_sample_task.set_power_data_frame.power_id);
            }
            calibration_save();
            meter_tx_buf[2] = g_sample_task.cmd_status;
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case ALL_POWER_EN:
            memcpy(g_sample_task.power_switch, &meter_rx_buf[2], 8);
            for (uint8_t i = 0; i < 8; i++)
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
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case SINGLE_POWER_EN:
            power_id = power_order[meter_rx_buf[2]];
            en = meter_rx_buf[3];
            if (en == 0x01)
                bsp_power_single_enable(power_id);
            else
                bsp_power_single_disable(power_id);
            M_SPI_DEBUG("power_id: %d, en: %d\r\n", power_id, en);
            M_SPI_DEBUG("power_on: %d\r\n", (uint8_t)power_enable_status[power_id]());

            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case SINGLE_VOL_GET:
            usr_idx = meter_rx_buf[2];
            M_SPI_DEBUG("usr_idx: %d\r\n", usr_idx);

            if (usr_idx < 8U)
            {
                if (!power_enable_status[usr_idx]())
                {
                    M_SPI_DEBUG("usr_idx:%d not enable,break\r\n", usr_idx);
                    memset(&meter_tx_buf[3], 0, sizeof(float));
                    task_com_resume();
                    g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
                    break;
                }
            }
            meter_wait_v_c_ready(usr_idx, 0);
            // 测完24pin和40pin关闭24pin和40pin的通道,避免干扰
            if (usr_idx == 10)
                bsp_close_24pin_channel();
            if (usr_idx == 9)
                bsp_close_40pin_channel();

            M_SPI_DEBUG("ads1256_ch_index: %d, d_trigger_ch_index: %d, latest_sample_ch_sel: %d\r\n", ads1256_ch_index, d_trigger_ch_index, latest_sample_ch_sel[ads1256_ch_index]);
            memcpy(&meter_tx_buf[3], (const void *)&latest_sample_data[ads1256_ch_index], sizeof(float));
            M_SPI_DEBUG("SINGLE_VOL_GET: channel %d, voltage %f\r\n", ads1256_ch_index, latest_sample_data[ads1256_ch_index]);
            M_SPI_DEBUG("latest_sample_raw_data: %f\r\n", latest_sample_raw_data[ads1256_ch_index]);
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case SINGLE_CUR_GET:
            usr_idx = meter_rx_buf[2];
            meter_wait_v_c_ready(usr_idx, 1);

            M_SPI_INFO("ads1256_ch_index: %d, d_trigger_ch_index: %d, latest_sample_ch_sel: %d\r\n", ads1256_ch_index, d_trigger_ch_index, latest_sample_ch_sel[ads1256_ch_index]);

            memcpy(&meter_tx_buf[3], (const void *)&latest_sample_data[ads1256_ch_index], sizeof(float));

            M_SPI_DEBUG("SINGLE_CUR_GET: channel %d, current %f\r\n", ads1256_ch_index, latest_sample_data[ads1256_ch_index]);
            M_SPI_DEBUG("latest_sample_raw_data: %f\r\n", latest_sample_raw_data[ads1256_ch_index]);

            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case GET_FREQUENCY:
            // 打开TE通道
            bsp_sel_test_freq_ch(1);
            memcpy(&ref_freq_vol, &meter_rx_buf[2], sizeof(float));
            M_SPI_DEBUG("%f\r\n", ref_freq_vol);

            g_calibration_manager.data.ref_freq_last = ref_freq_vol / 2;
            bsp_cali_and_set_power(17);
            get_freq_flag = 0;
            // 停止可能冲突的输出
            disableTim1PWMOutput();
            disableTim2PWMOutput();

            // 调用自适应测量函数
            if (Measure_Frequency_Adaptive() != 0)
            {
                g_sample_task.cmd_status = POWER_CMD_STATUS_TIMEOUT;
                M_SPI_INFO("GET_FREQ timeout\r\n");
            }

            // 关闭TE通道
            bsp_sel_test_freq_ch(0);
            bsp_led_pwm_init(10);
            enableTim1PWMOutput();
            M_SPI_DEBUG("Result: Freq: %lu Hz\n", uwFrequency);
            float freq_f = (float)uwFrequency;
            M_SPI_DEBUG("freq_f: %f\r\n", freq_f);
            memcpy(&meter_tx_buf[3], &freq_f, sizeof(float));
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        // 档位,时间,算法需与4.0采样板保持一致
        case GET_RESISTANCE:
            pin_p = meter_rx_buf[2];
            pin_n = meter_rx_buf[3];
            dev_vol.sample_res_gear_rd = OHM_100_OHM;
            dev_vol.channel_en = 0b00000100; // 只使能通道2
            bsp_ads1256_ch2_select(0);       // 先选择通道2的子通道0
            bsp_rd_select_r_level(dev_vol.sample_res_gear_rd);
            bsp_rd_select_mode(R_MODE);
            bsp_rd_select_pin(pin_p, pin_n, 1);
            t_temp_start = HAL_GetTick();
            M_SPI_DEBUG("t_temp_start: %lu\r\n", t_temp_start);

            M_SPI_DEBUG("GET_RESISTANCE: pin_p %d, pin_n %d, r_level %d\r\n", pin_p, pin_n, dev_vol.sample_res_gear_rd);
            wait_adc_one_round(1000);
            t0 = HAL_GetTick();
            while (latest_sample_ch_sel[2] != 0) // 等待ADS1256通道2的子通道切到0,且最新采样数据已准备好
            {
                if ((HAL_GetTick() - t0) >= 5000U)
                {
                    // 可按你的状态定义改成超时状态
                    g_sample_task.cmd_status = POWER_CMD_STATUS_TIMEOUT;
                    M_SPI_INFO("GET_RESISTANCE timeout\r\n");
                    break;
                }
                osDelay(1);
            }
            wait_adc_one_round(100); // 至少等6轮,从档六开始切
            wait_adc_one_round(100);
            wait_adc_one_round(100);
            wait_adc_one_round(100);
            wait_adc_one_round(100);
            wait_adc_one_round(100);
            wait_adc_one_round(100);
            // bsp_delay_ms(100);
            memcpy(&meter_tx_buf[3], (const void *)&latest_sample_data[2], sizeof(float));
            t_temp_end = HAL_GetTick();
            M_SPI_DEBUG("t_temp_end: %lu\r\n", t_temp_end);
            M_SPI_DEBUG("resistance measurement time: %lu ms\r\n", t_temp_end - t_temp_start);
            M_SPI_DEBUG("GET RESISTANCE: %f\r\n", latest_sample_data[2]);
            M_SPI_DEBUG("latest_sample_raw_data: %f\r\n", latest_sample_raw_data[2]);
            M_SPI_DEBUG("pin_p: %d, pin_n: %d, resistance: %f m\r\n", pin_p, pin_n, latest_sample_data[2] / 1000000);
            M_SPI_DEBUG("final sample_res_gear_rd: %d\r\n", dev_vol.sample_res_gear_rd);
            bsp_rd_select_mode(R_D_MODE_NULL);
            bsp_rd_select_pin(pin_p, pin_n, 0);
            dev_vol.channel_en = 0b11111111; // 使能所有通道
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case GET_DIODE:
            pin_p = meter_rx_buf[2];
            pin_n = meter_rx_buf[3];
            dev_vol.sample_res_gear_rd = OHM_4_point_7_K;
            bsp_rd_select_r_level(dev_vol.sample_res_gear_rd);
            dev_vol.channel_en = 0b00000100; // 只使能通道2
            bsp_rd_select_mode(D_MODE);
            bsp_ads1256_ch2_select(0);
            bsp_rd_select_pin(pin_p, pin_n, 1);

            M_SPI_DEBUG("GET_DIODE: pin_p %d, pin_n %d, r_level %d\r\n", pin_p, pin_n, dev_vol.sample_res_gear_rd);

            M_SPI_DEBUG("Waiting for ADS1256 channel 2 sub channel 0 data ready...\r\n");
            wait_adc_one_round(1000);
            t0 = HAL_GetTick();
            while (latest_sample_ch_sel[2] != 0) // 等待ADS1256通道2的数据准备好
            {
                if ((HAL_GetTick() - t0) >= 1000U) // 最多等待1s
                {
                    // 可按你的状态定义改成超时状态
                    g_sample_task.cmd_status = POWER_CMD_STATUS_TIMEOUT;
                    M_SPI_INFO("GET_DIODE timeout\r\n");
                    break;
                }
                osDelay(1);
            }
            wait_adc_one_round(1000);
            memcpy(&meter_tx_buf[3], (const void *)&latest_sample_raw_data[2], sizeof(float));

            M_SPI_DEBUG("pin_p: %d, pin_n: %d, vol: %f\r\n", pin_p, pin_n, latest_sample_raw_data[2]);
            bsp_rd_select_pin(pin_p, pin_n, 0);
            dev_vol.channel_en = 0b11111111; // 使能所有通道
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case SET_FREQUENCY:

            break;
        case GET_24PIN_VOLTAGE:
            M_SPI_DEBUG("GET_24PIN_VOLTAGE\r\n");
            pin_num = meter_rx_buf[2];
            pin_num = pin_num - 1; // 外部传入的pin_num从1开始,这里转换成从0开始
            bsp_select_24pin_channel(pin_num, 1);

            ads1256_ch_index = 2;
            d_trigger_ch_index = 2;
            latest_sample_ch_sel[ads1256_ch_index] = 8; // 先将latest_sample_ch_sel置为8，表示数据未准备好
            if (ads1256_ch_index == 0 && d_trigger_ch_index != 0xff)
                bsp_ads1256_ch0_select(d_trigger_ch_index);
            else if (ads1256_ch_index == 1 && d_trigger_ch_index != 0xff)
                bsp_ads1256_ch1_select(d_trigger_ch_index);
            else if (ads1256_ch_index == 2 && d_trigger_ch_index != 0xff)
                bsp_ads1256_ch2_select(d_trigger_ch_index);
            if (ads1256_ch_index < 3 && d_trigger_ch_index != 0xff)
            {
                t0 = HAL_GetTick();
                while (latest_sample_ch_sel[ads1256_ch_index] != d_trigger_ch_index)
                {
                    if ((HAL_GetTick() - t0) >= 2000U) // 最多等待2s
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
            memcpy(&meter_tx_buf[3], (const void *)&latest_sample_data[ads1256_ch_index], sizeof(float));
            M_SPI_DEBUG("latest_sample_raw_data: %f\r\n", latest_sample_raw_data[ads1256_ch_index]);
            M_SPI_DEBUG("SINGLE_VOL_GET: channel %d, voltage %f\r\n", ads1256_ch_index, latest_sample_data[ads1256_ch_index]);

            bsp_select_24pin_channel(pin_num, 0);
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case SEL_PIN_24:
            pin_num = meter_rx_buf[2];
            bsp_select_24pin_channel(pin_num, 1);
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case SEL_PIN_PN:
            pin_p = meter_rx_buf[2];
            pin_n = meter_rx_buf[3];
            bsp_rd_select_pin(pin_p, pin_n, 1);
            M_SPI_DEBUG("SEL_PIN_PN: pin_p %d, pin_n %d\r\n", pin_p, pin_n);
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case SEL_LIM_GEAR:
            M_SPI_DEBUG("SEL_LIM_GEAR\r\n");
            uint8_t idx_lim_gear = meter_rx_buf[2];
            uint8_t lim_gear = meter_rx_buf[3];
            // 全ua档 RST拉低,有一个是ma档,RST拉高
            uint8_t lim_gear_ua_count = 0;
            for (uint8_t i = 0; i < 8; i++)
                if (i != idx_lim_gear) // 其他档位
                    if (!lim_gear_status[i]())
                        lim_gear_ua_count++; // 统计其他档位中限流在ua档的数量
            if (lim_gear_ua_count == 7 && lim_gear == 0)
                bsp_lim_rst_set(0); // 8个档位都是ua档

            if (lim_gear == 1)
                bsp_lim_rst_set(1);

            if (idx_lim_gear == 0)
            {
                bsp_rly_gear_set(lim_gear, VCC_RLY);
                g_calibration_manager.data.da_data.vcc_ref_gain = 0.1;
                g_calibration_manager.data.da_data.vcc_ref_offset = 0;
            }
            else if (idx_lim_gear == 1)
            {
                bsp_rly_gear_set(lim_gear, IOVCC_RLY);
                g_calibration_manager.data.da_data.iovcc_ref_gain = 0.1;
                g_calibration_manager.data.da_data.iovcc_ref_offset = 0;
            }
            else if (idx_lim_gear == 2)
            {
                bsp_rly_gear_set(lim_gear, VSP_RLY);
                g_calibration_manager.data.da_data.vsp_ref_gain = 0.1;
                g_calibration_manager.data.da_data.vsp_ref_offset = 0;
            }
            else if (idx_lim_gear == 3)
            {
                bsp_rly_gear_set(lim_gear, VSN_RLY);
                g_calibration_manager.data.da_data.vsn_ref_gain = 0.1;
                g_calibration_manager.data.da_data.vsn_ref_offset = 0;
            }
            else if (idx_lim_gear == 4)
            {
                bsp_rly_gear_set(lim_gear, AVDD_RLY);
                g_calibration_manager.data.da_data.avdd_ref_gain = 0.1;
                g_calibration_manager.data.da_data.avdd_ref_offset = 0;
            }
            else if (idx_lim_gear == 5)
            {
                bsp_rly_gear_set(lim_gear, VDD_RLY);
                g_calibration_manager.data.da_data.vdd_ref_gain = 0.1;
                g_calibration_manager.data.da_data.vdd_ref_offset = 0;
            }
            else if (idx_lim_gear == 6)
            {
                bsp_rly_gear_set(lim_gear, ELVDD_RLY);
                g_calibration_manager.data.da_data.elvdd_ref_gain = 0.1;
                g_calibration_manager.data.da_data.elvdd_ref_offset = 0;
            }
            else if (idx_lim_gear == 7)
            {
                bsp_rly_gear_set(lim_gear, ELVSS_RLY);
                g_calibration_manager.data.da_data.elvss_ref_gain = 0.1;
                g_calibration_manager.data.da_data.elvss_ref_offset = 0;
            }
            M_SPI_DEBUG("idx_lim_gear: %d, lim_gear: %d\r\n", idx_lim_gear, lim_gear);
            M_SPI_DEBUG("lim_gear_ua_count: %d\r\n", lim_gear_ua_count);
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case SET_BACKLIGHT_CURRENT:
            memcpy(&float_bytes, &meter_rx_buf[2], sizeof(float_bytes));
            disableTim1CaptureCompareInterrupt();
            bsp_led_pwm_init((uint8_t)float_bytes.f);
            enableTim1PWMOutput();
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case READ_DA_DATA:
            usr_idx = meter_rx_buf[2];
            uint8_t dac_channel_index = power_order[usr_idx];
            dac_config_table_t *cfg = &dac_config_table[dac_channel_index];
            float da_data = (float)dac_chips[cfg->chip].val[cfg->channel];
            memcpy(&meter_tx_buf[3], &da_data, sizeof(float));
            M_SPI_DEBUG("READ_DA_DATA : %f\r\n", da_data);
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case READ_AD_DATA:
            usr_idx = meter_rx_buf[2];
            uint8_t data_type = meter_rx_buf[3];
            uint8_t gear = meter_rx_buf[4];
            M_SPI_DEBUG("usr_idx: %d\r\n", usr_idx);
            if (usr_idx < 8U)
            {
                if (!power_enable_status[usr_idx]())
                {
                    M_SPI_DEBUG("usr_idx:%d not enable,break\r\n", usr_idx);
                    memset(&meter_tx_buf[3], 0, sizeof(float));
                    task_com_resume();
                    g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
                    break;
                }
            }
            meter_wait_v_c_ready(usr_idx, data_type);
            // 测完24pin和40pin关闭24pin和40pin的通道,避免干扰
            if (data_type == 0 && usr_idx == 10)
                bsp_close_24pin_channel();
            if (data_type == 0 && usr_idx == 9)
                bsp_close_40pin_channel();

            M_SPI_DEBUG("ads1256_ch_index: %d, d_trigger_ch_index: %d, latest_sample_ch_sel: %d\r\n", ads1256_ch_index, d_trigger_ch_index, latest_sample_ch_sel[ads1256_ch_index]);
            memcpy(&meter_tx_buf[3], (const void *)&latest_sample_raw_data[ads1256_ch_index], sizeof(float));
            M_SPI_DEBUG("latest_sample_raw_data: %f\r\n", latest_sample_raw_data[ads1256_ch_index]);
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case WRITE_CALI_DATA:
        {
            printf("WRITE_CALI_DATA\r\n");
            calibration_data_t *cal = &g_calibration_manager.data;

            // 80 个 float 的临时拼接缓存
            static float temp_cali_data[80] = {0};
            float da_cali_data[16] = {0};
            float ad_v_cali_data[16] = {0};
            float ad_c_ma_cali_data[16] = {0};
            float ad_c_ua_cali_data[16] = {0};
            float ad_v_i_cali_data[8] = {0};
            float ad_c_i_cali_data[8] = {0};

            // 1. 读取当前是第几次发送 (0~7)
            uint8_t num = meter_rx_buf[2];
            uint8_t *ptr = &meter_rx_buf[3];

            // 防止 num 越界 (合法值为 0~7)
            if (num > 7)
            {
                printf("Invalid num: %d\r\n", num);
                task_com_resume();
                g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
                break;
            }

            // 2. 将当前包的 10 个 float (40字节) 拷贝到临时数组的对应位置
            memcpy(&temp_cali_data[num * 10], ptr, 10 * sizeof(float));

            // 3. 只有当接收到最后一包 (num == 7) 时，才进行数据分发
            if (num == 7)
            {
                uint8_t *p_src = (uint8_t *)temp_cali_data;

                // 拷贝前 16 个 float (索引 0~15) 给 da_cali_data
                memcpy(da_cali_data, p_src, sizeof(da_cali_data));
                p_src += sizeof(da_cali_data);

                // 拷贝 16 个 float (索引 16~31) 给 ad_v_cali_data
                memcpy(ad_v_cali_data, p_src, sizeof(ad_v_cali_data));
                p_src += sizeof(ad_v_cali_data);

                // 拷贝 16 个 float (索引 32~47) 给 ad_c_ma_cali_data
                memcpy(ad_c_ma_cali_data, p_src, sizeof(ad_c_ma_cali_data));
                p_src += sizeof(ad_c_ma_cali_data);

                // 拷贝 16 个 float (索引 48~63) 给 ad_c_ua_cali_data
                memcpy(ad_c_ua_cali_data, p_src, sizeof(ad_c_ua_cali_data));
                p_src += sizeof(ad_c_ua_cali_data);

                // 拷贝 8 个 float (索引 64~71) 给 ad_v_i_cali_data
                memcpy(ad_v_i_cali_data, p_src, sizeof(ad_v_i_cali_data));
                p_src += sizeof(ad_v_i_cali_data);

                // 拷贝 8 个 float (索引 72~79) 给 ad_c_i_cali_data
                memcpy(ad_c_i_cali_data, p_src, sizeof(ad_c_i_cali_data));
                p_src += sizeof(ad_c_i_cali_data);

                printf("All 80 calibration data received!\r\n");
                // TODO: 在这里执行实际的校准应用或写入 Flash 操作
                // ================= 开始写入校准值 =================

                // 5. 写入 DA 校准值
                // 顺序: vcc_set_gain, vcc_set_offset, iovcc, vsp, vsn, avdd, vdd, elvdd, elvss
                cal->da_data.vcc_set_gain = da_cali_data[0];
                cal->da_data.vcc_set_offset = da_cali_data[1];

                cal->da_data.iovcc_set_gain = da_cali_data[2];
                cal->da_data.iovcc_set_offset = da_cali_data[3];

                cal->da_data.vsp_set_gain = da_cali_data[4];
                cal->da_data.vsp_set_offset = da_cali_data[5];

                cal->da_data.vsn_set_gain = da_cali_data[6];
                cal->da_data.vsn_set_offset = da_cali_data[7];

                cal->da_data.avdd_set_gain = da_cali_data[8];
                cal->da_data.avdd_set_offset = da_cali_data[9];

                cal->da_data.vdd_set_gain = da_cali_data[10];
                cal->da_data.vdd_set_offset = da_cali_data[11];

                cal->da_data.elvdd_set_gain = da_cali_data[12];
                cal->da_data.elvdd_set_offset = da_cali_data[13];

                cal->da_data.elvss_set_gain = da_cali_data[14];
                cal->da_data.elvss_set_offset = da_cali_data[15];
                // 6. 写入 AD 电压校准值
                uint8_t v_idx_map[8] = {0, 1, 7, 3, 5, 6, 4, 2}; // 物理通道 0~7 对应的上位机数据索引
                for (uint8_t i = 0; i < 8; i++)
                {

                    uint8_t idx = v_idx_map[i];
                    cal->ad_data.ch0_gain[i] = ad_v_cali_data[idx * 2];               // 偶数索引为 gain
                    cal->ad_data.ch0_offset[i] = ad_v_cali_data[idx * 2 + 1] * 0.001; // 奇数索引为 offset
                }

                // 7. 写入 AD 电流校准值 (ADCI 的 gain 和 offset)
                cal->ad_data.ch3_gain = ad_c_ma_cali_data[0];
                cal->ad_data.ch3_offset = ad_c_ma_cali_data[1] * 0.001;
                cal->ad_data.ch4_gain = ad_c_ma_cali_data[2];
                cal->ad_data.ch4_offset = ad_c_ma_cali_data[3] * 0.001;
                cal->ad_data.ch5_gain = ad_c_ma_cali_data[4];
                cal->ad_data.ch5_offset = ad_c_ma_cali_data[5] * 0.001;
                cal->ad_data.ch6_gain = -ad_c_ma_cali_data[6];
                cal->ad_data.ch6_offset = -ad_c_ma_cali_data[7] * 0.001;

                cal->ad_data.ch1_gain[7] = ad_c_ma_cali_data[8];
                cal->ad_data.ch1_offset[7] = ad_c_ma_cali_data[9] * 0.001;
                cal->ad_data.ch7_gain = ad_c_ma_cali_data[10];
                cal->ad_data.ch7_offset = ad_c_ma_cali_data[11] * 0.001;
                cal->ad_data.ch1_gain[2] = ad_c_ma_cali_data[12];
                cal->ad_data.ch1_offset[2] = ad_c_ma_cali_data[13] * 0.001;
                cal->ad_data.ch1_gain[3] = -ad_c_ma_cali_data[14];
                cal->ad_data.ch1_offset[3] = -ad_c_ma_cali_data[15] * 0.001;

                cal->ad_data.ch3_gain_ua = ad_c_ua_cali_data[0];
                cal->ad_data.ch3_offset_ua = ad_c_ua_cali_data[1] * 0.001;
                cal->ad_data.ch4_gain_ua = ad_c_ua_cali_data[2];
                cal->ad_data.ch4_offset_ua = ad_c_ua_cali_data[3] * 0.001;
                cal->ad_data.ch5_gain_ua = ad_c_ua_cali_data[4];
                cal->ad_data.ch5_offset_ua = ad_c_ua_cali_data[5] * 0.001;
                cal->ad_data.ch6_gain_ua = -ad_c_ua_cali_data[6];
                cal->ad_data.ch6_offset_ua = -ad_c_ua_cali_data[7] * 0.001;

                cal->ad_data.ch1_gain_ua[7] = ad_c_ua_cali_data[8];
                cal->ad_data.ch1_offset_ua[7] = ad_c_ua_cali_data[9] * 0.001;
                cal->ad_data.ch7_gain_ua = ad_c_ua_cali_data[10];
                cal->ad_data.ch7_offset_ua = ad_c_ua_cali_data[11] * 0.001;
                cal->ad_data.ch1_gain_ua[2] = ad_c_ua_cali_data[12];
                cal->ad_data.ch1_offset_ua[2] = ad_c_ua_cali_data[13] * 0.001;
                cal->ad_data.ch1_gain_ua[3] = -ad_c_ua_cali_data[14];
                cal->ad_data.ch1_offset_ua[3] = -ad_c_ua_cali_data[15] * 0.001;

                printf("Calibration data applied successfully!\r\n");

                // ================= 打印赋值后的最终校准值 =================
                const char *pwr_names[8] = {"VCC", "IOVCC", "VSP", "VSN", "AVDD", "VDD", "ELVDD", "ELVSS"};
                const char *cur_names[8] = {"CH3", "CH4", "CH5", "CH6", "CH1[7]", "CH7", "CH1[2]", "CH1[3]"};
                printf("\r\n========== 1. DA 设定电压最终校准值 ==========\r\n");
                printf("%s: gain=%.6f, offset=%.6f\r\n", pwr_names[0], cal->da_data.vcc_set_gain, cal->da_data.vcc_set_offset);
                printf("%s: gain=%.6f, offset=%.6f\r\n", pwr_names[1], cal->da_data.iovcc_set_gain, cal->da_data.iovcc_set_offset);
                printf("%s: gain=%.6f, offset=%.6f\r\n", pwr_names[2], cal->da_data.vsp_set_gain, cal->da_data.vsp_set_offset);
                printf("%s: gain=%.6f, offset=%.6f\r\n", pwr_names[3], cal->da_data.vsn_set_gain, cal->da_data.vsn_set_offset);
                printf("%s: gain=%.6f, offset=%.6f\r\n", pwr_names[4], cal->da_data.avdd_set_gain, cal->da_data.avdd_set_offset);
                printf("%s: gain=%.6f, offset=%.6f\r\n", pwr_names[5], cal->da_data.vdd_set_gain, cal->da_data.vdd_set_offset);
                printf("%s: gain=%.6f, offset=%.6f\r\n", pwr_names[6], cal->da_data.elvdd_set_gain, cal->da_data.elvdd_set_offset);
                printf("%s: gain=%.6f, offset=%.6f\r\n", pwr_names[7], cal->da_data.elvss_set_gain, cal->da_data.elvss_set_offset);
                printf("\r\n========== 2. AD 采集电压最终校准值 (物理通道0~7) ==========\r\n");
                for (uint8_t i = 0; i < 8; i++)
                {
                    // 打印物理通道 i 的名字，以及对应的 gain 和 offset (注意 offset 已经乘了 0.001)
                    printf("PHY_CH%d: gain=%.6f, offset=%.6f\r\n", i, cal->ad_data.ch0_gain[i], cal->ad_data.ch0_offset[i]);
                }
                printf("\r\n========== 3. AD 采集 mA 电流最终校准值 ==========\r\n");
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[0], cal->ad_data.ch3_gain, cal->ad_data.ch3_offset);
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[1], cal->ad_data.ch4_gain, cal->ad_data.ch4_offset);
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[2], cal->ad_data.ch5_gain, cal->ad_data.ch5_offset);
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[3], cal->ad_data.ch6_gain, cal->ad_data.ch6_offset); // 已取反
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[4], cal->ad_data.ch1_gain[7], cal->ad_data.ch1_offset[7]);
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[5], cal->ad_data.ch7_gain, cal->ad_data.ch7_offset);
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[6], cal->ad_data.ch1_gain[2], cal->ad_data.ch1_offset[2]);
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[7], cal->ad_data.ch1_gain[3], cal->ad_data.ch1_offset[3]); // 已取反
                printf("\r\n========== 4. AD 采集 uA 电流最终校准值 ==========\r\n");
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[0], cal->ad_data.ch3_gain_ua, cal->ad_data.ch3_offset_ua);
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[1], cal->ad_data.ch4_gain_ua, cal->ad_data.ch4_offset_ua);
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[2], cal->ad_data.ch5_gain_ua, cal->ad_data.ch5_offset_ua);
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[3], cal->ad_data.ch6_gain_ua, cal->ad_data.ch6_offset_ua); // 已取反
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[4], cal->ad_data.ch1_gain_ua[7], cal->ad_data.ch1_offset_ua[7]);
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[5], cal->ad_data.ch7_gain_ua, cal->ad_data.ch7_offset_ua);
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[6], cal->ad_data.ch1_gain_ua[2], cal->ad_data.ch1_offset_ua[2]);
                printf("%s: gain=%.6f, offset=%.6f\r\n", cur_names[7], cal->ad_data.ch1_gain_ua[3], cal->ad_data.ch1_offset_ua[3]); // 已取反
                // =========================================================
                printf("Calibration data applied successfully!\r\n");
                calibration_save();

                printf("Calibration data applied successfully!\r\n");

                calibration_save();
            }
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        }
        default:
            M_SPI_DEBUG("unknow command\r\n");
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        }

        osDelay(5);
    }
}

void meter_wait_v_c_ready(uint8_t sample_id, uint8_t type)
{
    if (type == 0) // 电压
    {
        ads1256_ch_index = sample_vol_map[sample_id][0];
        d_trigger_ch_index = sample_vol_map[sample_id][1];
    }
    else if (type == 1) // 电流
    {
        ads1256_ch_index = sample_cur_map[sample_id][0];
        d_trigger_ch_index = sample_cur_map[sample_id][1];
    }

    if (ads1256_ch_index == 0 && d_trigger_ch_index != 0xff)
        bsp_ads1256_ch0_select(d_trigger_ch_index);
    else if (ads1256_ch_index == 1 && d_trigger_ch_index != 0xff)
        bsp_ads1256_ch1_select(d_trigger_ch_index);
    else if (ads1256_ch_index == 2 && d_trigger_ch_index != 0xff)
        bsp_ads1256_ch2_select(d_trigger_ch_index);
    if (ads1256_ch_index < 3 && d_trigger_ch_index != 0xff)
    {
        uint32_t t0 = HAL_GetTick();
        while (latest_sample_ch_sel[ads1256_ch_index] != d_trigger_ch_index)
        {
            if ((HAL_GetTick() - t0) >= 2000U) // 最多等待2s
            {
                g_sample_task.cmd_status = POWER_CMD_STATUS_TIMEOUT;
                M_SPI_INFO("SINGLE_VOL_GET timeout\r\n");
                break;
            }
            osDelay(1);
        }
    }
    for (uint8_t i = 0; i < 8; i++)
        wait_adc_one_round(200); // 一轮采样140ms
}
void task_sample_suspend(void)
{
    osThreadSuspend(task_sample_handle);
}

void task_sample_resume(void)
{
    osThreadResume(task_sample_handle);
}

void task_sample_task_mutex_acquire(void)
{
    osMutexAcquire(sample_mutex, osWaitForever);
}

void task_sample_task_mutex_release(void)
{
    osMutexRelease(sample_mutex);
}
void task_sample_init(void)
{
    sample_mutex = osMutexNew(&sample_mutex_attributes);
    configASSERT(sample_mutex != NULL);

    task_sample_handle = osThreadNew(task_sample_run, NULL, &task_sample_attributes);
    configASSERT(task_sample_handle != NULL);
}

/* ==================== 8. 静态私有函数实现 ==================== */
