/**
 * @file       bsp_gtb.h
 * @brief      RA_PowerEX
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-10-13
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

#ifndef __BSP_GTB_H
#define __BSP_GTB_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "debug.h"
/* Exported types ------------------------------------------------------------*/
typedef struct{
    bool ex_ti_flag;
    bool transfer_flag;
    bool cs_low_en;
    bool cs_high_en;
    bool debug_enable;
    bool flash_program_flash;
    bool int_trans;
    bool int_flag;
    uint32_t fw_len;
    uint8_t fw_mode;
    bool one_cs_for_host_download_enable;
    uint16_t ic_touch_data_len;
    uint16_t ic_touch_data_len1;
    uint16_t ic_touch_data_index;
    uint8_t ic_type_index;
    bool long_packet_enable;
    bool raw_data_flag;
    uint16_t long_packet_count;
    uint8_t offline_mode;
    bool download_enable;
    bool download_complete;
    bool transfer_feedback_enable;
    uint16_t spi_i2c_data_len;
    uint16_t spi_i2c_count;
    bool interface_mode;
    uint8_t dynamic_cmd_count;
    uint8_t dynamic_total_cmd_num;
    uint16_t dynamic_cmd_data_len;
    HAL_StatusTypeDef transfer_status;
    int test_count;
    int retry_count;
    uint8_t raw_data_delay_time;
    bool ipx_format;
    uint8_t i2c_slave_address;				//I2C slave address,7 bit
    uint8_t i2c_speed_mode;					//I2C speed select,0 for 100K,1 for 400k
    uint32_t flash_address;					//flash address,bit0~30
    uint32_t flash_id;
    uint32_t flash_data_len;
    uint8_t flash_ret;
    uint16_t report_num;
    uint8_t fw_report_cmd;
    uint8_t fw_spi_data_out;
    uint8_t fw_data_len;
    uint8_t fw_data_len1;
}tp_config_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

void tp_spi_set_speed(uint8_t speed);

void tp_spi_set_mode(uint8_t mode);

void tp_i2c_set_speed(uint8_t speed);

void bsp_gtb_init(uint8_t mode);

HAL_StatusTypeDef spi_read_write_data1( uint8_t *write_data, uint8_t *read_data,
                                        uint16_t read_size,uint16_t write_size);
HAL_StatusTypeDef spi_read_write_data2( uint8_t *write_data, uint8_t *read_data,
                                        uint16_t read_size,uint16_t write_size);
HAL_StatusTypeDef spi_read_write_data3( uint8_t *write_data, uint8_t *read_data,
                                        uint16_t read_size,uint16_t write_size);

HAL_StatusTypeDef spi_read_data2( uint8_t *read_data,uint16_t read_size);

HAL_StatusTypeDef tp_spi_read_write_byte(uint8_t byte);

void tp_spi_cs_enable(bool state);

HAL_StatusTypeDef gtb_write_data(tp_config_t *tp_config,bool mode,uint8_t *pData,uint32_t data_len);

HAL_StatusTypeDef gtb_read_data(tp_config_t *tp_config,bool mode,uint8_t *read_cmd,uint8_t *read_data,uint32_t data_len);

void ex_ti_initial(tp_config_t *tp_config,FunctionalState state);

HAL_StatusTypeDef i2c_write_data(uint8_t slave_address,uint8_t *write_data,uint16_t write_size);

HAL_StatusTypeDef i2c_read_data(uint8_t slave_address,uint8_t *read_data,uint16_t read_size);

void tp_int_mode_change(uint8_t mode);
#endif /* __BSP_GTB_H */

