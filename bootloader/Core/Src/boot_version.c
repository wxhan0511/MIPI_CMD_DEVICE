/**
 * @file    boot_version.c
 * @brief   Metadata block access and boot-time CRC verification.
 */

#include "boot_version.h"

#include "boot_crc.h"

void Boot_MetaRead(boot_meta_t *meta)
{
  const uint8_t *src = (const uint8_t *)BOOT_META_ADDRESS;
  for (uint32_t i = 0; i < sizeof(boot_meta_t); i++)
  {
    ((uint8_t *)meta)[i] = src[i];
  }
}

uint32_t Boot_MetaComputeCRC(void)
{
  /* Internal flash is memory-mapped, so CRC the app slot directly. */
  return Boot_CRC32((const uint8_t *)BOOT_APP_ADDRESS, BOOT_APP_DATA_LEN);
}

uint8_t Boot_MetaVerify(void)
{
  boot_meta_t meta;
  Boot_MetaRead(&meta);
  return (Boot_MetaComputeCRC() == meta.crc32) ? 1U : 0U;
}
