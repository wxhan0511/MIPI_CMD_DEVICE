/**
 * @file    boot_crc.c
 * @brief   Software CRC-32/ISO-HDLC implementation (see boot_crc.h).
 */

#include "boot_crc.h"

static uint32_t s_crc_table[256];
static uint8_t s_crc_table_ready = 0U;

static void boot_crc_build_table(void)
{
  for (uint32_t i = 0; i < 256U; i++)
  {
    uint32_t c = i;
    for (uint32_t k = 0; k < 8U; k++)
    {
      c = (c & 1U) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
    }
    s_crc_table[i] = c;
  }
  s_crc_table_ready = 1U;
}

void Boot_CRC32_Init(uint32_t *ctx)
{
  if (!s_crc_table_ready)
  {
    boot_crc_build_table();
  }
  *ctx = 0xFFFFFFFFU;
}

void Boot_CRC32_Update(uint32_t *ctx, const uint8_t *data, uint32_t len)
{
  uint32_t crc = *ctx;
  for (uint32_t i = 0; i < len; i++)
  {
    crc = s_crc_table[(crc ^ data[i]) & 0xFFU] ^ (crc >> 8);
  }
  *ctx = crc;
}

uint32_t Boot_CRC32_Final(uint32_t ctx)
{
  return ctx ^ 0xFFFFFFFFU;
}

uint32_t Boot_CRC32(const uint8_t *data, uint32_t len)
{
  uint32_t ctx;
  Boot_CRC32_Init(&ctx);
  Boot_CRC32_Update(&ctx, data, len);
  return Boot_CRC32_Final(ctx);
}
