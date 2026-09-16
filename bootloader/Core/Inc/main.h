/**
 * @file    main.h
 * @brief   Bootloader-wide definitions.
 */

#ifndef BOOT_MAIN_H
#define BOOT_MAIN_H

#include "stm32f4xx_hal.h"

/* ---------------- Flash partition (must match the application project) ---- */
#define BOOT_APP_ADDRESS 0x08010000U  /* Application slot start (Sector 4)          */
/* Flashable window from the app slot up to the physical end of flash: the
   application bin carries its 88-byte version block as the tail (linked at
   0x080E0000, Sector 11), so erase/program bounds extend past the version
   region to flash end. Sectors 0..3 (the bootloader itself) stay protected. */
#define BOOT_APP_MAX_SIZE 0xF0000U    /* 0x08100000 - 0x08010000 = 960 KB */
#define BOOT_APP_END (BOOT_APP_ADDRESS + BOOT_APP_MAX_SIZE)

/* ---------------- Host notification -------------------------------------- */
/* The bootloader raises M_INT (PC4) once it is ready to receive upgrade
 * frames. Pin names mirror the application project (Core/Inc/pin_defs.h). */
#define BOOT_M_INT_Pin GPIO_PIN_4
#define BOOT_M_INT_GPIO_Port GPIOC

void Error_Handler(void);

#endif /* BOOT_MAIN_H */
