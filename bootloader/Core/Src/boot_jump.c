/**
 * @file    boot_jump.c
 * @brief   Application sanity check and safe hand-off to the application.
 */

#include "boot_jump.h"

/* Validity bounds: the application SP must live in the 128 KB SRAM and the
   application reset vector must point inside the application flash slot. */
#define RAM_START 0x20000000U
#define RAM_END 0x20020000U

typedef void (*pFunction)(void);

uint8_t Boot_AppIsValid(void)
{
  uint32_t app_stack = *(__IO uint32_t *)BOOT_APP_ADDRESS;
  uint32_t app_reset = *(__IO uint32_t *)(BOOT_APP_ADDRESS + 4U);

  /* The initial MSP points to the first free byte above the stack, i.e. the
     top of RAM: an application linked with its stack at the very end of RAM
     legitimately has app_stack == RAM_END, hence "<=" here. */
  if ((app_stack > RAM_START) && (app_stack <= RAM_END) &&
      (app_reset >= BOOT_APP_ADDRESS) && (app_reset < BOOT_APP_END) &&
      ((app_reset & 1U) != 0U)) /* Thumb bit must be set */
  {
    return 1U;
  }
  return 0U;
}

void Boot_JumpToApp(void)
{
  uint32_t app_stack = *(__IO uint32_t *)BOOT_APP_ADDRESS;
  uint32_t app_reset = *(__IO uint32_t *)(BOOT_APP_ADDRESS + 4U);
  pFunction jump = (pFunction)app_reset;

  /* Stop the SysTick timer before leaving the bootloader context */
  SysTick->CTRL = 0U;
  SysTick->LOAD = 0U;
  SysTick->VAL = 0U;

  /* Disable all interrupts and clear any pending ones */
  __disable_irq();
  for (uint32_t i = 0; i < 8U; i++)
  {
    NVIC->ICER[i] = 0xFFFFFFFFU;
    NVIC->ICPR[i] = 0xFFFFFFFFU;
  }

  /* Switch the vector table to the application and start it */
  SCB->VTOR = BOOT_APP_ADDRESS;
  __set_MSP(app_stack);
  __enable_irq(); /* The application expects interrupts enabled after its own init */

  __DSB();
  __ISB();
  jump();

  while (1)
  {
    /* Never reached */
  }
}
