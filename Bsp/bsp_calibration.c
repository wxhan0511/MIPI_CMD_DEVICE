/**
 * @file       bsp_calibration.c
 * @brief      calibration
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-08-06
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

#include "bsp_calibration.h"
#include "bsp_spi_flash.h"
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "crc.h"

/* Exported variables --------------------------------------------------------*/
calibration_manager_t g_calibration_manager = {0};


/* Internal buffer */
static uint8_t cal_buffer[sizeof(calibration_data_t) + 256]; // Extra space for alignment



/**
 * @brief CRC32 calculation function (using STM32 hardware CRC)
 * @param data Data pointer
 * @param length Data length (bytes)
 * @return CRC32 value
 */
uint32_t calibration_calculate_crc32(uint8_t *data, uint32_t length)
{
    uint32_t crc_value;
    
    // Reset CRC calculator
    __HAL_CRC_DR_RESET(&hcrc);
    // Calculate 4-byte aligned part
    uint32_t word_count = length / 4;
    uint32_t remaining_bytes = length % 4;
    
    if (word_count > 0) {
        crc_value = HAL_CRC_Accumulate(&hcrc, (uint32_t*)data, word_count);
    } else {
        crc_value = hcrc.Instance->DR;
    }
    
    // Handle remaining bytes
    if (remaining_bytes > 0) {
        uint32_t last_word = 0;
        uint8_t *last_bytes = data + (word_count * 4);
        
        for (int i = 0; i < remaining_bytes; i++) {
            last_word |= ((uint32_t)last_bytes[i]) << (i * 8);
        }
        
        // Use HAL function for proper CRC accumulation
        crc_value = HAL_CRC_Accumulate(&hcrc, &last_word, 1);
    }
    
    return crc_value;
}

/**
 * @brief Verify calibration data CRC
 * @param cal_data Calibration data pointer
 * @return HAL_OK: CRC correct, HAL_ERROR: CRC error
 */
HAL_StatusTypeDef calibration_verify_crc(calibration_data_t *cal_data)
{
    uint32_t stored_crc = cal_data->crc32;
    uint32_t calc_crc = calibration_calculate_crc32((uint8_t*)cal_data, 
                                                   sizeof(calibration_data_t) - sizeof(uint32_t));
    
    if (stored_crc == calc_crc) {
        return HAL_OK;
    } else {
        RA_POWEREX_INFO("CRC check failed: stored=0x%08lX, calculated=0x%08lX\r\n", stored_crc, calc_crc);
        g_calibration_manager.last_error = CAL_ERROR_CRC;
        return HAL_ERROR;
    }
}

/**
 * @brief Initialize SPI Flash
 * @return HAL status
 */
HAL_StatusTypeDef calibration_flash_init(void)
{
    // Read Flash ID to verify connection
    uint32_t flash_id = bsp_flash_read_id();
    g_calibration_manager.flash_id = flash_id;
    
    RA_POWEREX_INFO("Detected Flash ID: 0x%06lX\r\n", flash_id);
    
    // W25Q256JVEQ ID should be 0xEF4019
    if ((flash_id & 0xFFFFFF) == 0xEF4019) {
        RA_POWEREX_INFO("W25Q256JVEQ Flash detected successfully\r\n");
        return HAL_OK;
    } else {
        RA_POWEREX_INFO("Flash ID mismatch, expected: 0xEF4019, actual: 0x%06lX\r\n", flash_id);
        g_calibration_manager.last_error = CAL_ERROR_FLASH_READ;
        return HAL_ERROR;
    }
}

/**
 * @brief Set default calibration values
 * @return HAL status
 */
