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
#include "math.h"
//Notice:mcp4728 2000和0值附近不要用,极限值附近计算校准参数有偏差
const float vi[20][2]=
{
    {2000, 1500}, {1500, 1300}, {1500, 1200}, {1500, 2000},
    {1500, 3000}, {1500, 3000}, {1500, 3000}, {1500, 3000},
    {1500, 1550}, {1500, 1400}, {1500, 1100}, {1500, 1100},
    {1500, 3000}, {1500, 3000}, {1500, 3000}, {1500, 3000},
    {1500, 3000}, {1500, 3000}, {2500, 2400}, {3000, 2000}
};
const float vo[20][2]=
{
    {1505.5,4946}, {2623,4003}, {2792.5,5312}, {-3219,-1235.7},
    {1500, 3000}, {1500, 3000}, {1500, 3000}, {1500, 3000},
    {4894, 4546}, {2702, 3535.7}, {2637.6, 6000.6}, {-3115, -5796},
    {1500, 3000}, {1500, 3000}, {1500, 3000}, {1500, 3000},
    {1500, 3000}, {1500, 3000},{8526, 9198}, {-18441, -14282}
};
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
        MIPI_CMD_INFO("CRC check failed: stored=0x%08lX, calculated=0x%08lX\r\n", stored_crc, calc_crc);
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
    
    MIPI_CMD_INFO("Detected Flash ID: 0x%06lX\r\n", flash_id);
    
    // W25Q256JVEQ ID should be 0xEF4019
    if ((flash_id & 0xFFFFFF) == 0xEF4019) {
        MIPI_CMD_INFO("W25Q256JVEQ Flash detected successfully\r\n");
        return HAL_OK;
    } else {
        MIPI_CMD_INFO("Flash ID mismatch, expected: 0xEF4019, actual: 0x%06lX\r\n", flash_id);
        g_calibration_manager.last_error = CAL_ERROR_FLASH_READ;
        return HAL_ERROR;
    }
}
HAL_StatusTypeDef calu_calibration_data()
{
    da_calibration_data_t *da = &g_calibration_manager.data.da_data;

    float *gain_ptr[20] = {
        &da->vcc_set_gain, &da->iovcc_set_gain, &da->vsp_set_gain, &da->vsn_set_gain,
        &da->vcc_ref_gain, &da->iovcc_ref_gain, &da->vsp_ref_gain, &da->vsn_ref_gain,
        &da->avdd_set_gain, &da->vdd_set_gain, &da->elvdd_set_gain, &da->elvss_set_gain,
        &da->avdd_ref_gain, &da->vdd_ref_gain, &da->elvdd_ref_gain, &da->elvss_ref_gain,
        &da->v_level_shift_gain, &da->ref_freq_gain,&da->vadj_p_gain, &da->vadj_n_gain
    };

    float *offset_ptr[20] = {
        &da->vcc_set_offset, &da->iovcc_set_offset, &da->vsp_set_offset, &da->vsn_set_offset,
        &da->vcc_ref_offset, &da->iovcc_ref_offset, &da->vsp_ref_offset, &da->vsn_ref_offset,
        &da->avdd_set_offset, &da->vdd_set_offset, &da->elvdd_set_offset, &da->elvss_set_offset,
        &da->avdd_ref_offset, &da->vdd_ref_offset, &da->elvdd_ref_offset, &da->elvss_ref_offset,
        &da->v_level_shift_offset, &da->ref_freq_offset,&da->vadj_p_offset, &da->vadj_n_offset
    };

    for (uint8_t i = 0; i < 20; i++)
    {
        const float vi1 = vi[i][0];
        const float vi2 = vi[i][1];
        const float vo1 = vo[i][0];
        const float vo2 = vo[i][1];

        const float dvi = vi2 - vi1;
        if (fabsf(dvi) < 1e-6f) {
            MIPI_CMD_INFO("calu_calibration_data: group %u invalid, vi1==vi2\r\n", i);
            return HAL_ERROR;
        }

        const float k = (vo2 - vo1) / dvi;
        const float b = vo1 - k * vi1;

        *gain_ptr[i] = k;
        *offset_ptr[i] = b;
        MIPI_CMD_INFO("calu_calibration_data: group %u, k=%.6f, b=%.2f\r\n", i, k, b);
    }

    // 更新CRC（如果后续要保存到Flash）
        g_calibration_manager.data.timestamp = HAL_GetTick();
        g_calibration_manager.data.crc32 = calibration_calculate_crc32(
        (uint8_t *)&g_calibration_manager.data,
        sizeof(calibration_data_t) - sizeof(uint32_t));

    g_calibration_manager.is_loaded = true;
    g_calibration_manager.is_valid = true;
    g_calibration_manager.last_error = CAL_ERROR_NONE;

    return HAL_OK;
}
/**
 * @brief Set default calibration values
 * @return HAL status
 */
