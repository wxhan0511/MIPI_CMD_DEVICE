/*
 * task_sample.h
 *  与上位机通信
 *  Created on: Jun 30, 2025
 *      Author: Wenxiao Han
 */

#ifndef _TASK_SAMPLE_H_
#define _TASK_SAMPLE_H_

#include "cmsis_os.h"
#include "bsp.h"
#include "task_manage.h"

typedef enum {
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
    SET_RESISTANCE = 0x18,
    GET_RESISTANCE = 0x19,
    GET_DIODE = 0x1A,
    SET_FREQUENCY = 0x1B,
    GET_FREQUENCY = 0x1C,
    SEL_PIN_24 = 0x1D,
    SEL_PIN_PN = 0x1E,
    GET_24PIN_VOLTAGE = 0x1F
} vol_cur_control_cmd_type;

typedef struct {
    uint8_t frame_header;
    uint8_t cmd_type;                 
    uint8_t power_id;
    uint8_t reserved[1];                
    union {
        uint8_t bytes[4];        
        float float_value;       
    } value;
                        
} SetPowerDataFrame_S;

typedef struct {
    uint8_t frame_header;             
    uint8_t cmd_type;                 
    uint8_t power_id;               
    PowerCmdStatus_E cmd_status;      
    union {
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
    uint8_t cmd_type;
    uint8_t reserved[2];

    GetPowerDataFrame_S get_power_data_frame;
    SetPowerDataFrame_S set_power_data_frame;
    PowerCmdStatus_E cmd_status; 
    LimitValue_U limit_value;
    uint8_t power_switch[8]; // 8个电源开关状态

} SampleTask_S;

typedef struct
{
    uint8_t h0;
    uint8_t h1;
} protocol_header_t;

void task_sample_init(void);
void task_sample_run();
/* 在 SPI 中断回调里调用（ISR上下文） */
void task_sample_spi_frame_isr(const uint8_t *rx, uint16_t len);

/* 供 SPI 层取当前要回传的帧 */
void task_sample_get_tx_frame(uint8_t *out, uint16_t len);
#endif /* _TASK_SAMPLE_H_ */
