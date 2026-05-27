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
#include "utils.h"
#include "bsp.h"
#include "widget_func.h"
#include "math.h"

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
        [PROTO_GET_ID] = {0xA0, 0x10},                    // 获取 ID
        [PROTO_GET_VERSION] = {0xA0, 0x11},               // 获取软件版本号
        [PROTO_SET_VOLTAGE] = {0xA0, 0x12},               // 电压配置
        [PROTO_SET_CURRENT_LIM] = {0xA0, 0x13},           // 限流配置
        [PROTO_SET_ALL_POWER_EN] = {0xA0, 0x14},          // 所有电源使能
        [PROTO_SET_POWER_EN] = {0xA0, 0x15},              // 单路电源使能
        [PROTO_GET_VOLTAGE] = {0xA0, 0x16},               // 单路电源电压获取
        [PROTO_GET_CURRENT] = {0xA0, 0x17},               // 单路电源电流获取
        [PROTO_SET_ALL_POWER_VOLTAGE] = {0xA0, 0x18},     // 所有电源电压配置
        [PROTO_SET_ALL_POWER_CURRENT_LIM] = {0xA0, 0x19}, // 所有电源限流配置
};

osMutexId_t sample_mutex;
osStaticMutexDef_t sample_mutex_control_block;
const osMutexAttr_t sample_mutex_attributes = {
    .name = "show_mutex",
    .cb_mem = &sample_mutex_control_block,
    .cb_size = sizeof(sample_mutex_control_block),
};
extern volatile uint8_t meter_com_flag;
extern uint8_t meter_rx_buf[SPI2_SLAVE_RX_LEN];
extern uint8_t meter_tx_buf[SPI2_SLAVE_TX_LEN];
extern volatile TEST_R_D_RES_LEVEL r_level_selected;
extern ads1256_dev_t dev_vol;
// 一轮完整的采样流程：
#define WAIT_ADC_1_IDLE           \
    while (dev_vol.step_cnt != 6) \
    {                             \
        osDelay(10);              \
    }                             \
    extern __IO uint32_t uwDutyCycle;
/* Frequency Value */
extern __IO uint32_t uwFrequency;
extern uint8_t get_freq_flag;
sample_data_t sample_data;
SampleTask_S g_sample_task = {0};
uint8_t sample_vol_id = 0;
uint8_t sample_cur_id = 0;
uint8_t ads1256_ch_index = 0;
uint8_t d_trigger_ch_index = 0;
extern lcd_show_t lcd_show;

static inline int get_VSN_status(void) { return !bsp_d_trigger_get_channel_state(&d_3, 0); }
static inline int get_ELVSS_status(void) { return !bsp_d_trigger_get_channel_state(&d_3, 1); }
static inline int get_ELVDD_status(void) { return bsp_d_trigger_get_channel_state(&d_3, 2); }
static inline int get_VDD_status(void) { return bsp_d_trigger_get_channel_state(&d_3, 3); }
static inline int get_AVDD_status(void) { return bsp_d_trigger_get_channel_state(&d_3, 4); }
static inline int get_VSP_status(void) { return bsp_d_trigger_get_channel_state(&d_3, 5); }
static inline int get_IOVCC_status(void) { return bsp_d_trigger_get_channel_state(&d_3, 6); }
static inline int get_VCC_status(void) { return bsp_d_trigger_get_channel_state(&d_3, 7); }

typedef int (*power_status_func_t)(void);

power_status_func_t power_enable_status[8] = {
    get_VCC_status,
    get_IOVCC_status,
    get_VSP_status,
    get_VSN_status,
    get_AVDD_status,
    get_VDD_status,
    get_ELVDD_status,
    get_ELVSS_status};
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
int find_sample_vol_map_index(uint8_t chip_index, uint8_t d_trigger_ch_index)
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
int find_sample_cur_map_index(uint8_t chip_index, uint8_t d_trigger_ch_index)
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

