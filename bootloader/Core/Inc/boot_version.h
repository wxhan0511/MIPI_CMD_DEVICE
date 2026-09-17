/**
 * @file    boot_version.h
 * @brief   16-byte firmware metadata block and its boot-time CRC verification.
 *
 * The application binary ends with a 16-byte metadata block linked at
 * BOOT_META_ADDRESS (the last 16 bytes of the 512 KB app slot). The CRC stored
 * inside it covers the app slot up to (but not including) the block itself, so
 * the bootloader can detect a corrupt application and refuse to jump into it.
 */

#ifndef BOOT_VERSION_H
#define BOOT_VERSION_H

#include <stdint.h>

#include "main.h"

/* Fixed address of the 16-byte metadata block (tail of the app slot). */
#define BOOT_META_ADDRESS 0x0808FFF0U

/* Length of the CRC'ed app region: from the app slot start up to the metadata
   block (512 KB - 16 bytes). */
#define BOOT_APP_DATA_LEN (BOOT_META_ADDRESS - BOOT_APP_ADDRESS)

typedef struct
{
  uint8_t fw_version[4]; /* major.minor.patch.build */
  uint8_t hw_version[4]; /* major.minor.patch.build */
  uint32_t crc32;        /* CRC-32/ISO-HDLC over BOOT_APP_DATA_LEN bytes */
  uint32_t reserved;     /* must be 0 */
} boot_meta_t;

void Boot_MetaRead(boot_meta_t *meta);
uint32_t Boot_MetaComputeCRC(void);
uint8_t Boot_MetaVerify(void);

#endif /* BOOT_VERSION_H */