HAL_StatusTypeDef calibration_set_defaults(void)
{
    calibration_data_t *cal = &g_calibration_manager.data;
    
    RA_POWEREX_INFO("Setting default calibration values...\r\n");

    // Clear the structure
    memset(cal, 0, sizeof(calibration_data_t));

    // Set header information
    cal->magic = CALIBRATION_MAGIC;
    cal->version = CALIBRATION_VERSION;
    cal->timestamp = HAL_GetTick();

    cal->ch0_set_v_offset = 115.35f;
    cal->ch0_set_v_gain = 1.0f;
    cal->ch0_read_v_offset = -66854;
    cal->ch0_read_v_gain = 1.0f;
    cal->ch0_read_c_offset = 0.0f;
    cal->ch0_read_c_gain = 1.0f;

    cal->ch1_set_v_offset = 115.35f;
    cal->ch1_set_v_gain = 1.0f;
    cal->ch1_read_v_offset = -66854;
    cal->ch1_read_v_gain = 1.0f;
    cal->ch1_read_c_offset = 0.0f;
    cal->ch1_read_c_gain = 1.0f;

    cal->ch2_set_v_offset = 81.25f;
    cal->ch2_set_v_gain = 1.0f;
    cal->ch2_read_v_offset = 57760;
    cal->ch2_read_v_gain = 1.0f;
    cal->ch2_read_c_offset = 0.0f;
    cal->ch2_read_c_gain = 1.0f;

    cal->ch3_set_v_offset = -203.75f;
    cal->ch3_set_v_gain = 1.0f;
    cal->ch3_read_v_offset = -106917;
    cal->ch3_read_v_gain = 1.0f;
    cal->ch3_read_c_offset = 0.0f;
    cal->ch3_read_c_gain = 1.0f;

    cal->ch4_set_v_offset = -203.75f;
    cal->ch4_set_v_gain = 1.0f;
    cal->ch4_read_v_offset = -106917;
    cal->ch4_read_v_gain = 1.0f;
    cal->ch4_read_c_offset = 761.5f;
    cal->ch4_read_c_gain = 1.0f;

    cal->ch5_set_v_offset = 115.35f;
    cal->ch5_set_v_gain = 1.0f;
    cal->ch5_read_v_offset = -66854;
    cal->ch5_read_v_gain = 1.0f;
    cal->ch5_read_c_offset = -527.0f;
    cal->ch5_read_c_gain = 1.0f;

    cal->ch6_set_v_offset = 81.25f;
    cal->ch6_set_v_gain = 1.0f;
    cal->ch6_read_v_offset = 57760;
    cal->ch6_read_v_gain = 1.0f;
    cal->ch6_read_c_offset = -1455.0f;
    cal->ch6_read_c_gain = 1.0f;

    cal->ch7_set_v_offset = 115.35f;
    cal->ch7_set_v_gain = 1.0f;
    cal->ch7_read_v_offset = -66854;
    cal->ch7_read_v_gain = 1.0f;
    cal->ch7_read_c_offset = -527.0f;
    cal->ch7_read_c_gain = 1.0f;

    cal->da_data.elvdd_set_gain = -3.557;
    cal->da_data.elvdd_set_offset =12850;
    cal->da_data.elvss_set_gain = 3.557;
    cal->da_data.elvss_set_offset = 12850;
    cal->da_data.vcc_set_gain = -1.576;
    cal->da_data.vcc_set_offset = 5100;
    cal->da_data.iovcc_set_gain = -1.576;
    cal->da_data.iovcc_set_offset = 5100;
    // Clear reserved fields
    memset(cal->reserved, 0, sizeof(cal->reserved));

    // Calculate and set CRC , execlude crc32 field itself
    cal->crc32 = calibration_calculate_crc32((uint8_t*)cal,
                                             sizeof(calibration_data_t) - sizeof(uint32_t));

    // Update manager status
    g_calibration_manager.is_loaded = true;
    g_calibration_manager.is_valid = true;
    g_calibration_manager.last_error = CAL_ERROR_NONE;



    
    return HAL_OK;
}

/**
 * @brief Load calibration data from Flash
 * @return HAL status
 */
HAL_StatusTypeDef calibration_load(void)
{
    calibration_data_t *cal = &g_calibration_manager.data;
    
    RA_POWEREX_INFO("Loading calibration data from Flash...\r\n");
    g_calibration_manager.load_attempts++;
	//sf_WaitForWriteEnd();
    // Read data from Flash to buffer
    bsp_flash_read(cal_buffer, CALIBRATION_MAIN_ADDR, sizeof(calibration_data_t));
    
    // Copy to calibration data structure
    memcpy(cal, cal_buffer, sizeof(calibration_data_t));
    
    // Check magic number
    if (cal->magic != CALIBRATION_MAGIC) {
        RA_POWEREX_INFO("Magic number check failed: 0x%08lX (expected: 0x%08lX)\r\n", 
               cal->magic, CALIBRATION_MAGIC);
        calibration_set_defaults();
        calibration_save();
        calibration_load();
        
        g_calibration_manager.last_error = CAL_ERROR_MAGIC;
        return HAL_ERROR;
    }
    
    // Check version compatibility
    if (cal->version > CALIBRATION_MAX_VERSION) {
        RA_POWEREX_INFO("Version not compatible: %lu (max supported: %d)\r\n", 
               cal->version, CALIBRATION_MAX_VERSION);
        g_calibration_manager.last_error = CAL_ERROR_VERSION;
        return HAL_ERROR;
    }
    
    // Verify CRC
    if (calibration_verify_crc(cal) != HAL_OK) {
        RA_POWEREX_INFO("CRC check failed, trying backup data\r\n");
        return calibration_restore_from_backup();
    }
    
    // Load successful
    g_calibration_manager.is_loaded = true;
    g_calibration_manager.is_valid = true;
    g_calibration_manager.last_error = CAL_ERROR_NONE;
    
    RA_POWEREX_INFO("Calibration data loaded successfully (version: %lu, timestamp: %lu, CRC: 0x%08lX)\r\n", 
           cal->version, cal->timestamp, cal->crc32);
    
    return HAL_OK;
}

