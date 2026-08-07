/************************************************************
 * @file       bsp_calibration.h
 * @brief      校准数据管理模块头文件
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-08-06
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 ************************************************************/

/*==================== 1. 头文件保护 ====================*/
#ifndef BSP_CALIBRATION_H
#define BSP_CALIBRATION_H

/*==================== 2. 头文件包含 ====================*/
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "bsp_spi_flash.h"
#include "bsp_mcp4728.h"

/*==================== 3. 宏定义 ====================*/
// 校准数据魔数和版本
#define CALIBRATION_MAGIC 0x505745FF // "PWEC" - Power Enhancement Calibration
#define CALIBRATION_VERSION 1
#define CALIBRATION_MAX_VERSION 10

// 存储地址配置
#define W25Q256_CALIBRATION_START 0x00300000
#define CALIBRATION_MAIN_ADDR (W25Q256_CALIBRATION_START + 0x0000)
#define CALIBRATION_BACKUP1_ADDR (W25Q256_CALIBRATION_START + 0x1000)

// 扇区大小定义
#define CALIBRATION_SECTOR_SIZE 4096

// 错误代码定义
#define CAL_ERROR_NONE 0x00
#define CAL_ERROR_MAGIC 0x01
#define CAL_ERROR_VERSION 0x02
#define CAL_ERROR_CRC 0x03
#define CAL_ERROR_FLASH_READ 0x04
#define CAL_ERROR_FLASH_WRITE 0x05
#define CAL_ERROR_FLASH_ERASE 0x06
#define CAL_ERROR_BACKUP_FAILED 0x07

/*==================== 4. 类型定义 ====================*/
// DA校准参数结构体
typedef struct
{
    float vcc_set_offset;
    float vcc_set_gain;
    float iovcc_set_offset;
    float iovcc_set_gain;
    float vsp_set_offset;
    float vsp_set_gain;
    float vsn_set_offset;
    float vsn_set_gain;
    float vcc_ref_offset;
    float vcc_ref_gain;
    float iovcc_ref_offset;
    float iovcc_ref_gain;
    float vsp_ref_offset;
    float vsp_ref_gain;
    float vsn_ref_offset;
    float vsn_ref_gain;
    float avdd_set_offset;
    float avdd_set_gain;
    float vdd_set_offset;
    float vdd_set_gain;
    float elvdd_set_offset;
    float elvdd_set_gain;
    float elvss_set_offset;
    float elvss_set_gain;
    float avdd_ref_offset;
    float avdd_ref_gain;
    float vdd_ref_offset;
    float vdd_ref_gain;
    float elvdd_ref_offset;
    float elvdd_ref_gain;
    float elvss_ref_offset;
    float elvss_ref_gain;
    float v_level_shift_offset;
    float v_level_shift_gain;
    float ref_freq_offset;
    float ref_freq_gain;
    float vadj_p_offset;
    float vadj_p_gain;
    float vadj_n_offset;
    float vadj_n_gain;
} da_calibration_data_t;

// AD校准参数结构体
typedef struct
{
    float ch0_offset[8];
    float ch0_gain[8];
    float ch1_offset[8];
    float ch1_gain[8];
    float ch2_offset[8];
    float ch2_gain[8];

    float ch0_offset_ua[8];
    float ch0_gain_ua[8];
    float ch1_offset_ua[8];
    float ch1_gain_ua[8];
    float ch2_offset_ua[8];
    float ch2_gain_ua[8];

    float ch3_offset;
    float ch3_gain;
    float ch4_offset;
    float ch4_gain;
    float ch5_offset;
    float ch5_gain;
    float ch6_offset;
    float ch6_gain;
    float ch7_offset;
    float ch7_gain;

    float ch3_offset_ua;
    float ch3_gain_ua;
    float ch4_offset_ua;
    float ch4_gain_ua;
    float ch5_offset_ua;
    float ch5_gain_ua;
    float ch6_offset_ua;
    float ch6_gain_ua;
    float ch7_offset_ua;
    float ch7_gain_ua;

} ad_calibration_data_t;

// 校准数据结构体
typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t timestamp;
    float vsn_last_voltage;
    float vsp_last_voltage;
    float iovcc_last_voltage;
    float vcc_last_voltage;
    float elvss_last_voltage;
    float elvdd_last_voltage;
    float vdd_last_voltage;
    float avdd_last_voltage;
    float vcc_ref_last;
    float iovcc_ref_last;
    float vsp_ref_last;
    float vsn_ref_last;
    float avdd_ref_last;
    float vdd_ref_last;
    float elvdd_ref_last;
    float elvss_ref_last;
    float ref_freq_last;
    float v_level_shift_last;
    float vadj_p_last;
    float vadj_n_last;
    da_calibration_data_t da_data;
    ad_calibration_data_t ad_data;
    uint32_t reserved[4];
    uint32_t crc32;
} calibration_data_t;

// 校准数据管理器结构体
typedef struct
{
    calibration_data_t data;
    bool is_loaded;
    bool is_valid;
    uint8_t padding[2];
    uint32_t load_attempts;
    uint32_t save_count;
    uint32_t last_error;
    uint32_t flash_id;
} calibration_manager_t;

/*==================== 5. 外部全局变量声明 ====================*/
extern calibration_manager_t g_calibration_manager;
extern CRC_HandleTypeDef hcrc;

/*==================== 6. 外部函数声明 ====================*/
HAL_StatusTypeDef calibration_init(void);
HAL_StatusTypeDef calibration_load(void);
HAL_StatusTypeDef calibration_save(void);
HAL_StatusTypeDef calibration_backup(void);
HAL_StatusTypeDef calibration_restore_from_backup(void);
HAL_StatusTypeDef calibration_set_defaults(void);
HAL_StatusTypeDef calibration_factory_reset(void);

uint32_t calibration_calculate_crc32(uint8_t *data, uint32_t length);
HAL_StatusTypeDef calibration_verify_crc(calibration_data_t *cal_data);

calibration_data_t *calibration_get_data(void);
bool calibration_is_valid(void);

HAL_StatusTypeDef calibration_flash_init(void);

void calibration_print_info(void);
void calibration_test_crc(void);
void calibration_dump_data(uint32_t addr, uint32_t size);

/*==================== 7. 结束头文件保护 ====================*/
#endif /* BSP_CALIBRATION_H */
