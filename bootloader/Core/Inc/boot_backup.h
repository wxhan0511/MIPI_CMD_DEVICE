/**
 * @file    boot_backup.h
 * @brief   Firmware backup/restore between the internal app slot and the
 *          external W25Q256 backup region.
 */

#ifndef BOOT_BACKUP_H
#define BOOT_BACKUP_H

#include "boot_version.h"

uint8_t Boot_Backup_Create(void);  /* 1 = ok, 0 = fail */
uint8_t Boot_Backup_Restore(void); /* 0 = ok, 1 = no backup, 2 = erase fail,
                                      3 = ext read fail, 4 = write fail, 5 = crc fail */
uint8_t Boot_Backup_GetInfo(boot_meta_t *meta);

#endif /* BOOT_BACKUP_H */