HAL_StatusTypeDef calibration_set_defaults(void)
{
    calibration_data_t *cal = &g_calibration_manager.data;
    
    MIPI_CMD_INFO("Setting default calibration values...\r\n");

    // Clear the structure
    memset(cal, 0, sizeof(calibration_data_t));

    // Set header information
    cal->magic = CALIBRATION_MAGIC;
    cal->version = CALIBRATION_VERSION;
    cal->timestamp = HAL_GetTick();
    //Calibration Parameter Settings
    //calu_calibration_data();
    cal->da_data.vadj_n_gain =-5;
    cal->da_data.vadj_n_offset = -4884;

    cal->da_data.vadj_p_gain = -5.612;
    cal->da_data.vadj_p_offset = 25641;

    cal->da_data.v_level_shift_gain = 1;
    cal->da_data.v_level_shift_offset = 0;
    cal->da_data.ref_freq_gain = 1;
    cal->da_data.ref_freq_offset = 0;
    cal->da_data.vcc_ref_gain = 1.0f;
    cal->da_data.vcc_ref_offset = 0.0f;
    cal->da_data.iovcc_ref_gain = 1.0f;
    cal->da_data.iovcc_ref_offset = 0.0f;
    cal->da_data.vsp_ref_gain = 1.0f;
    cal->da_data.vsp_ref_offset = 0.0f;
    cal->da_data.vsn_ref_gain = 1.0f;
    cal->da_data.vsn_ref_offset = 0.0f;
    cal->da_data.avdd_ref_gain = 1.0f;
    cal->da_data.avdd_ref_offset = 0.0f;
    cal->da_data.vdd_ref_gain = 1.0f;
    cal->da_data.vdd_ref_offset = 0.0f;
    cal->da_data.elvdd_ref_gain = 1.0f;
    cal->da_data.elvdd_ref_offset = 0.0f;
    cal->da_data.elvss_ref_gain = 1.0f;
    cal->da_data.elvss_ref_offset = 0.0f;

    cal->da_data.elvss_set_gain = 11.111f;
    cal->da_data.elvss_set_offset = -24675;
    cal->da_data.vsn_set_gain = 11.111f;
    cal->da_data.vsn_set_offset = -24675;

    cal->da_data.vcc_set_gain = -10.408;
    cal->da_data.vcc_set_offset = 22400;
    cal->da_data.iovcc_set_gain = -10.408;
    cal->da_data.iovcc_set_offset = 22400;
    cal->da_data.vsp_set_gain = -10.408;
    cal->da_data.vsp_set_offset =22400;
    cal->da_data.avdd_set_gain = -10.408;
    cal->da_data.avdd_set_offset = 22400;
    cal->da_data.vdd_set_gain = -10.408;
    cal->da_data.vdd_set_offset = 22400;
    cal->da_data.elvdd_set_gain =-10.408;
    cal->da_data.elvdd_set_offset = 22400;




    //Default Voltage Value
    cal->vsn_last_voltage = -3000;
    cal->vsp_last_voltage = 3000;
    cal->iovcc_last_voltage = 3000;
    cal->vcc_last_voltage = 3000;
    cal->elvss_last_voltage = -3000;
    cal->elvdd_last_voltage = 3000;
    cal->vdd_last_voltage = 3000;
    cal->avdd_last_voltage = 3000;
    cal->vcc_ref_last = 1000;
    cal->iovcc_ref_last = 1000;
    cal->vsp_ref_last = 1000;
    cal->vsn_ref_last = 1000;
    cal->avdd_ref_last = 1000;
    cal->vdd_ref_last = 1000;
    cal->elvdd_ref_last = 1000;
    cal->elvss_ref_last = 1000;
    cal->v_level_shift_last = 980;
    cal->ref_freq_last = 980;
    cal->vadj_p_last = 10000;
    cal->vadj_n_last = -10000;
    
    //AD Calibration Default Values
    cal->ad_data.ch0_gain[0] = 5.0f;
    cal->ad_data.ch0_offset[0] = 0.0f;
    cal->ad_data.ch0_gain[1] = 5.0f;
    cal->ad_data.ch0_offset[1] = 0.0f;
    cal->ad_data.ch0_gain[2] = 5.0f;
    cal->ad_data.ch0_offset[2] = 0.0f;
    cal->ad_data.ch0_gain[3] = 5.0f;
    cal->ad_data.ch0_offset[3] = 0.0f;
    cal->ad_data.ch0_gain[4] = 5.0f;
    cal->ad_data.ch0_offset[4] = 0.0f;
    cal->ad_data.ch0_gain[5] = 5.0f;
    cal->ad_data.ch0_offset[5] = 0.0f;
    cal->ad_data.ch0_gain[6] = 5.0f;
    cal->ad_data.ch0_offset[6] = 0.0f;
    cal->ad_data.ch0_gain[7] = 5.0f;
    cal->ad_data.ch0_offset[7] = 0.0f;

    cal->ad_data.ch1_gain[0] = 6.0f;
    cal->ad_data.ch1_offset[0] = 0.0f;
    cal->ad_data.ch1_gain[1] = 1.0f;//无
    cal->ad_data.ch1_offset[1] = 0.0f;
    cal->ad_data.ch1_gain[2] = 0.2f;
    cal->ad_data.ch1_offset[2] = 0.0f;
    cal->ad_data.ch1_gain[3] = 0.2f;
    cal->ad_data.ch1_offset[3] = 0.0f;
    cal->ad_data.ch1_gain[4] = 0.5f;
    cal->ad_data.ch1_offset[4] = 0.0f;
    cal->ad_data.ch1_gain[5] = 0.05f;
    cal->ad_data.ch1_offset[5] = 0.0f;
    cal->ad_data.ch1_gain[6] = 0.05f;
    cal->ad_data.ch1_offset[6] = 0.0f;
    cal->ad_data.ch1_gain[7] = 0.2f;
    cal->ad_data.ch1_offset[7] = 0.0f;

    cal->ad_data.ch2_gain[0] = 1.0f;  //R=VoRt/(0.5-Vo) Vo为AD值    二极管模式采到的即为实际压降,必须用4.7K电阻档
    cal->ad_data.ch2_offset[0] = 0.0f;
    cal->ad_data.ch2_gain[1] = 1.0f; //无
    cal->ad_data.ch2_offset[1] = 0.0f;
    cal->ad_data.ch2_gain[2] = 11.0f;
    cal->ad_data.ch2_offset[2] = 2500.0f;
    cal->ad_data.ch2_gain[3] = 1.0f;//无
    cal->ad_data.ch2_offset[3] = 0.0f;
    cal->ad_data.ch2_gain[4] = 1.0f;//AD_V_BLAS_I
    cal->ad_data.ch2_offset[4] = 0.0f;
    cal->ad_data.ch2_gain[5] = 1.0f;//BLAS_V
    cal->ad_data.ch2_offset[5] = 0.0f;
    cal->ad_data.ch2_gain[6] = 0.099f;
    cal->ad_data.ch2_offset[6] = 0.0f;
    cal->ad_data.ch2_gain[7] = -5.0f;
    cal->ad_data.ch2_offset[7] = 0.0f;

    cal->ad_data.ch3_gain = 0.2f;
    cal->ad_data.ch3_offset = 0.0f;
    cal->ad_data.ch4_gain =  0.2f;
    cal->ad_data.ch4_offset = 0.0f;
    cal->ad_data.ch5_gain =  0.2f;
    cal->ad_data.ch5_offset = 0.0f;
    cal->ad_data.ch6_gain =  0.2f;
    cal->ad_data.ch6_offset = 0.0f;
    cal->ad_data.ch7_gain =  0.2f;
    cal->ad_data.ch7_offset = 0.0f;

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

    g_calibration_manager.load_attempts++;
    // Read data from Flash to buffer
    bsp_flash_read(cal_buffer, CALIBRATION_MAIN_ADDR, sizeof(calibration_data_t));
    
    // Copy to calibration data structure
    memcpy(cal, cal_buffer, sizeof(calibration_data_t));
    
    // Check magic number
    if (cal->magic != CALIBRATION_MAGIC) {
        W25Q256JVEQ_ERROR("Magic number check failed: 0x%08lX (expected: 0x%08lX)\r\n", 
               cal->magic, CALIBRATION_MAGIC);
        calibration_set_defaults();
        W25Q256JVEQ_INFO("Default calibration values have been set\r\n");
        calibration_save();
        calibration_load();
        
        g_calibration_manager.last_error = CAL_ERROR_MAGIC;
        return HAL_ERROR;
    }
    else
    {
        W25Q256JVEQ_INFO("Loaded calibration data with magic number: 0x%08lX\r\n", cal->magic);
    }
    
    // Check version compatibility
    if (cal->version > CALIBRATION_MAX_VERSION) {
        W25Q256JVEQ_INFO("Version not compatible: %lu (max supported: %d)\r\n", 
               cal->version, CALIBRATION_MAX_VERSION);
        g_calibration_manager.last_error = CAL_ERROR_VERSION;
        return HAL_ERROR;
    }
    
    // Verify CRC
    if (calibration_verify_crc(cal) != HAL_OK) {
        W25Q256JVEQ_ERROR("CRC check failed, trying backup data\r\n");
        return calibration_restore_from_backup();
    }
    
    // Load successful
    g_calibration_manager.is_loaded = true;
    g_calibration_manager.is_valid = true;
    g_calibration_manager.last_error = CAL_ERROR_NONE;
    
    W25Q256JVEQ_INFO("Calibration data loaded successfully (version: %lu, timestamp: %lu, CRC: 0x%08lX)\r\n", 
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
    
    W25Q256JVEQ_INFO("Saving calibration data to Flash...\r\n");
    
    // Update timestamp and CRC
    cal->timestamp = HAL_GetTick();
    cal->crc32 = calibration_calculate_crc32((uint8_t*)cal, 
                                            sizeof(calibration_data_t) - sizeof(uint32_t));//不计算CRC32字段本身
    
    // Copy data to buffer
    memcpy(cal_buffer, cal, sizeof(calibration_data_t));
    
    // Write to Flash
    if (!bsp_flash_write(cal_buffer, CALIBRATION_MAIN_ADDR, sizeof(calibration_data_t))) {
        W25Q256JVEQ_ERROR(" Flash write failed\r\n");
        g_calibration_manager.last_error = CAL_ERROR_FLASH_WRITE;
        return HAL_ERROR;
    }
    
    // Verify written data
    bsp_flash_read(cal_buffer, CALIBRATION_MAIN_ADDR, sizeof(calibration_data_t));
    if (memcmp(cal_buffer, cal, sizeof(calibration_data_t)) != 0) {
        W25Q256JVEQ_ERROR("Flash write verification failed\r\n");
        g_calibration_manager.last_error = CAL_ERROR_FLASH_WRITE;
        return HAL_ERROR;
    }
    
    g_calibration_manager.save_count++;
    W25Q256JVEQ_INFO("Calibration data saved successfully (CRC: 0x%08lX, save count: %lu)\r\n", 
           cal->crc32, g_calibration_manager.save_count);
    
    // Auto create backup
    if (calibration_backup() != HAL_OK) {
        W25Q256JVEQ_INFO("Backup creation failed, but main data saved\r\n");
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
    
    W25Q256JVEQ_INFO("Creating calibration data backup...\r\n");
    
    // Copy data to buffer
    memcpy(cal_buffer, cal, sizeof(calibration_data_t));
    
    // Write to backup 1
    if (!bsp_flash_write(cal_buffer, CALIBRATION_BACKUP1_ADDR, sizeof(calibration_data_t))) {
        W25Q256JVEQ_ERROR("Backup 1 write failed\r\n");
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
    
    W25Q256JVEQ_INFO(" Restoring calibration data from backup...\r\n");
    
    // Try backup 1
    bsp_flash_read(cal_buffer, CALIBRATION_BACKUP1_ADDR, sizeof(calibration_data_t));
    memcpy(&temp_cal, cal_buffer, sizeof(calibration_data_t));
    
    if (temp_cal.magic == CALIBRATION_MAGIC && calibration_verify_crc(&temp_cal) == HAL_OK) {
        W25Q256JVEQ_INFO(" Restored successfully from backup 1\r\n");
        memcpy(cal, &temp_cal, sizeof(calibration_data_t));
        g_calibration_manager.is_loaded = true;
        g_calibration_manager.is_valid = true;
        
        // Restore main data
        return calibration_save();
    }
    
    // All backups failed, use default values
    W25Q256JVEQ_INFO(" All backup data corrupted, using default values\r\n");
    calibration_set_defaults();
    return calibration_save();
}

/**
 * @brief Factory reset
 * @return HAL status
 */
HAL_StatusTypeDef calibration_factory_reset(void)
{
    W25Q256JVEQ_INFO("Performing factory reset...\r\n");
    
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
    MIPI_CMD_INFO("🚀 Initializing calibration data system...\r\n");
    
    // Initialize manager
    memset(&g_calibration_manager, 0, sizeof(calibration_manager_t));
    
    // Initialize Flash
    if (calibration_flash_init() != HAL_OK) {
        W25Q256JVEQ_ERROR("Flash initialization failed\r\n");
        return HAL_ERROR;
    }
    
    // Try to load data from Flash
    if (calibration_load() == HAL_OK) {
        W25Q256JVEQ_INFO("Calibration data system initialized successfully\r\n");
        return HAL_OK;
    }
    
    // Load failed, use default values
    W25Q256JVEQ_INFO("Unable to load valid calibration data, using default values\r\n");
    calibration_set_defaults();
    
    // Save default values to Flash
    if (calibration_save() == HAL_OK) {
        W25Q256JVEQ_INFO("Default calibration data saved to Flash\r\n");
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
    
    W25Q256JVEQ_INFO("\r\n=== Flash Data Dump (Addr: 0x%08lX, Size: %lu) ===\r\n", addr, size);
    
    for (uint32_t i = 0; i < size; i++) {
        if (i % 16 == 0) {
            W25Q256JVEQ_INFO("\r\n%08lX: ", addr + i);
        }
        W25Q256JVEQ_INFO("%02X ", cal_buffer[i]);
    }
    W25Q256JVEQ_INFO("\r\n");
}

/**
 * @brief CRC function test
 */
void calibration_test_crc(void)
{
    W25Q256JVEQ_INFO("\r\n=== CRC Function Test ===\r\n");
    
    // Test data
    uint8_t test_data[] = "Hello W25Q256 Calibration!";
    uint32_t test_len = strlen((char*)test_data);
    
    uint32_t crc = calibration_calculate_crc32(test_data, test_len);
    W25Q256JVEQ_INFO("Test data: %s\r\n", test_data);
    W25Q256JVEQ_INFO("CRC32: 0x%08lX\r\n", crc);
    
    // Test calibration data CRC
    calibration_data_t *cal = calibration_get_data();
    if (cal) {
        uint32_t calc_crc = calibration_calculate_crc32((uint8_t*)cal, 
                                                       sizeof(calibration_data_t) - sizeof(uint32_t));
        W25Q256JVEQ_INFO("Calibration data stored CRC: 0x%08lX\r\n", cal->crc32);
        W25Q256JVEQ_INFO("Calibration data calculated CRC: 0x%08lX\r\n", calc_crc);
        W25Q256JVEQ_INFO("CRC check: %s\r\n", (calc_crc == cal->crc32) ? " Pass" : "Fail");
    }
}


