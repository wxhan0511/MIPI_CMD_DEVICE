/**
 * @file    boot_flash.c
 * @brief   Flash erase/program helpers for the STM32F407 application slot.
 *
 * The F407 flash erases whole sectors only, so Boot_FlashEraseRange() walks
 * the sector table and erases every sector touched by [addr, addr+len).
 * Programming uses 32-bit words; short tails are padded with 0xFF.
 */

#include "boot_flash.h"

/* STM32F407VGT6 sector map (1 MB) */
typedef struct
{
  uint32_t start;
  uint32_t size;
} flash_sector_t;

static const flash_sector_t sector_map[] = {
    {0x08000000U, 16U * 1024U},  /* Sector 0  */
    {0x08004000U, 16U * 1024U},  /* Sector 1  */
    {0x08008000U, 16U * 1024U},  /* Sector 2  */
    {0x0800C000U, 16U * 1024U},  /* Sector 3  */
    {0x08010000U, 64U * 1024U},  /* Sector 4  */
    {0x08020000U, 128U * 1024U}, /* Sector 5  */
    {0x08040000U, 128U * 1024U}, /* Sector 6  */
    {0x08060000U, 128U * 1024U}, /* Sector 7  */
    {0x08080000U, 128U * 1024U}, /* Sector 8  */
    {0x080A0000U, 128U * 1024U}, /* Sector 9  */
    {0x080C0000U, 128U * 1024U}, /* Sector 10 */
    {0x080E0000U, 128U * 1024U}, /* Sector 11 */
};

#define SECTOR_COUNT (sizeof(sector_map) / sizeof(sector_map[0]))

uint32_t Boot_FlashAddrToSector(uint32_t addr)
{
  for (uint32_t i = 0; i < SECTOR_COUNT; i++)
  {
    uint32_t start = sector_map[i].start;
    uint32_t end = start + sector_map[i].size;
    if (addr >= start && addr < end)
    {
      return i; /* Sector number used by HAL_FLASH_Erase_Sector */
    }
  }
  return 0xFFFFFFFFU; /* Invalid address */
}

HAL_StatusTypeDef Boot_FlashEraseRange(uint32_t addr, uint32_t len)
{
  if (len == 0U)
  {
    return HAL_OK;
  }
  if ((addr < BOOT_APP_ADDRESS) || ((addr + len) > BOOT_APP_END))
  {
    return HAL_ERROR; /* Only the application slot may be erased */
  }

  uint32_t first = Boot_FlashAddrToSector(addr);
  uint32_t last = Boot_FlashAddrToSector(addr + len - 1U);
  if (first == 0xFFFFFFFFU || last == 0xFFFFFFFFU)
  {
    return HAL_ERROR;
  }

  HAL_FLASH_Unlock();

  FLASH_EraseInitTypeDef erase = {0};
  uint32_t sector_error = 0U;
  erase.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase.Sector = first;
  erase.NbSectors = (last - first) + 1U;
  erase.VoltageRange = FLASH_VOLTAGE_RANGE_3; /* 3.3 V supply: 32-bit parallelism */

  HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase, &sector_error);

  HAL_FLASH_Lock();
  return status;
}

HAL_StatusTypeDef Boot_FlashWrite(uint32_t addr, const uint8_t *data, uint32_t len)
{
  if (len == 0U)
  {
    return HAL_OK;
  }
  if ((addr < BOOT_APP_ADDRESS) || ((addr + len) > BOOT_APP_END))
  {
    return HAL_ERROR;
  }
  if ((addr % 4U) != 0U)
  {
    return HAL_ERROR; /* Word programming requires 4-byte alignment */
  }

  HAL_FLASH_Unlock();

  uint32_t word_addr = addr;
  uint32_t i = 0U;
  while (i < len)
  {
    uint32_t word = 0xFFFFFFFFU;
    for (uint32_t b = 0; b < 4U && (i + b) < len; b++)
    {
      ((uint8_t *)&word)[b] = data[i + b];
    }
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, word_addr, word) != HAL_OK)
    {
      HAL_FLASH_Lock();
      return HAL_ERROR;
    }
    word_addr += 4U;
    i += 4U;
  }

  HAL_FLASH_Lock();
  return HAL_OK;
}
