/**
 * @file    boot_flag.h
 * @brief   Boot-flag API shared by the application and the bootloader.
 *
 * NOTE: The constants below must stay in sync with the application project
 *       (Bsp/boot_flag.h).
 */

#ifndef BOOT_FLAG_H
#define BOOT_FLAG_H

#include <stdint.h>

#define BOOT_MAGIC 0x5AA55AA5U /* Upgrade-request magic written to RTC_BKP_DR0 */

uint8_t Boot_IsUpgradeRequested(void);
void Boot_ClearUpgradeRequest(void);
void Boot_BkUpInit(void);

#endif /* BOOT_FLAG_H */
