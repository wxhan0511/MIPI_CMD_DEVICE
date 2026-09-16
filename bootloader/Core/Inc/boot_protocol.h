/**
 * @file    boot_protocol.h
 * @brief   Upgrade command set exchanged over the SPI2 transport.
 */

#ifndef BOOT_PROTOCOL_H
#define BOOT_PROTOCOL_H

#include "boot_spi.h"

/* Command bytes (fixed 64-byte frames). SYNC deliberately shares the value
 * the application uses for CMD_ENTER_BOOT so the host hand-off is seamless. */
#define BOOT_CMD_SYNC 0x27U    /* Handshake: reply with protocol info            */
#define BOOT_CMD_ERASE 0x28U   /* Erase the application area                     */
#define BOOT_CMD_WRITE 0x29U   /* Program one block at the given offset          */
#define BOOT_CMD_READ 0x2AU    /* Read back a block for host-side verification   */
#define BOOT_CMD_JUMP_APP 0x30U/* Validate the application and boot it           */
#define BOOT_CMD_GET_INFO 0x31U/* Report slot layout and application validity    */

/* Frame layout (both directions): [0]=0xA0 head, [1]=command, [2]=status,
   [3..]=command-specific payload, unused bytes 0x00. */
#define BOOT_FRAME_HEAD 0xA0U

/* Status codes returned in byte [2] */
#define BOOT_STATUS_OK 0x00U
#define BOOT_STATUS_ERR_GENERIC 0x01U
#define BOOT_STATUS_ERR_ADDR 0x02U   /* Bad offset/alignment/out of range */
#define BOOT_STATUS_ERR_FLASH 0x03U  /* HAL erase/program failure         */
#define BOOT_STATUS_ERR_LEN 0x04U    /* Invalid length                    */
#define BOOT_STATUS_ERR_HEAD 0x05U   /* Unknown frame head                */

#define BOOT_PROTOCOL_VERSION 1U

/* Maximum payload sizes inside a 64-byte frame ([3..63] = 61 bytes) */
#define BOOT_WRITE_MAX (BOOT_FRAME_LEN - 8U) /* offset(4) + len(1) + data -> 56 bytes */
#define BOOT_READ_MAX (BOOT_FRAME_LEN - 3U)  /* data -> 61 bytes            */

/**
 * @brief  Parse and execute one command frame.
 * @param  rx  Received 64-byte command frame.
 * @param  tx  Response frame to transmit (always fully rewritten).
 * @retval 1 The caller must boot the application after transmitting tx,
 *         0 Stay in upgrade mode.
 */
uint8_t Boot_Protocol_Handle(const uint8_t *rx, uint8_t *tx);

#endif /* BOOT_PROTOCOL_H */