void task_sample_run()
{

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
    static const uint8_t power_order[8] = {0, 1, 2, 3, 8, 9, 10, 11};
    static const uint8_t set_power_order[20] = {0, 1, 2, 3, 8, 9, 10, 11, 4, 5, 6, 7, 12, 13, 14, 15, 16, 17, 18, 19};
    for (;;)
    {
        switch (g_sample_task.cmd_type)
        {
        case NORMAL_LOOP_EVENT:
            task_sample_task_mutex_acquire(); // 通信时无法获取互斥锁,空闲时更新采样数据到显示屏上
            for (uint8_t i = 0; i < 8; i++)
            {
                // printf("chip_index: %d, d_trigger_ch_index: %d\n", i, latest_sample_ch_sel[sample_vol_map[i][0]]);
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
                // printf("latest_sample_ch_sel[%d]: %d, idx_vol: %d, idx_cur: %d\n", i, latest_sample_ch_sel[i], idx_vol, idx_cur);

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
                        // printf("idx_vol: %d, voltage: %f\n", idx_vol, latest_sample_data[i]);
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
                        lcd_show.current[idx_cur] = latest_sample_data[i];
                    else
                        lcd_show.current[idx_cur] = 0; // 未使能
                }
                if (idx_vol == -1 && idx_cur == -1)
                {
                    // printf("no idx found for chip_index: %d, d_trigger_ch_index: %d\n", i, latest_sample_ch_sel[sample_vol_map[i][0]]);
                    continue;
                }
            }
            task_sample_task_mutex_release();
            osDelay(5);
            break;
        case GET_ID:
            meter_tx_buf[3] = id; // 1,2,3,4:bit1~4
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
            g_sample_task.set_power_data_frame.frame_header = meter_rx_buf[0];
            g_sample_task.set_power_data_frame.cmd_type = meter_rx_buf[1];
            g_sample_task.set_power_data_frame.power_id = meter_rx_buf[2];
            memcpy(&float_bytes, &meter_rx_buf[3], sizeof(float_bytes));
            memcpy(g_sample_task.set_power_data_frame.value.bytes, float_bytes.b, sizeof(float_bytes.b));
            g_sample_task.set_power_data_frame.power_id = set_power_order[g_sample_task.set_power_data_frame.power_id];
            M_SPI_DEBUG("set_power_data_frame.power_id:%x\r\n", g_sample_task.set_power_data_frame.power_id);
            M_SPI_DEBUG("set_power_data_frame.value.bytes:%02X %02X %02X %02X\r\n",
                        float_bytes.b[0], float_bytes.b[1], float_bytes.b[2], float_bytes.b[3]);
            M_SPI_DEBUG("set_power_data_frame.value.float_value:%f\r\n", float_bytes.f);

            *(dac_config_table[g_sample_task.set_power_data_frame.power_id].last_voltage) = float_bytes.f;

            bsp_cali_and_set_power(g_sample_task.set_power_data_frame.power_id);
            // calibration_save();
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
            MIPI_CMD_DEBUG("power_id:%d, en:%d\r\n", power_id, en);

            power_id = power_order[power_id];
            if (en == 0x01)
                bsp_power_single_enable(power_id);
            else
                bsp_power_single_disable(power_id);
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case SINGLE_VOL_GET:
            M_SPI_DEBUG("SINGLE_VOL_GET\r\n");

            sample_vol_id = meter_rx_buf[2];
            M_SPI_DEBUG("sample_vol_id: %d\r\n", sample_vol_id);

            meter_wait_v_c_ready(sample_vol_id, 0);
            // 测完24pin和40pin关闭24pin和40pin的通道,避免干扰
            if (sample_vol_id == 10)
                bsp_close_24pin_channel();
            if (sample_vol_id == 9)
                bsp_close_40pin_channel();

            M_SPI_INFO("ads1256_ch_index: %d, d_trigger_ch_index: %d, latest_sample_ch_sel: %d\r\n", ads1256_ch_index, d_trigger_ch_index, latest_sample_ch_sel[ads1256_ch_index]);
            memcpy(&meter_tx_buf[3], (const void *)&latest_sample_data[ads1256_ch_index], sizeof(float));
            M_SPI_DEBUG("SINGLE_VOL_GET: channel %d, voltage %f\r\n", ads1256_ch_index, latest_sample_data[ads1256_ch_index]);
            M_SPI_DEBUG("latest_sample_raw_data: %f\r\n", latest_sample_raw_data[ads1256_ch_index]);
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case SINGLE_CUR_GET:
            sample_cur_id = meter_rx_buf[2];
            meter_wait_v_c_ready(sample_cur_id, 1);
            M_SPI_INFO("ads1256_ch_index: %d, d_trigger_ch_index: %d, latest_sample_ch_sel: %d\r\n", ads1256_ch_index, d_trigger_ch_index, latest_sample_ch_sel[ads1256_ch_index]);
            memcpy(&meter_tx_buf[3], (const void *)&latest_sample_data[ads1256_ch_index], sizeof(float));
            M_SPI_DEBUG("SINGLE_CUR_GET: channel %d, current %f\r\n", ads1256_ch_index, latest_sample_data[ads1256_ch_index]);
            M_SPI_DEBUG("latest_sample_raw_data: %f\r\n", latest_sample_raw_data[ads1256_ch_index]);
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case GET_FREQUENCY:
            pin_num = meter_rx_buf[2];
            pin_num = pin_num - 1; // 外部传入的pin_num从1开始,这里转换成从0开始
            bsp_select_24pin_channel(pin_num, 1);
            memcpy(&ref_freq_vol, &meter_rx_buf[3], sizeof(float));
            printf("%f\r\n", ref_freq_vol);

            bsp_select_24pin_channel(pin_num, 1);
            bsp_d_trigger_set_channel(&d_1, 6, 1);
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

            // 恢复环境
            bsp_select_24pin_channel(pin_num, 0);
            bsp_led_pwm_init(10);
            enableTim1PWMOutput();
            printf("Result: Freq: %lu Hz\n", uwFrequency);
            float freq_f = (float)uwFrequency;
            printf("freq_f: %f\r\n", freq_f);
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
            HAL_GPIO_WritePin(PULSE_A_GPIO_Port, PULSE_A_Pin, GPIO_PIN_SET); // 测试用
            printf("t_temp_start: %lu\r\n", t_temp_start);

            M_SPI_DEBUG("GET_RESISTANCE: pin_p %d, pin_n %d, r_level %d\r\n", pin_p, pin_n, dev_vol.sample_res_gear_rd);

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
            wait_adc_one_round(1000); // 至少等6轮,从档六开始切
            wait_adc_one_round(1000);
            wait_adc_one_round(1000);
            wait_adc_one_round(1000);
            wait_adc_one_round(1000);
            wait_adc_one_round(1000);
            wait_adc_one_round(1000);
            // bsp_delay_ms(100);
            memcpy(&meter_tx_buf[3], (const void *)&latest_sample_data[2], sizeof(float));
            t_temp_end = HAL_GetTick();
            printf("t_temp_end: %lu\r\n", t_temp_end);
            printf("resistance measurement time: %lu ms\r\n", t_temp_end - t_temp_start);
            M_SPI_DEBUG("GET RESISTANCE: %f\r\n", latest_sample_data[2]);
            M_SPI_DEBUG("latest_sample_raw_data: %f\r\n", latest_sample_raw_data[2]);
            printf("pin_p: %d, pin_n: %d, resistance: %f m\r\n", pin_p, pin_n, latest_sample_data[2] / 1000000);
            printf("final sample_res_gear_rd: %d\r\n", dev_vol.sample_res_gear_rd);
            bsp_rd_select_mode(R_D_MODE_NULL);
            bsp_rd_select_pin(pin_p, pin_n, 0);
            dev_vol.channel_en = 0b11111111; // 使能所有通道
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case GET_DIODE:
            pin_p = meter_rx_buf[2];
            pin_n = meter_rx_buf[3];
            bsp_rd_select_r_level(OHM_4_point_7_K);
            bsp_rd_select_pin(pin_p, pin_n, 1);
            bsp_rd_select_mode(D_MODE);
            M_SPI_DEBUG("GET_DIODE: pin_p %d, pin_n %d, r_level %d\r\n", pin_p, pin_n, dev_vol.sample_res_gear_rd);
            bsp_ads1256_ch2_select(0);
            while (dev_vol.work_channel != 2)
            {
                osDelay(1);
            };
            wait_adc_one_round(1000);

            M_SPI_DEBUG("Waiting for ADS1256 channel 2 sub channel 0 data ready...\r\n");
            t0 = HAL_GetTick();
            while (latest_sample_ch_sel[2] != 0) // 等待ADS1256通道2的数据准备好
            {
                if ((HAL_GetTick() - t0) >= 1000U) // 最多等待1s
                {
                    // 可按你的状态定义改成超时状态
                    g_sample_task.cmd_status = POWER_CMD_STATUS_TIMEOUT;
                    M_SPI_INFO("GET_RESISTANCE timeout\r\n");
                    break;
                }
                osDelay(1);
            }

            memcpy(&meter_tx_buf[3], (const void *)&latest_sample_data[2], sizeof(float));

            M_SPI_DEBUG("GET DIODE: %f\r\n", latest_sample_data[2]);
            printf("pin_p: %d, pin_n: %d, vol: %f\r\n", pin_p, pin_n, latest_sample_data[2]);
            bsp_rd_select_pin(pin_p, pin_n, 0);
            task_com_resume();
            g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
            break;
        case SET_RESISTANCE:
            r_level = meter_rx_buf[2];
            bsp_rd_select_r_level(r_level);
            M_SPI_DEBUG("SET_RESISTANCE: r_level %d\r\n", r_level);
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
        default:
            M_SPI_DEBUG("unknow command\r\n");
            break;
        }

        osDelay(5);
    }
}

void meter_wait_v_c_ready(uint8_t sample_id, uint8_t type)
{
    if (type == 0) // 电压
    {
        ads1256_ch_index = sample_vol_map[sample_vol_id][0];
        d_trigger_ch_index = sample_vol_map[sample_vol_id][1];
    }
    else if (type == 1) // 电流
    {
        ads1256_ch_index = sample_cur_map[sample_cur_id][0];
        d_trigger_ch_index = sample_cur_map[sample_cur_id][1];
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
    task_sample_handle = osThreadNew(task_sample_run, NULL, &task_sample_attributes);
}
