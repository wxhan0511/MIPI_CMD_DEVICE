/**
 * @file    main.h
 * @brief   Bootloader-wide definitions.
 */

#ifndef BOOT_MAIN_H
#define BOOT_MAIN_H

#include "stm32f4xx_hal.h"

/* ---------------- Flash partition (must match the application project) ---- */
#define BOOT_APP_ADDRESS 0x08010000U  /* Application slot start (Sector 4)          */
/* Application slot: 512 KB (0x08010000 .. 0x08090000). The binary ends with a
   16-byte metadata block at 0x0808FFF0 (see boot_version.h); sectors 0..3 (the
   bootloader itself) stay protected. */
#define BOOT_APP_MAX_SIZE 0x80000U    /* 512 KB */
#define BOOT_APP_END (BOOT_APP_ADDRESS + BOOT_APP_MAX_SIZE)

/* ---------------- Host notification -------------------------------------- */
/* The bootloader raises M_INT (PC4) once it is ready to receive upgrade
 * frames. Pin names mirror the application project (Core/Inc/pin_defs.h). */
#define BOOT_M_INT_Pin GPIO_PIN_4
#define BOOT_M_INT_GPIO_Port GPIOC

void Error_Handler(void);

#endif /* BOOT_MAIN_H */
