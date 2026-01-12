#ifndef __BSP_CALIBRATION_H
#define __BSP_CALIBRATION_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "bsp_spi_flash.h"
#include "bsp_mcp4728.h"
/* 校准数据魔数和版本 */
#define CALIBRATION_MAGIC           0x505745FF  // "PWEC" - Power Enhancement Calibration
#define CALIBRATION_VERSION         1
#define CALIBRATION_MAX_VERSION     10

/* 存储地址配置 */
#define W25Q256_CALIBRATION_START   0x00300000    // 校准数据区起始地址 (3MB位置)
#define CALIBRATION_MAIN_ADDR       (W25Q256_CALIBRATION_START + 0x0000)     // 主校准数据
#define CALIBRATION_BACKUP1_ADDR    (W25Q256_CALIBRATION_START + 0x1000)     // 备份 (4KB后)

/* 扇区大小定义 */
#define CALIBRATION_SECTOR_SIZE     4096        // 扇区大小 4KB



/**
 * @brief 校准数据结构体
 */
typedef struct
{
    uint32_t magic;                 // 魔数标识: 0x50574543
    uint32_t version;               // 版本号
    uint32_t timestamp;             // 校准时间戳
    
    // ch0校准参数
    float ch0_set_v_offset;          // ch0设置电压偏移校准值
    float ch0_set_v_gain;            // ch0设置电压增益校准值
    float ch0_read_v_offset;         // ch0读取电压偏移校准值
    float ch0_read_v_gain;           // ch0读取电压增益校准值
    float ch0_read_c_offset;         // ch0读取电流偏移校准值
    float ch0_read_c_gain;           // ch0读取电流增益校准值
    
    // ch1校准参数
    float ch1_set_v_offset;          // ch1设置电压偏移校准值
    float ch1_set_v_gain;            // ch1设置电压增益校准值
    float ch1_read_v_offset;         // ch1读取电压偏移校准值
    float ch1_read_v_gain;           // ch1读取电压增益校准值
    float ch1_read_c_offset;         // ch1读取电流偏移校准值
    float ch1_read_c_gain;           // ch1读取电流增益校准值

    // ch2校准参数
    float ch2_set_v_offset;          // ch2设置电压偏移校准值
    float ch2_set_v_gain;            // ch2设置电压增益校准值
    float ch2_read_v_offset;         // ch2读取电压偏移校准值
    float ch2_read_v_gain;           // ch2读取电压增益校准值
    float ch2_read_c_offset;         // ch2读取电流偏移校准值
    float ch2_read_c_gain;           // ch2读取电流增益校准值

    // ch3校准参数
    float ch3_set_v_offset;          // ch3设置电压偏移校准值
    float ch3_set_v_gain;            // ch3设置电压增益校准值
    float ch3_read_v_offset;         // ch3读取电压偏移校准值
    float ch3_read_v_gain;           // ch3读取电压增益校准值
    float ch3_read_c_offset;         // ch3读取电流偏移校准值
    float ch3_read_c_gain;           // ch3读取电流增益校准值

    // ch4校准参数
    float ch4_set_v_offset;          // ch4设置电压偏移校准值
    float ch4_set_v_gain;            // ch4设置电压增益校准值
    float ch4_read_v_offset;         // ch4读取电压偏移校准值
    float ch4_read_v_gain;           // ch4读取电压增益校准值
    float ch4_read_c_offset;         // ch4读取电流偏移校准值
    float ch4_read_c_gain;           // ch4读取电流增益校准值

    //ch5校准参数
    float ch5_set_v_offset;          // ch5设置电压偏移校准值
    float ch5_set_v_gain;            // ch5设置电压增益校准值
    float ch5_read_v_offset;         // ch5读取电压偏移校准值
    float ch5_read_v_gain;           // ch5读取电压增益校准值
    float ch5_read_c_offset;         // ch5读取电流偏移校准值
    float ch5_read_c_gain;           // ch5读取电流增益校准值

    //ch6校准参数
    float ch6_set_v_offset;          // ch6设置电压偏移校准值
    float ch6_set_v_gain;            // ch6设置电压增益校准值
    float ch6_read_v_offset;         // ch6读取电压偏移校准值
    float ch6_read_v_gain;           // ch6读取电压增益校准值
    float ch6_read_c_offset;         // ch6读取电流偏移校准值
    float ch6_read_c_gain;           // ch6读取电流增益校准值

    //ch7校准参数
    float ch7_set_v_offset;          // ch7设置电压偏移校准值
    float ch7_set_v_gain;            // ch7设置电压增益校准值
    float ch7_read_v_offset;         // ch7读取电压偏移校准值
    float ch7_read_v_gain;           // ch7读取电压增益校准值
    float ch7_read_c_offset;         // ch7读取电流偏移校准值
    float ch7_read_c_gain;           // ch7读取电流增益校准值

    float elvss_last_voltage;       
    float iovcc_last_voltage;       
    float elvdd_last_voltage;
    float vcc_last_voltage;
    float vdd_last_voltage;
    float vsn_last_voltage;
    float vbat_last_voltage; 
    da_calibration_data_t da_data;  // DA校准数据
    
    uint32_t reserved[4];           // 保留字段（用于扩展）
    uint32_t crc32;                 // CRC32校验值（必须放在最后）
} __attribute__((packed)) calibration_data_t;

