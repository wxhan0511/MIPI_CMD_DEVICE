/**
 * @file    boot_crc.h
 * @brief   Software CRC-32/ISO-HDLC (matches Python zlib.crc32 / binascii.crc32).
 *
 * The STM32F4 hardware CRC peripheral implements CRC-32/MPEG-2 (non-reflected,
 * no final XOR) which does NOT match the CRC the host computes with zlib, so we
 * use this small table-driven software implementation instead.
 */

#ifndef BOOT_CRC_H
#define BOOT_CRC_H

#include <stdint.h>

uint32_t Boot_CRC32(const uint8_t *data, uint32_t len);

void Boot_CRC32_Init(uint32_t *ctx);
void Boot_CRC32_Update(uint32_t *ctx, const uint8_t *data, uint32_t len);
uint32_t Boot_CRC32_Final(uint32_t ctx);

#endif /* BOOT_CRC_H */
