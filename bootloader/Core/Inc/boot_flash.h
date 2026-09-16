/**
 * @file    boot_flash.h
 * @brief   Flash erase/program helpers for the application slot.
 */

#ifndef BOOT_FLASH_H
#define BOOT_FLASH_H

#include "main.h"

uint32_t Boot_FlashAddrToSector(uint32_t addr);
HAL_StatusTypeDef Boot_FlashEraseRange(uint32_t addr, uint32_t len);
HAL_StatusTypeDef Boot_FlashWrite(uint32_t addr, const uint8_t *data, uint32_t len);

#endif /* BOOT_FLASH_H */
