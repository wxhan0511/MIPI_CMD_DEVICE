/**
 * @file    boot_log.h
 * @brief   Polling UART log output for the bootloader (USART3, 115200 8N1,
 *          PD8=TX / PD9=RX -- the same console the application prints to).
 */

#ifndef BOOT_LOG_H
#define BOOT_LOG_H

#include "main.h"

/**
 * @brief  Initialize USART3 for log output. Must be called after the clock
 *         tree is configured (the baud rate depends on HCLK).
 */
void Boot_Log_Init(void);

/**
 * @brief  Formatted log line. A CRLF is appended automatically.
 * @note   Integer/string formats only (%u/%x/%s/%08lX...); avoid %f -- the
 *         bootloader does not link the float printf backend.
 */
void Boot_Log_Printf(const char *fmt, ...);

#endif /* BOOT_LOG_H */
