/**
 * @file    boot_protocol.h
 * @brief   Upgrade command set exchanged over the SPI2 transport.
 */

#ifndef BOOT_PROTOCOL_H
#define BOOT_PROTOCOL_H

#include "boot_spi.h"

/* Command bytes. SYNC deliberately shares the value the application uses for
 * CMD_ENTER_BOOT so the host hand-off is seamless. */
#define BOOT_CMD_SYNC 0x27U    /* Handshake: reply with protocol info            */
#define BOOT_CMD_ERASE 0x28U   /* Erase the application area                     */
#define BOOT_CMD_WRITE 0x29U   /* Program one block at the given offset          */
#define BOOT_CMD_READ 0x2AU    /* Read back a block for host-side verification   */
#define BOOT_CMD_JUMP_APP 0x30U/* Validate the application and boot it           */
#define BOOT_CMD_GET_INFO 0x31U/* Report slot layout and application validity    */
#define BOOT_CMD_SET_FRAME 0x32U/* Negotiate a larger upgrade frame             */
#define BOOT_CMD_BACKUP 0x33U   /* Copy the app slot to the external backup     */
#define BOOT_CMD_RESTORE 0x34U  /* Restore the app slot from the backup         */
#define BOOT_CMD_GET_BACKUP_INFO 0x35U/* Report backup magic/version/crc        */
#define BOOT_CMD_VERIFY 0x36U   /* Verify the app slot CRC against the meta    */

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
#define BOOT_STATUS_ERR_CRC 0x06U    /* Application CRC mismatch          */

#define BOOT_PROTOCOL_VERSION 1U

/**
 * @brief  Parse and execute one command frame.
 * @param  rx  Received command frame.
 * @param  tx  Response frame to transmit (always fully rewritten).
 * @param  frame_len Current frame length.
 * @param  next_frame_len Frame length to use after the response is sent.
 * @retval 1 The caller must boot the application after transmitting tx,
 *         0 Stay in upgrade mode.
 */
uint8_t Boot_Protocol_Handle(const uint8_t *rx, uint8_t *tx,
                             uint16_t frame_len, uint16_t *next_frame_len);

#endif /* BOOT_PROTOCOL_H */
