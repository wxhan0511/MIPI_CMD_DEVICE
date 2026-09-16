/**
 * @file    boot_flag.c
 * @brief   Boot-flag implementation based on the RTC backup registers
 *          (identical logic to the application's Bsp/boot_flag.c).
 */

#include "boot_flag.h"

#include "stm32f4xx_hal.h"

void Boot_BkUpInit(void)
{
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
