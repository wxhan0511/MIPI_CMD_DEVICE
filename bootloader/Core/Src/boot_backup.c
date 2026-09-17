/**
 * @file    boot_backup.c
 * @brief   Backup/restore of the application slot via the external flash.
 */

#include "boot_backup.h"

#include "boot_crc.h"
#include "boot_extflash.h"
#include "boot_flash.h"
#include "boot_log.h"

static uint8_t s_chunk[BOOT_EXT_SECTOR_SIZE];

uint8_t Boot_Backup_Create(void)
{
  Boot_Log_Printf("backup: erase ext flash @0x%08lX (%lu bytes)...",
                  (uint32_t)BOOT_EXT_BACKUP_ADDR, (uint32_t)BOOT_EXT_BACKUP_SIZE);
  if (!Boot_ExtFlash_EraseRange(BOOT_EXT_BACKUP_ADDR, BOOT_EXT_BACKUP_SIZE))
  {
    return 0U;
  }

  uint32_t off = 0;
  while (off < BOOT_EXT_BACKUP_SIZE)
  {
    uint32_t n = BOOT_EXT_BACKUP_SIZE - off;
    if (n > sizeof(s_chunk))
    {
      n = sizeof(s_chunk);
    }

    const uint8_t *src = (const uint8_t *)(BOOT_APP_ADDRESS + off);
    for (uint32_t i = 0; i < n; i++)
    {
      s_chunk[i] = src[i];
    }

    if (!Boot_ExtFlash_Write(s_chunk, BOOT_EXT_BACKUP_ADDR + off, n))
    {
      return 0U;
    }
    off += n;
  }

  /* Verify the written backup's metadata block against the internal slot. */
  uint8_t expect[sizeof(boot_meta_t)];
  uint8_t actual[sizeof(boot_meta_t)];
  const uint8_t *meta_src = (const uint8_t *)BOOT_META_ADDRESS;
  for (uint32_t i = 0; i < sizeof(boot_meta_t); i++)
  {
    expect[i] = meta_src[i];
  }
  uint32_t meta_off = BOOT_META_ADDRESS - BOOT_APP_ADDRESS;
  if (!Boot_ExtFlash_Read(actual, BOOT_EXT_BACKUP_ADDR + meta_off,
                          sizeof(boot_meta_t)))
  {
    Boot_Log_Printf("backup: read-back verify failed");
    return 0U;
  }
  for (uint32_t i = 0; i < sizeof(boot_meta_t); i++)
  {
    if (actual[i] != expect[i])
    {
      Boot_Log_Printf("backup: verify mismatch @%u (expect 0x%02X got 0x%02X)",
                      (unsigned)i, expect[i], actual[i]);
      return 0U;
    }
  }
  Boot_Log_Printf("backup: done");
  return 1U;
}

uint8_t Boot_Backup_Restore(void)
{
  boot_meta_t meta;
  if (!Boot_Backup_GetInfo(&meta))
  {
    Boot_Log_Printf("restore: no valid backup");
    return 1U; /* reason: no backup */
  }

  Boot_Log_Printf("restore: erase internal app (%lu bytes)...",
                  (uint32_t)BOOT_APP_MAX_SIZE);
  if (Boot_FlashEraseRange(BOOT_APP_ADDRESS, BOOT_APP_MAX_SIZE) != HAL_OK)
  {
    Boot_Log_Printf("restore: internal erase failed");
    return 2U; /* reason: erase failed */
  }

  uint32_t off = 0;
  while (off < BOOT_EXT_BACKUP_SIZE)
  {
    uint32_t n = BOOT_EXT_BACKUP_SIZE - off;
    if (n > sizeof(s_chunk))
    {
      n = sizeof(s_chunk);
    }

    if (!Boot_ExtFlash_Read(s_chunk, BOOT_EXT_BACKUP_ADDR + off, n))
    {
      Boot_Log_Printf("restore: ext read failed @0x%lX", (uint32_t)off);
      return 3U; /* reason: ext read failed */
    }
    if (Boot_FlashWrite(BOOT_APP_ADDRESS + off, s_chunk, n) != HAL_OK)
    {
      Boot_Log_Printf("restore: internal write failed @0x%lX", (uint32_t)off);
      return 4U; /* reason: internal write failed */
    }
    off += n;
  }

  if (!Boot_MetaVerify())
  {
    Boot_Log_Printf("restore: crc verify failed");
    return 5U; /* reason: crc mismatch */
  }
  Boot_Log_Printf("restore: done");
  return 0U; /* OK */
}

uint8_t Boot_Backup_GetInfo(boot_meta_t *meta)
{
  uint32_t meta_off = BOOT_META_ADDRESS - BOOT_APP_ADDRESS;
  uint8_t raw[sizeof(boot_meta_t)];

  if (!Boot_ExtFlash_Read(raw, BOOT_EXT_BACKUP_ADDR + meta_off, sizeof(boot_meta_t)))
  {
    return 0U;
  }

  uint8_t erased = 1U;
  for (uint32_t i = 0; i < sizeof(boot_meta_t); i++)
  {
    ((uint8_t *)meta)[i] = raw[i];
    if (raw[i] != 0xFFU)
    {
      erased = 0U;
    }
  }
  return erased ? 0U : 1U;
}