/**
 * @brief Save calibration data to Flash
 * @return HAL status
 */
HAL_StatusTypeDef calibration_save(void)
{
    calibration_data_t *cal = &g_calibration_manager.data;
    
    RA_POWEREX_INFO("Saving calibration data to Flash...\r\n");
    
    // Update timestamp and CRC
    cal->timestamp = HAL_GetTick();
    cal->crc32 = calibration_calculate_crc32((uint8_t*)cal, 
                                            sizeof(calibration_data_t) - sizeof(uint32_t));//不计算CRC32字段本身
    
    // Copy data to buffer
    memcpy(cal_buffer, cal, sizeof(calibration_data_t));
    
    // Write to Flash
    if (!bsp_flash_write(cal_buffer, CALIBRATION_MAIN_ADDR, sizeof(calibration_data_t))) {
        RA_POWEREX_INFO(" Flash write failed\r\n");
        g_calibration_manager.last_error = CAL_ERROR_FLASH_WRITE;
        return HAL_ERROR;
    }
    
    // Verify written data
    bsp_flash_read(cal_buffer, CALIBRATION_MAIN_ADDR, sizeof(calibration_data_t));
    if (memcmp(cal_buffer, cal, sizeof(calibration_data_t)) != 0) {
        RA_POWEREX_INFO("Flash write verification failed\r\n");
        g_calibration_manager.last_error = CAL_ERROR_FLASH_WRITE;
        return HAL_ERROR;
    }
    
    g_calibration_manager.save_count++;
    RA_POWEREX_INFO("Calibration data saved successfully (CRC: 0x%08lX, save count: %lu)\r\n", 
           cal->crc32, g_calibration_manager.save_count);
    
    // Auto create backup
    if (calibration_backup() != HAL_OK) {
        RA_POWEREX_INFO("Backup creation failed, but main data saved\r\n");
    }
    
    return HAL_OK;
}

/**
 * @brief Create calibration data backup
 * @return HAL status
 */
HAL_StatusTypeDef calibration_backup(void)
{
    calibration_data_t *cal = &g_calibration_manager.data;
    
    RA_POWEREX_INFO("Creating calibration data backup...\r\n");
    
    // Copy data to buffer
    memcpy(cal_buffer, cal, sizeof(calibration_data_t));
    
    // Write to backup 1
    if (!bsp_flash_write(cal_buffer, CALIBRATION_BACKUP1_ADDR, sizeof(calibration_data_t))) {
        RA_POWEREX_INFO("Backup 1 write failed\r\n");
        g_calibration_manager.last_error = CAL_ERROR_BACKUP_FAILED;
        return HAL_ERROR;
    }
    return HAL_OK;
}

/**
 * @brief Restore calibration data from backup
 * @return HAL status
 */
HAL_StatusTypeDef calibration_restore_from_backup(void)
{
    calibration_data_t temp_cal;
    calibration_data_t *cal = &g_calibration_manager.data;
    
    RA_POWEREX_INFO(" Restoring calibration data from backup...\r\n");
    
    // Try backup 1
    bsp_flash_read(cal_buffer, CALIBRATION_BACKUP1_ADDR, sizeof(calibration_data_t));
    memcpy(&temp_cal, cal_buffer, sizeof(calibration_data_t));
    
    if (temp_cal.magic == CALIBRATION_MAGIC && calibration_verify_crc(&temp_cal) == HAL_OK) {
        RA_POWEREX_INFO(" Restored successfully from backup 1\r\n");
        memcpy(cal, &temp_cal, sizeof(calibration_data_t));
        g_calibration_manager.is_loaded = true;
        g_calibration_manager.is_valid = true;
        
        // Restore main data
        return calibration_save();
    }
    
    // All backups failed, use default values
    RA_POWEREX_INFO(" All backup data corrupted, using default values\r\n");
    calibration_set_defaults();
    return calibration_save();
}

