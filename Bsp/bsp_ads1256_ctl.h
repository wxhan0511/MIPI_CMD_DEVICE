/**
 * @file       bsp_ads1256_ctl.h
 * @brief      RA_PowerEX
 * @author     wxhan
 * @version    1.0.0
 * @date       2026-01-29
 * @copyright  Copyright (c) 2026 gcoreinc
 * @license    MIT License
 */

#ifndef __BSP_ADS1256_CTL_H
#define __BSP_ADS1256_CTL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

// ADC通道枚举定义
typedef enum {
    ADC_CH_VBAT     = 0,    // ADC0: AD_V_VBAT -  VABT电压
    ADC_CH_ELVDD    = 1,    // ADC1: AD_V_ELVDD - ELVDD电压
    ADC_CH_ELVSS    = 2,    // ADC2: AD_V_ELVSS - ELVSS电压
    ADC_CH_VBAT_I   = 3,    // ADC3: AD_VBAT_I -  VBAT电流
    ADC_CH_ELVDD_I  = 4,    // ADC4: AD_I_ELVDD - ELVDD电流
    ADC_CH_VREF     = 5,    // ADC5: 基准电压
    ADC_CH_ELVSS_I  = 6,    // ADC6: AD_I_ELVSS - ELVSS电流
    ADC_CH_MAX      = 7     // 最大通道数
} adc_channel_t;


typedef enum {
    GET_SOFTWARE_ID             = 0x40,
    GET_HARDWARE_ID             = 0x41,
    SET_POWER_VOLTAGE           = 0x50,
    GET_POWER_VOLTAGE           = 0x51,
    GET_POWER_CURRENT           = 0x52,
    GET_3CH_POWER_VOLTAGE       = 0x53,
    GET_3CH_POWER_CURRENT       = 0x54,
    FACTORY_SETTING             = 0x60,
    POWER_ON_SEQUENCE_SETTING   = 0x61
} power_board_cmd_t;



typedef struct {
    uint8_t header;
    uint8_t cmd;
    uint8_t power_name;
    uint8_t cmd_status;
	union {
		uint8_t bytes[12];
		float float_value[3];
	} value;
} power_data_frame;

typedef struct __attribute__((packed)){
    uint8_t header;
    uint8_t cmd;
    uint8_t power_name;
	union {
		uint8_t bytes[4];
		float float_value[1];
	} value;
} set_power_data_frame;


typedef enum
{
	VBAT = 0x00,
	ELVDD,
	ELVSS
} power_name;


/* Exported constants --------------------------------------------------------*/
extern volatile uint16_t raw_data_queue_head;
extern float latest_sample_data[8];
extern double raw_data;
/* Exported macro ------------------------------------------------------------*/
#define RAW_DATA_QUEUE_SIZE  (4092 / sizeof(float)) 
#define RAW_DATA_INDEX_QUEUE_SIZE  4092
#define ADC_DEBUG 1

#define AVG_CNT 1

#define ADC_RATIO (0.596)

#define SINGLE_VOL_CHANGE_GEAR 0
/* Exported functions prototypes ---------------------------------------------*/
void raw_data_queue_push(float value , uint8_t index);
uint16_t raw_data_queue_get_count(void);
float raw_data_queue_get_data(uint16_t index);
uint8_t raw_data_queue_get_index(uint16_t index);
void sample_data_cali();
extern uint8_t latest_sample_ch_sel[8];
#ifdef __cplusplus
}
#endif

#endif /* __BSP_ADS1256_CTL_H */
