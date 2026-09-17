/**
 * @file    boot_extflash.h
 * @brief   Minimal W25Q256 (SPI3) driver used for the firmware backup area.
 *
 * The external 32 MB flash stores the backup copy of the application slot.
 * The backup region starts at 4 MB, safely past the 3 MB calibration region.
 */

#ifndef BOOT_EXTFLASH_H
#define BOOT_EXTFLASH_H

#include "main.h"

/* Backup region on the external W25Q256 (32 MB). */
#define BOOT_EXT_BACKUP_ADDR 0x00400000U
#define BOOT_EXT_BACKUP_SIZE BOOT_APP_MAX_SIZE /* 512 KB */
#define BOOT_EXT_SECTOR_SIZE 4096U
#define BOOT_EXT_TOTAL_SIZE (32U * 1024U * 1024U)

void Boot_ExtFlash_Init(void);
void Boot_ExtFlash_DeInit(void);
uint8_t Boot_ExtFlash_EraseRange(uint32_t addr, uint32_t len);
uint8_t Boot_ExtFlash_Read(uint8_t *buf, uint32_t addr, uint32_t len);
uint8_t Boot_ExtFlash_Write(uint8_t *buf, uint32_t addr, uint32_t len);

#endif /* BOOT_EXTFLASH_H */
