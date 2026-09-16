/**
 * @file    boot_flag.h
 * @brief   Boot-flag API shared by the application and the bootloader.
 *
 * The upgrade request is stored in an RTC backup register (BKP0R), which
 * survives both a system reset and a power cycle (VBAT domain), so no Flash
 * sector has to be sacrificed for the flag.
 *
 * NOTE: The constants below must stay in sync with the bootloader project
 *       (bootloader/boot_flag.h).
 */

#ifndef BOOT_FLAG_H
#define BOOT_FLAG_H

#include <stdint.h>

#define BOOT_MAGIC 0x5AA55AA5U /* Upgrade-request magic written to RTC_BKP_DR0 */

/**
 * @brief  Request the next boot to enter the bootloader upgrade mode.
 *         Writes the magic value to RTC_BKP_DR0 and performs a system reset.
 *         This function never returns.
 */
void Boot_RequestUpgrade(void);

/**
 * @brief  Check whether an upgrade was requested before the last reset.
 * @retval 1 Upgrade requested, 0 Normal boot
 */
uint8_t Boot_IsUpgradeRequested(void);

/**
 * @brief  Clear the upgrade request (call once the bootloader has taken over).
 */
void Boot_ClearUpgradeRequest(void);

/**
 * @brief  Make sure the backup domain clock (LSI -> RTC) is running so the
 *         backup registers are accessible. Called lazily by the functions
 *         above; no Calendar configuration is performed.
 */
void Boot_BkUpInit(void);

#endif /* BOOT_FLAG_H */
