/**
 * @file    boot_protocol.c
 * @brief   Implementation of the upgrade command set.
 */

#include "boot_protocol.h"

#include <string.h>

#include "boot_backup.h"
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

static void boot_reply(uint8_t *tx, uint16_t frame_len, uint8_t cmd,
                       uint8_t status)
{
  memset(tx, 0, frame_len);
  tx[0] = BOOT_FRAME_HEAD;
  tx[1] = cmd;
  tx[2] = status;
}

uint8_t Boot_Protocol_Handle(const uint8_t *rx, uint8_t *tx,
                             uint16_t frame_len, uint16_t *next_frame_len)
{
  if (frame_len < 9U || frame_len > BOOT_MAX_FRAME_LEN)
    return 0U;

  *next_frame_len = frame_len;
  uint8_t cmd = rx[1];

  if (rx[0] != BOOT_FRAME_HEAD)
  {
    boot_reply(tx, frame_len, cmd, BOOT_STATUS_ERR_HEAD);
    return 0U;
  }

  switch (cmd)
  {
  case BOOT_CMD_SYNC:
  case BOOT_CMD_GET_INFO:
  {
    boot_reply(tx, frame_len, cmd, BOOT_STATUS_OK);
    tx[3] = BOOT_PROTOCOL_VERSION;
    tx[4] = Boot_AppIsValid();
    frame_put_u32(&tx[5], BOOT_APP_MAX_SIZE);
    if (frame_len >= 11U)
      frame_put_u16(&tx[9], BOOT_MAX_FRAME_LEN);
    return 0U;
  }

  case BOOT_CMD_SET_FRAME:
  {
    uint16_t requested = frame_get_u16(&rx[3]);
    if (requested != BOOT_CONTROL_FRAME_LEN && requested != BOOT_FAST_FRAME_LEN)
    {
      boot_reply(tx, frame_len, cmd, BOOT_STATUS_ERR_LEN);
      return 0U;
    }
    boot_reply(tx, frame_len, cmd, BOOT_STATUS_OK);
    *next_frame_len = requested;
    return 0U;
  }

  case BOOT_CMD_ERASE:
  {
    /* Payload: u32 size to erase from the slot start; 0 erases the whole slot */
    uint32_t size = frame_get_u32(&rx[3]);
    if (size > BOOT_APP_MAX_SIZE)
    {
      boot_reply(tx, frame_len, cmd, BOOT_STATUS_ERR_LEN);
      return 0U;
    }
    if (size == 0U)
    {
      size = BOOT_APP_MAX_SIZE;
    }
    Boot_Log_Printf("erasing app area: 0x%08lX + %lu bytes...",
                    (uint32_t)BOOT_APP_ADDRESS, size);
    boot_reply(tx, frame_len, cmd,
               (Boot_FlashEraseRange(BOOT_APP_ADDRESS, size) == HAL_OK)
                   ? BOOT_STATUS_OK
                   : BOOT_STATUS_ERR_FLASH);
    return 0U;
  }

  case BOOT_CMD_WRITE:
  {
    /* Legacy 64-byte frames use u8 length at [7] and data at [8]. Negotiated
       large frames use u16 length at [7..8] and data at [9]. */
    uint32_t offset = frame_get_u32(&rx[3]);
    uint16_t len;
    const uint8_t *data;
    uint16_t max_len;
    if (frame_len == BOOT_CONTROL_FRAME_LEN)
    {
      len = rx[7];
      data = &rx[8];
      max_len = frame_len - 8U;
    }
    else
    {
      len = frame_get_u16(&rx[7]);
      data = &rx[9];
      max_len = frame_len - 9U;
    }
    if (len == 0U || len > max_len)
    {
      boot_reply(tx, frame_len, cmd, BOOT_STATUS_ERR_LEN);
      return 0U;
    }
    if ((offset % 4U) != 0U || (offset + len) > BOOT_APP_MAX_SIZE)
    {
      boot_reply(tx, frame_len, cmd, BOOT_STATUS_ERR_ADDR);
      return 0U;
    }
    if (Boot_FlashWrite(BOOT_APP_ADDRESS + offset, data, len) != HAL_OK)
    {
      Boot_Log_Printf("write failed: offset=%lu len=%u", offset, len);
      boot_reply(tx, frame_len, cmd, BOOT_STATUS_ERR_FLASH);
      return 0U;
    }
    boot_reply(tx, frame_len, cmd, BOOT_STATUS_OK);
    return 0U;
  }

  case BOOT_CMD_READ:
  {
    /* Payload: u32 offset, u16 length; reply data starts at tx[3]. */
    uint32_t offset = frame_get_u32(&rx[3]);
    uint16_t len = frame_get_u16(&rx[7]);
    if (len > (frame_len - 3U) || (offset + len) > BOOT_APP_MAX_SIZE)
    {
      boot_reply(tx, frame_len, cmd, BOOT_STATUS_ERR_LEN);
      return 0U;
    }
    boot_reply(tx, frame_len, cmd, BOOT_STATUS_OK);
    memcpy(&tx[3], (const void *)(BOOT_APP_ADDRESS + offset), len);
    return 0U;
  }

  case BOOT_CMD_JUMP_APP:
  {
    uint8_t status;
    if (!Boot_AppIsValid())
    {
      status = BOOT_STATUS_ERR_GENERIC;
    }
    else if (!Boot_MetaVerify())
    {
      status = BOOT_STATUS_ERR_CRC;
    }
    else
    {
      status = BOOT_STATUS_OK;
    }
    boot_reply(tx, frame_len, cmd, status);
    return (status == BOOT_STATUS_OK) ? 1U : 0U;
  }

  case BOOT_CMD_BACKUP:
  {
    boot_reply(tx, frame_len, cmd,
               Boot_Backup_Create() ? BOOT_STATUS_OK : BOOT_STATUS_ERR_FLASH);
    return 0U;
  }

  case BOOT_CMD_RESTORE:
  {
    uint8_t reason = Boot_Backup_Restore();
    boot_reply(tx, frame_len, cmd,
               reason == 0U ? BOOT_STATUS_OK : BOOT_STATUS_ERR_FLASH);
    tx[3] = reason; /* diagnostic: 0=ok,1=no backup,2=erase,3=read,4=write,5=crc */
    return reason == 0U ? 1U : 0U; /* jump into the restored app on success */
  }

  case BOOT_CMD_GET_BACKUP_INFO:
  {
    boot_meta_t meta;
    uint8_t has = Boot_Backup_GetInfo(&meta);
    boot_reply(tx, frame_len, cmd,
               has ? BOOT_STATUS_OK : BOOT_STATUS_ERR_GENERIC);
    if (has)
    {
      memcpy(&tx[3], meta.fw_version, 4);
      memcpy(&tx[7], meta.hw_version, 4);
      frame_put_u32(&tx[11], meta.crc32);
    }
    return 0U;
  }

  case BOOT_CMD_VERIFY:
  {
    boot_meta_t meta;
    Boot_MetaRead(&meta);
    uint32_t computed = Boot_MetaComputeCRC();
    uint8_t ok = (computed == meta.crc32) ? 1U : 0U;
    boot_reply(tx, frame_len, cmd, ok ? BOOT_STATUS_OK : BOOT_STATUS_ERR_CRC);
    frame_put_u32(&tx[3], computed); /* diagnostic: actual flash CRC  */
    frame_put_u32(&tx[7], meta.crc32); /* diagnostic: stored CRC       */
    return 0U;
  }

  default:
    boot_reply(tx, frame_len, cmd, BOOT_STATUS_ERR_GENERIC);
    return 0U;
  }
}
