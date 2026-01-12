//
// Created by Bobby on 2023/8/9.
//

#ifndef AE_TOOL_MOTHER_BOARD_GTB_FLASH_OP_H
#define AE_TOOL_MOTHER_BOARD_GTB_FLASH_OP_H

#include <stdbool.h>
#include "main.h"
#include "bsp_gtb.h"
bool fspi_init(tp_config_t *tp_config);

bool fspi_cs_enable(tp_config_t *tp_config,bool en);

void drv_flash_wait_for_write_end( void );

void drv_flash_wait_for_write_enable_latch( void );

uint8_t drv_flash_read_security_reg( void );

void drv_flash_write_enable( void );

uint8_t drv_flash_sector_erase( uint32_t sector_addr );

uint8_t drv_spi_flash_block_64_erase( uint32_t block_addr );

uint8_t drv_spi_flash_page_write( uint32_t write_addr, uint8_t* write_buffer, uint32_t num );

//ok:2,fail:0
uint8_t drv_spi_flash_write(uint32_t write_addr, uint8_t* write_buffer, uint16_t num);

void drv_spi_flash_read( uint32_t addr, uint8_t *readBuffer, uint32_t numByteRead );

uint32_t drv_spi_flash_read_id( void );

uint16_t drv_spi_flash_crc_16( unsigned char *pcDataStream, unsigned short sNumberOfDataBytes );

void drv_spi_flash_power_down( void );

void drv_spi_flash_power_wake_up( void );

bool fspi_addr_write(tp_config_t *tp_config,uint32_t addr,uint8_t *data,uint16_t length);

bool fspi_write(tp_config_t *tp_config,uint8_t *data,uint16_t length);

bool fspi_read(tp_config_t *tp_config,uint8_t *data);

bool fspi_flash_write_wait_for_enable_latch(tp_config_t *tp_config);

bool fspi_flash_wait_for_write_end(tp_config_t *tp_config,uint32_t timeout);


#endif //AE_TOOL_MOTHER_BOARD_GTB_FLASH_OP_H
