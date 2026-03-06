//
// Created by Bobby on 2023/10/27.
//

#ifndef AE_TOOL_MOTHER_BOARD_BOOTLOADER_BSP_IN_FLASH_H
#define AE_TOOL_MOTHER_BOARD_BOOTLOADER_BSP_IN_FLASH_H

#include "bsp.h"

#define FLASH_BASE_ADDR             (uint32_t)(FLASH_BASE)
#define FLASH_END_ADDR              (uint32_t)(0x08080000)
//#define FLASH_SINGLE_SECTOR_SIZE    (uint32_t)(0x20000)


/* Base address of the Flash sectors Bank 1 */
#define ADDR_FLASH_SECTOR_0_BANK1     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1_BANK1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2_BANK1     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3_BANK1     ((uint32_t)0x0800c000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4_BANK1     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5_BANK1     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6_BANK1     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7_BANK1     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */

int mem_init(void);
int mem_deinit(void);
int mem_erase(uint32_t address,uint32_t size);
int mem_write(uint32_t address, uint8_t *buf, uint32_t len);
int mem_read(uint32_t address,uint32_t *buf,uint32_t size);
uint16_t mem_read_half_word(uint32_t address);
uint32_t mem_get_sector(uint32_t address);
#endif //AE_TOOL_MOTHER_BOARD_BOOTLOADER_BSP_IN_FLASH_H
