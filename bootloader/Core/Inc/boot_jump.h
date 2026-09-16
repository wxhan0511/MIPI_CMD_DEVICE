/**
 * @file    boot_jump.h
 * @brief   Application validation and hand-off.
 */

#ifndef BOOT_JUMP_H
#define BOOT_JUMP_H

#include "main.h"

uint8_t Boot_AppIsValid(void);
void Boot_JumpToApp(void) __attribute__((noreturn));

#endif /* BOOT_JUMP_H */
