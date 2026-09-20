/*
 * task_sample.h
 * Sampling task interface for communicating with the host
 */

/* ==================== 1. Header guard ==================== */
#ifndef _TASK_SAMPLE_H_
#define _TASK_SAMPLE_H_

/* ==================== 2. Includes ==================== */
#include "cmsis_os.h"
#include "bsp.h"
#include "task_manage.h"

/* ==================== 3. Macros ==================== */
/* None */

/* ==================== 4. Type definitions ==================== */

typedef enum
{
    POWER_CMD_STATUS_SUCCESS = 0,
    POWER_CMD_STATUS_FAILED,
    POWER_CMD_STATUS_BUSY,
    POWER_CMD_STATUS_TIMEOUT,
    POWER_CMD_STATUS_MAX
} PowerCmdStatus_E;

typedef enum
{
    GET_ID = 0x10,
    GET_SW_VERSION = 0x11,
    VOL_SET = 0x12,
    LIM_SET = 0x13,
    ALL_POWER_EN = 0x14,
    SINGLE_POWER_EN = 0x15,
    SINGLE_VOL_GET = 0x16,
    SINGLE_CUR_GET = 0x17,
    SET_ALL_POWER_VOLTAGE = 0x18,
    GET_RESISTANCE = 0x19,
    GET_DIODE = 0x1A,
    SET_FREQUENCY = 0x1B,
    GET_FREQUENCY = 0x1C,
    SEL_PIN_24 = 0x1D,
    SEL_PIN_PN = 0x1E,
    GET_24PIN_VOLTAGE = 0x1F,
    SEL_LIM_GEAR = 0x20,
    SET_BACKLIGHT_CURRENT = 0x21,
    READ_DA_DATA = 0x22,
    READ_AD_DATA = 0x23,
    WRITE_CALI_DATA = 0x24,
    enable_lim = 0x25,
    self_test = 0x26,
    CMD_ENTER_BOOT = 0x27, /* Hand control to the bootloader for firmware upgrade */
    SET_NETWORK_INFO = 0x28,
    GET_PANEL_EVENT = 0x29,
    ACK_PANEL_EVENT = 0x2A,
    NORMAL_LOOP_EVENT = 0xFF
} vol_cur_control_cmd_type;

typedef enum
{
    PANEL_EVENT_NONE = 0,
    PANEL_EVENT_NEXT_WIFI = 1
} panel_event_type_t;

typedef enum
{
    NETWORK_STATE_OFFLINE = 0,
    NETWORK_STATE_CONNECTING = 1,
    NETWORK_STATE_CONNECTED = 2,
    NETWORK_STATE_FAILED = 3
} network_state_t;

typedef struct
{
    uint8_t frame_header;
    uint8_t cmd_type;
    uint8_t power_id;
    uint8_t reserved[1];
    union
    {
        uint8_t bytes[4];
        float float_value;
    } value;

} SetPowerDataFrame_S;

typedef struct
{
    uint8_t frame_header;
    uint8_t cmd_type;
    uint8_t power_id;
    PowerCmdStatus_E cmd_status;
    union
    {
        uint8_t bytes[12];
        float float_value[3];
    } value;
} GetPowerDataFrame_S;

typedef union
{
    uint8_t bytes[4];
    float float_value;
} LimitValue_U;

typedef struct
{
    uint8_t frame_header;
    volatile uint8_t cmd_type;
    uint8_t reserved[2];

    GetPowerDataFrame_S get_power_data_frame;
    SetPowerDataFrame_S set_power_data_frame;
    PowerCmdStatus_E cmd_status;
    LimitValue_U limit_value;
    uint8_t power_switch[8]; // State of the 8 power switches

} SampleTask_S;

typedef struct
{
    uint8_t h0;
    uint8_t h1;
} protocol_header_t;
typedef struct
{
    __attribute__((aligned(4))) float vol_channel[8];
    __attribute__((aligned(4))) float cur_channel[8];
    __attribute__((aligned(4))) uint8_t vol_gear[8];
    __attribute__((aligned(4))) uint8_t cur_gear[8];
    __attribute__((aligned(4))) float print_vol_channel[8];
    __attribute__((aligned(4))) float print_cur_channel[8];
    __attribute__((aligned(4))) uint8_t print_vol_gear[8];
    __attribute__((aligned(4))) uint8_t print_cur_gear[8];
} sample_data_t;

/* ==================== 5. External global variable declarations ==================== */
/* None */

/* ==================== 6. External function declarations ==================== */
void task_sample_init(void);
void task_sample_run(void *argument);
void task_sample_suspend(void);
void task_sample_resume(void);
void task_sample_task_mutex_acquire(void);
void task_sample_panel_event_post(panel_event_type_t event_type);
osStatus_t task_sample_task_mutex_try_acquire(void);
void task_sample_task_mutex_release(void);
void meter_wait_v_c_ready(uint8_t sample_id, uint8_t type);

/* ==================== 7. End of header guard ==================== */
#endif /* _TASK_SAMPLE_H_ */