/**
 * @brief 校准数据管理器
 */
typedef struct {
    calibration_data_t data;        // 校准数据
    bool is_loaded;                 // 是否已加载
    bool is_valid;                  // 数据是否有效
    uint32_t load_attempts;         // 加载尝试次数
    uint32_t save_count;            // 保存次数
    uint32_t last_error;            // 最后一次错误码
    uint32_t flash_id;              // Flash ID
} calibration_manager_t;

/* 错误代码定义 */
#define CAL_ERROR_NONE              0x00
#define CAL_ERROR_MAGIC             0x01
#define CAL_ERROR_VERSION           0x02
#define CAL_ERROR_CRC               0x03
#define CAL_ERROR_FLASH_READ        0x04
#define CAL_ERROR_FLASH_WRITE       0x05
#define CAL_ERROR_FLASH_ERASE       0x06
#define CAL_ERROR_BACKUP_FAILED     0x07

/* 全局变量声明 */
extern calibration_manager_t g_calibration_manager;
extern CRC_HandleTypeDef hcrc;

/* 函数声明 */
HAL_StatusTypeDef calibration_init(void);
HAL_StatusTypeDef calibration_load(void);
HAL_StatusTypeDef calibration_save(void);
HAL_StatusTypeDef calibration_backup(void);
HAL_StatusTypeDef calibration_restore_from_backup(void);
HAL_StatusTypeDef calibration_set_defaults(void);
HAL_StatusTypeDef calibration_factory_reset(void);

// CRC Calculation Function
uint32_t calibration_calculate_crc32(uint8_t *data, uint32_t length);
HAL_StatusTypeDef calibration_verify_crc(calibration_data_t *cal_data);

// Data Access Function
calibration_data_t* calibration_get_data(void);
bool calibration_is_valid(void);

// 参数设置函数
HAL_StatusTypeDef calibration_update_vbat_set(float offset, float gain);
HAL_StatusTypeDef calibration_update_vbat_read(float offset, float gain);
HAL_StatusTypeDef calibration_update_vbat_current(float offset, float gain);
HAL_StatusTypeDef calibration_update_elvdd_set(float offset, float gain);
HAL_StatusTypeDef calibration_update_elvdd_read(float offset, float gain);
HAL_StatusTypeDef calibration_update_elvdd_current(float offset, float gain);
HAL_StatusTypeDef calibration_update_elvss_set(float offset, float gain);
HAL_StatusTypeDef calibration_update_elvss_read(float offset, float gain);
HAL_StatusTypeDef calibration_update_elvss_current(float offset, float gain);

// Flash操作函数
HAL_StatusTypeDef calibration_flash_init(void);


// 调试函数
void calibration_print_info(void);
void calibration_test_crc(void);
void calibration_dump_data(uint32_t addr, uint32_t size);

#endif /* __BSP_CALIBRATION_H */