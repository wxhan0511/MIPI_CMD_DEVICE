/**
 * @file    stm32f4xx_it.c
 * @brief   Interrupt service routines for the bootloader (SysTick only).
 */

#include "main.h"
#include "stm32f4xx_it.h"
#include "boot_spi.h"

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

void SPI2_IRQHandler(void)
{
  Boot_Spi_IRQHandler();
}
