/**
 * @file    boot_flag.c
 * @brief   Boot-flag implementation based on the RTC backup registers.
 *
 * The RTC is only used as a provider for the backup registers (VBAT domain);
 * no calendar/timekeeping is configured. The code works at register level so
 * neither the HAL RTC module nor a CubeMX RTC instance is required.
 */

#include "boot_flag.h"

#include "stm32f4xx_hal.h"

/* RTC backup register used to carry the upgrade request across a reset (BKP0R) */

void Boot_BkUpInit(void)
{
  /* Enable the PWR interface clock and unlock the backup domain */
  __HAL_RCC_PWR_CLK_ENABLE();
  SET_BIT(PWR->CR, PWR_CR_DBP);
}

uint8_t Boot_IsUpgradeRequested(void)
{
  Boot_BkUpInit();
  return (RTC->BKP0R == BOOT_MAGIC) ? 1U : 0U;
}

void Boot_ClearUpgradeRequest(void)
{
  Boot_BkUpInit();
  RTC->BKP0R = 0U;
}

void Boot_RequestUpgrade(void)
{
  Boot_BkUpInit();
  RTC->BKP0R = BOOT_MAGIC;

  /* Flush any pending I/O, then reset. The bootloader takes over on the next
     boot and consumes the flag. */
  __DSB();
  __ISB();
  NVIC_SystemReset();

  while (1)
  {
    /* Never reached */
  }
}
