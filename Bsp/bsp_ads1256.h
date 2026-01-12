/**
 * @file       bsp_ads1256.h
 * @brief      RA_PowerEX
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-08-28
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */


#ifndef BSP_ADS1256_H
#define BSP_ADS1256_H

#include <bsp.h>

extern void raw_data_queue_push(float value , uint8_t index);
extern uint16_t raw_data_queue_get_count(void);
extern float raw_data_queue_get_data(uint16_t index);
extern uint8_t raw_data_queue_get_index(uint16_t index);

extern volatile uint16_t raw_data_queue_head;
extern float latest_sample_data[8];
#define RAW_DATA_QUEUE_SIZE  (4092 / sizeof(float)) 
#define RAW_DATA_INDEX_QUEUE_SIZE  4092

typedef enum
{
    REG_STATUS = 0x00,
    REG_MUX,
    REG_ADCON,
    REG_DRATE,
    REG_IO,
    REG_OFC0,
    REG_OFC1,
    REG_OFC2,
    REG_FSC0,
    REG_FSC1,
    REG_FSC2,
}ADS1256_REG;

typedef enum
{
	CMD_WAKEUP  = 0x00,	// Completes SYNC and Exits Standby Mode 0000  0000 (00h)
	CMD_RDATA   = 0x01, // Read Data 0000  0001 (01h)
	CMD_RDATAC  = 0x03, // Read Data Continuously 0000   0011 (03h)
	CMD_SDATAC  = 0x0F, // Stop Read Data Continuously 0000   1111 (0Fh)
	CMD_RREG    = 0x10, // Read from REG rrr 0001 rrrr (1xh)
	CMD_WREG    = 0x50, // Write to REG rrr 0101 rrrr (5xh)
	CMD_SELFCAL = 0xF0, // Offset and Gain Self-Calibration 1111    0000 (F0h)
	CMD_SELFOCAL= 0xF1, // Offset Self-Calibration 1111    0001 (F1h)
	CMD_SELFGCAL= 0xF2, // Gain Self-Calibration 1111    0010 (F2h)
	CMD_SYSOCAL = 0xF3, // System Offset Calibration 1111   0011 (F3h)
	CMD_SYSGCAL = 0xF4, // System Gain Calibration 1111    0100 (F4h)
	CMD_SYNC    = 0xFC, // Synchronize the A/D Conversion 1111   1100 (FCh)
	CMD_STANDBY = 0xFD, // Begin Standby Mode 1111   1101 (FDh)
	CMD_RESET   = 0xFE, // Reset to Power-Up Values 1111   1110 (FEh)
}ADS1256_CMD;


typedef enum
{
	NORMAL_MODE  = 0x00,
	IRQ_MODE,
}ADS1256_WORK_MODE;

typedef enum
{
	GEAR_OL,
	GEAR_OK,
}SAMPLE_GEAR_STATUS;


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

typedef enum
{
	success = 0x00,
	busy,
	timeout
} cmd_status;

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

/* The corresponding channel number is also used as a command */
typedef enum {
    AD_I_IOVCC          = 0x0,
    AD_I_VCC            = 0x1,
    AD_I_ELVDD          = 0x2,
    AD_I_ELVSS          = 0x3,
    AD_V_ELVSS          = 0x4,
    AD_V_VCC            = 0x5,
    AD_V_ELVDD          = 0x6,
    AD_V_IOVCC          = 0x7,
    
} ra_powername_cmd_t;

typedef struct ads1256_dev
{
	uint8_t step_cnt;
	uint8_t channel_en;

	uint8_t auto_change_gear;
	ADS1256_WORK_MODE work_mode;
	uint8_t work_channel;
	__IO uint8_t last_channel;
	uint8_t sample_cnt[8];
	int32_t data_buffer[8][10];
	__IO double data_buffer_avg[8];
    GPIO_TypeDef* cs_group;
    uint16_t cs_pin;
	GPIO_TypeDef* drdy_group;
	uint16_t drdy_pin;
	uint8_t sample_res_gear[8];//VSN VSP VCC IOVCC VDD R&D BIAS+ BIAS-
	uint8_t sample_res_gear_rd;
	SAMPLE_GEAR_STATUS sample_status[8];
	void (*cs_control)(GPIO_TypeDef* group,uint16_t pin,uint8_t state);
	BSP_STATUS (*read_reg)(const struct ads1256_dev *handle, uint8_t first_cmd,uint8_t *read_data, uint8_t reg_num);
	BSP_STATUS (*write_reg)(const struct ads1256_dev *handle, uint8_t first_cmd, const uint8_t *write_data, uint8_t reg_num);
	BSP_STATUS (*write_byte)(const struct ads1256_dev *handle,uint8_t data);
	BSP_STATUS (*read_byte)(const struct ads1256_dev *handle,uint8_t *data);
}ads1256_dev_t;


extern ads1256_dev_t dev_h_m_interface_board;
extern ads1256_dev_t dev_ic_power_board;

typedef enum
{
    GAIN_1,
    GAIN_2,
    GAIN_4,
    GAIN_8,
    GAIN_16,
    GAIN_32,
    GAIN_64,
}ADS1256_GAIN;

typedef enum
{
    SPS_30000,
    SPS_15000,
    SPS_7500,
    SPS_3750,
    SPS_2000,
    SPS_1000,
    SPS_500,
    SPS_100,
    SPS_60,
    SPS_50,
    SPS_30,
    SPS_25,
    SPS_15,
    SPS_10,
    SPS_5,
    SPS_2_5
}ADS1256_SPS;

extern ads1256_dev_t dev_adc;

void bsp_ads1256_init(const ads1256_dev_t *handle);

BSP_STATUS bsp_ads1256_set_single_channel(const ads1256_dev_t *handle, uint8_t channel);

void bsp_ads1256_config(ADS1256_GAIN gain,ADS1256_SPS sps);

BSP_STATUS bsp_ads1256_write_byte(const ads1256_dev_t *handle,uint8_t data);

BSP_STATUS bsp_ads1256_read_byte(const ads1256_dev_t *handle,uint8_t *data);

BSP_STATUS bsp_ads1256_read_data(const ads1256_dev_t *handle,int32_t *val);

BSP_STATUS bsp_ads1256_read_reg(const ads1256_dev_t *handle, uint8_t first_cmd,uint8_t *read_data, uint8_t reg_num);

BSP_STATUS bsp_ads1256_write_reg(const ads1256_dev_t *handle, uint8_t first_cmd, const uint8_t *write_data, uint8_t reg_num);

BSP_STATUS bsp_ads1256_read_id(const ads1256_dev_t *handle,uint8_t *id);

void bsp_ads1256_irq_enable(const ads1256_dev_t *handle);

void bsp_ads1256_irq_handle_voltage(ads1256_dev_t *handle);

void bsp_ads1256_irq_handle_current(ads1256_dev_t *handle);

void bsp_ads1256_read_channel_data(const ads1256_dev_t* handle, uint8_t channel,int32_t *data);

void bsp_ads1256_sync_wakeup(const ads1256_dev_t* handle);

void bsp_ads1256_irq_handle(ads1256_dev_t* handle);


#endif //BSP_ADS1256_H
