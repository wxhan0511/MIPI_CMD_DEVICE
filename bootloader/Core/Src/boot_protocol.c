/**
 * @file    boot_protocol.c
 * @brief   Implementation of the upgrade command set.
 */

#include "boot_protocol.h"

#include <string.h>

#include "boot_flash.h"
#include "boot_jump.h"
#include "boot_log.h"

/* Little-endian scalar accessors for the frame payload */
static uint32_t frame_get_u32(const uint8_t *p)
{
  return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t frame_get_u16(const uint8_t *p)
{
  return (uint16_t)(((uint16_t)p[0]) | ((uint16_t)p[1] << 8));
}

static void frame_put_u32(uint8_t *p, uint32_t v)
{
  p[0] = (uint8_t)(v & 0xFFU);
  p[1] = (uint8_t)((v >> 8) & 0xFFU);
  p[2] = (uint8_t)((v >> 16) & 0xFFU);
  p[3] = (uint8_t)((v >> 24) & 0xFFU);
}

static void frame_put_u16(uint8_t *p, uint16_t v)
{
  p[0] = (uint8_t)(v & 0xFFU);
  p[1] = (uint8_t)((v >> 8) & 0xFFU);
}

static void boot_reply(uint8_t *tx, uint8_t cmd, uint8_t status)
{
  memset(tx, 0, BOOT_FRAME_LEN);
  tx[0] = BOOT_FRAME_HEAD;
  tx[1] = cmd;
  tx[2] = status;
}

uint8_t Boot_Protocol_Handle(const uint8_t *rx, uint8_t *tx)
{
  uint8_t cmd = rx[1];

  if (rx[0] != BOOT_FRAME_HEAD)
  {
    boot_reply(tx, cmd, BOOT_STATUS_ERR_HEAD);
    return 0U;
  }

  switch (cmd)
  {
  case BOOT_CMD_SYNC:
  case BOOT_CMD_GET_INFO:
  {
    boot_reply(tx, cmd, BOOT_STATUS_OK);
    tx[3] = BOOT_PROTOCOL_VERSION;
    tx[4] = Boot_AppIsValid();
    frame_put_u32(&tx[5], BOOT_APP_MAX_SIZE);
    return 0U;
  }

  case BOOT_CMD_ERASE:
  {
    /* Payload: u32 size to erase from the slot start; 0 erases the whole slot */
    uint32_t size = frame_get_u32(&rx[3]);
    if (size > BOOT_APP_MAX_SIZE)
    {
      boot_reply(tx, cmd, BOOT_STATUS_ERR_LEN);
      return 0U;
    }
    if (size == 0U)
    {
      size = BOOT_APP_MAX_SIZE;
    }
    Boot_Log_Printf("erasing app area: 0x%08lX + %lu bytes...",
                    (uint32_t)BOOT_APP_ADDRESS, size);
    boot_reply(tx, cmd, (Boot_FlashEraseRange(BOOT_APP_ADDRESS, size) == HAL_OK)
                            ? BOOT_STATUS_OK
                            : BOOT_STATUS_ERR_FLASH);
    return 0U;
  }

  case BOOT_CMD_WRITE:
  {
    /* Payload: u32 offset into the slot (4-byte aligned), u8 data length
       (<= 56), then the data bytes. The final block may be short; the
       bootloader pads the last programming word with 0xFF. */
    uint32_t offset = frame_get_u32(&rx[3]);
    uint16_t len = rx[7];
    if (len == 0U || len > BOOT_WRITE_MAX)
    {
      boot_reply(tx, cmd, BOOT_STATUS_ERR_LEN);
      return 0U;
    }
    if ((offset % 4U) != 0U || (offset + len) > BOOT_APP_MAX_SIZE)
    {
      boot_reply(tx, cmd, BOOT_STATUS_ERR_ADDR);
      return 0U;
    }
    if (Boot_FlashWrite(BOOT_APP_ADDRESS + offset, &rx[8], len) != HAL_OK)
    {
      Boot_Log_Printf("write failed: offset=%lu len=%u", offset, len);
      boot_reply(tx, cmd, BOOT_STATUS_ERR_FLASH);
      return 0U;
    }
    boot_reply(tx, cmd, BOOT_STATUS_OK);
    return 0U;
  }

  case BOOT_CMD_READ:
  {
    /* Payload: u32 offset, u16 length (<= 61); reply data starts at tx[3] */
    uint32_t offset = frame_get_u32(&rx[3]);
    uint16_t len = frame_get_u16(&rx[7]);
    if (len > BOOT_READ_MAX || (offset + len) > BOOT_APP_MAX_SIZE)
    {
      boot_reply(tx, cmd, BOOT_STATUS_ERR_LEN);
      return 0U;
    }
    boot_reply(tx, cmd, BOOT_STATUS_OK);
    memcpy(&tx[3], (const void *)(BOOT_APP_ADDRESS + offset), len);
    return 0U;
  }

  case BOOT_CMD_JUMP_APP:
  {
    boot_reply(tx, cmd, Boot_AppIsValid() ? BOOT_STATUS_OK : BOOT_STATUS_ERR_GENERIC);
    return 1U; /* Caller transmits the reply, then boots the application */
  }

  default:
    boot_reply(tx, cmd, BOOT_STATUS_ERR_GENERIC);
    return 0U;
  }
}