/**
 * @brief Factory reset
 * @return HAL status
 */
HAL_StatusTypeDef calibration_factory_reset(void)
{
    RA_POWEREX_INFO("Performing factory reset...\r\n");
    
    // Erase main data area
    bsp_flash_erase_sector(CALIBRATION_MAIN_ADDR);
    
    // Erase backup area
    bsp_flash_erase_sector(CALIBRATION_BACKUP1_ADDR);
    
    // Set default values and save
    calibration_set_defaults();
    return calibration_save();
}

/**
 * @brief Initialize calibration data system
 * @return HAL status
 */
HAL_StatusTypeDef calibration_init(void)
{
    RA_POWEREX_INFO("🚀 Initializing calibration data system...\r\n");
    
    // Initialize manager
    memset(&g_calibration_manager, 0, sizeof(calibration_manager_t));
    
    // Initialize Flash
    if (calibration_flash_init() != HAL_OK) {
        RA_POWEREX_INFO("Flash initialization failed\r\n");
        return HAL_ERROR;
    }
    
    // Try to load data from Flash
    if (calibration_load() == HAL_OK) {
        RA_POWEREX_INFO("Calibration data system initialized successfully\r\n");
        return HAL_OK;
    }
    
    // Load failed, use default values
    RA_POWEREX_INFO("Unable to load valid calibration data, using default values\r\n");
    calibration_set_defaults();
    
    // Save default values to Flash
    if (calibration_save() == HAL_OK) {
        RA_POWEREX_INFO("Default calibration data saved to Flash\r\n");
    }
    
    return HAL_OK;
}

/**
 * @brief Get calibration data pointer
 * @return Calibration data pointer, NULL if invalid
 */
calibration_data_t* calibration_get_data(void)
{
    if (g_calibration_manager.is_valid) {
        return &g_calibration_manager.data;
    }
    return NULL;
}

/**
 * @brief Check if calibration data is valid
 * @return true: valid, false: invalid
 */
bool calibration_is_valid(void)
{
    return g_calibration_manager.is_valid;
}

/**
 * @brief Dump Flash data (for debugging)
 */
void calibration_dump_data(uint32_t addr, uint32_t size)
{
    if (size > sizeof(cal_buffer)) {
        size = sizeof(cal_buffer);
    }
    
    bsp_flash_read(cal_buffer, addr, size);
    
    RA_POWEREX_INFO("\r\n=== Flash Data Dump (Addr: 0x%08lX, Size: %lu) ===\r\n", addr, size);
    
    for (uint32_t i = 0; i < size; i++) {
        if (i % 16 == 0) {
            RA_POWEREX_INFO("\r\n%08lX: ", addr + i);
        }
        RA_POWEREX_INFO("%02X ", cal_buffer[i]);
    }
    RA_POWEREX_INFO("\r\n");
}

/**
 * @brief CRC function test
 */
void calibration_test_crc(void)
{
    RA_POWEREX_INFO("\r\n=== CRC Function Test ===\r\n");
    
    // Test data
    uint8_t test_data[] = "Hello W25Q256 Calibration!";
    uint32_t test_len = strlen((char*)test_data);
    
    uint32_t crc = calibration_calculate_crc32(test_data, test_len);
    RA_POWEREX_INFO("Test data: %s\r\n", test_data);
    RA_POWEREX_INFO("CRC32: 0x%08lX\r\n", crc);
    
    // Test calibration data CRC
    calibration_data_t *cal = calibration_get_data();
    if (cal) {
        uint32_t calc_crc = calibration_calculate_crc32((uint8_t*)cal, 
                                                       sizeof(calibration_data_t) - sizeof(uint32_t));
        RA_POWEREX_INFO("Calibration data stored CRC: 0x%08lX\r\n", cal->crc32);
        RA_POWEREX_INFO("Calibration data calculated CRC: 0x%08lX\r\n", calc_crc);
        RA_POWEREX_INFO("CRC check: %s\r\n", (calc_crc == cal->crc32) ? " Pass" : "Fail");
    }
}


