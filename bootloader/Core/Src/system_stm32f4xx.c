/**
 * @file    system_stm32f4xx.c
 * @brief   Minimal CMSIS system support for the bootloader.
 *
 * SystemInit() only enables the FPU (so the hard-float application can be
 * jumped to) and relocates the vector table to the flash base. The clock
 * tree is configured later in main() via HAL_RCC_ClockConfig().
 */

#include "stm32f4xx_hal.h"

uint32_t SystemCoreClock = 16000000U; /* Updated by HAL_RCC_ClockConfig() */
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};

void SystemInit(void)
{
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
  /* Set CP10 and CP11 to full access */
  SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
#endif

  /* The bootloader image lives at the flash base */
  SCB->VTOR = FLASH_BASE;
}

void SystemCoreClockUpdate(void)
{
  SystemCoreClock = HAL_RCC_GetHCLKFreq();
}
