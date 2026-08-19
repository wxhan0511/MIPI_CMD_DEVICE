/************************************************************
 * @file       bsp_calibration.c
 * @brief      calibration
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-08-06
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 ************************************************************/

/*==================== 1. 头文件包含 ====================*/
#include "bsp_calibration.h"
#include "bsp_spi_flash.h"
#include <string.h>
#include <stdio.h>
#include "crc.h"
#include "math.h"

/*==================== 2. 宏定义 ====================*/
// 此处可添加本文件专用宏定义

/*==================== 3. 类型定义（结构体、枚举、别名） ====================*/
// 本文件无新增类型定义，相关类型在 .h 文件中定义

/*==================== 4. 外部全局变量 ====================*/
calibration_manager_t g_calibration_manager = {0};

/*==================== 5. 静态私有变量 ====================*/
static uint8_t cal_buffer[sizeof(calibration_data_t) + 256]; // Flash操作缓冲区
static const float vi[20][2] = {
    {2000, 1500}, {1500, 1300}, {1500, 1200}, {1500, 2000}, {1500, 3000}, {1500, 3000}, {1500, 3000}, {1500, 3000}, {1500, 1550}, {1500, 1400}, {1500, 1100}, {1500, 1100}, {1500, 3000}, {1500, 3000}, {1500, 3000}, {1500, 3000}, {1500, 3000}, {1500, 3000}, {2500, 2400}, {3000, 2000}};
static const float vo[20][2] = {
    {1505.5, 4946}, {2623, 4003}, {2792.5, 5312}, {-3219, -1235.7}, {1500, 3000}, {1500, 3000}, {1500, 3000}, {1500, 3000}, {4894, 4546}, {2702, 3535.7}, {2637.6, 6000.6}, {-3115, -5796}, {1500, 3000}, {1500, 3000}, {1500, 3000}, {1500, 3000}, {1500, 3000}, {1500, 3000}, {8526, 9198}, {-18441, -14282}};

/*==================== 6. 静态函数声明 ====================*/
static HAL_StatusTypeDef calu_calibration_data(void);

/*==================== 7. 外部可调用函数实现 ====================*/

/**
 * @brief CRC32计算（使用STM32硬件CRC）
 */
uint32_t calibration_calculate_crc32(uint8_t *data, uint32_t length)
{
    uint32_t crc_value;
    __HAL_CRC_DR_RESET(&hcrc);
    uint32_t word_count = length / 4;
    uint32_t remaining_bytes = length % 4;

    if (word_count > 0)
    {
        crc_value = HAL_CRC_Accumulate(&hcrc, (uint32_t *)data, word_count);
    }
    else
    {
        crc_value = hcrc.Instance->DR;
    }

    if (remaining_bytes > 0)
    {
        uint32_t last_word = 0;
        uint8_t *last_bytes = data + (word_count * 4);
        for (int i = 0; i < remaining_bytes; i++)
        {
            last_word |= ((uint32_t)last_bytes[i]) << (i * 8);
        }
        crc_value = HAL_CRC_Accumulate(&hcrc, &last_word, 1);
    }
    return crc_value;
}

/**
 * @brief 校验校准数据CRC
 */
HAL_StatusTypeDef calibration_verify_crc(calibration_data_t *cal_data)
{
    uint32_t stored_crc = cal_data->crc32;
    uint32_t calc_crc = calibration_calculate_crc32((uint8_t *)cal_data, sizeof(calibration_data_t) - sizeof(uint32_t));
    if (stored_crc == calc_crc)
    {
        return HAL_OK;
    }
    else
    {
        MIPI_CMD_INFO("CRC check failed: stored=0x%08lX, calculated=0x%08lX\r\n", stored_crc, calc_crc);
        g_calibration_manager.last_error = CAL_ERROR_CRC;
        return HAL_ERROR;
    }
}

/**
 * @brief SPI Flash初始化
 */
HAL_StatusTypeDef calibration_flash_init(void)
{
    uint32_t flash_id = bsp_flash_read_id();
    g_calibration_manager.flash_id = flash_id;
    MIPI_CMD_INFO("Detected Flash ID: 0x%06lX\r\n", flash_id);
    if ((flash_id & 0xFFFFFF) == 0xEF4019)
    {
        MIPI_CMD_INFO("W25Q256JVEQ Flash detected successfully\r\n");
        return HAL_OK;
    }
    else
    {
        MIPI_CMD_INFO("Flash ID mismatch, expected: 0xEF4019, actual: 0x%06lX\r\n", flash_id);
        g_calibration_manager.last_error = CAL_ERROR_FLASH_READ;
        return HAL_ERROR;
    }
}

/**
 * @brief 设置校准默认值
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
    // Calibration Parameter Settings
    // calu_calibration_data();
    cal->da_data.vadj_n_gain = -5;
    cal->da_data.vadj_n_offset = -7326;

    cal->da_data.vadj_p_gain = -5.612;
    cal->da_data.vadj_p_offset = 25641;

    cal->da_data.v_level_shift_gain = -1.109;
    cal->da_data.v_level_shift_offset = 5087;
    cal->da_data.ref_freq_gain = 1;
    cal->da_data.ref_freq_offset = 0;
    cal->da_data.vcc_ref_gain = 0.2f;
    cal->da_data.vcc_ref_offset = 0.0f;
    cal->da_data.iovcc_ref_gain = 0.2f;
    cal->da_data.iovcc_ref_offset = 0.0f;
    cal->da_data.vsp_ref_gain = 0.2f;
    cal->da_data.vsp_ref_offset = 0.0f;
    cal->da_data.vsn_ref_gain = 0.2f;
    cal->da_data.vsn_ref_offset = 0.0f;
    cal->da_data.avdd_ref_gain = 0.2f;
    cal->da_data.avdd_ref_offset = 0.0f;
    cal->da_data.vdd_ref_gain = 0.2f;
    cal->da_data.vdd_ref_offset = 0.0f;
    cal->da_data.elvdd_ref_gain = 0.2f;
    cal->da_data.elvdd_ref_offset = 0.0f;
    cal->da_data.elvss_ref_gain = 0.2f;
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
    cal->da_data.vsp_set_offset = 22400;
    cal->da_data.avdd_set_gain = -10.408;
    cal->da_data.avdd_set_offset = 22400;
    cal->da_data.vdd_set_gain = -10.408;
    cal->da_data.vdd_set_offset = 22400;
    cal->da_data.elvdd_set_gain = -10.408;
    cal->da_data.elvdd_set_offset = 22400;

    // Default Voltage Value
    cal->vsn_last_voltage = -5000;
    cal->vsp_last_voltage = 3000;
    cal->iovcc_last_voltage = 3000;
    cal->vcc_last_voltage = 3000;
    cal->elvss_last_voltage = -5000;
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
    cal->v_level_shift_last = 1800;
    cal->ref_freq_last = 980;
    cal->vadj_p_last = 10000;
    cal->vadj_n_last = -10000;

    // AD Calibration Default Values
    cal->ad_data.ch0_gain[0] = 5.0f;
    cal->ad_data.ch0_offset[0] = 0.0f;
    cal->ad_data.ch0_gain[1] = 5.0f;
    cal->ad_data.ch0_offset[1] = 0.0f;
    cal->ad_data.ch0_gain[2] = -5.0f;
    cal->ad_data.ch0_offset[2] = 0.0f;
    cal->ad_data.ch0_gain[3] = -5.0f;
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
    cal->ad_data.ch1_gain[1] = 1.0f; // 无
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

    cal->ad_data.ch2_gain[0] = 1.0f; // R=VoRt/(0.5-Vo) Vo为AD值    二极管模式采到的即为实际压降,必须用4.7K电阻档
    cal->ad_data.ch2_offset[0] = 0.0f;
    cal->ad_data.ch2_gain[1] = 1.0f; // 无
    cal->ad_data.ch2_offset[1] = 0.0f;
    cal->ad_data.ch2_gain[2] = 11.0f;
    cal->ad_data.ch2_offset[2] = -27.5f;
    cal->ad_data.ch2_gain[3] = 1.0f; // 无
    cal->ad_data.ch2_offset[3] = 0.0f;
    cal->ad_data.ch2_gain[4] = 1.0f; // AD_V_BLAS_I
    cal->ad_data.ch2_offset[4] = 0.0f;
    cal->ad_data.ch2_gain[5] = 1.0f; // BLAS_V
    cal->ad_data.ch2_offset[5] = 0.0f;
    cal->ad_data.ch2_gain[6] = 0.099f;
    cal->ad_data.ch2_offset[6] = 0.0f;
    cal->ad_data.ch2_gain[7] = -5.0f;
    cal->ad_data.ch2_offset[7] = 0.0f;

    cal->ad_data.ch3_gain = 0.2f;
    cal->ad_data.ch3_offset = 0.0f;
    cal->ad_data.ch4_gain = 0.2f;
    cal->ad_data.ch4_offset = 0.0f;
    cal->ad_data.ch5_gain = 0.2f;
    cal->ad_data.ch5_offset = 0.0f;
    cal->ad_data.ch6_gain = 0.2f;
    cal->ad_data.ch6_offset = 0.0f;
    cal->ad_data.ch7_gain = 0.2f;
    cal->ad_data.ch7_offset = 0.0f;

    // Clear reserved fields
    memset(cal->reserved, 0, sizeof(cal->reserved));

    // Calculate and set CRC , execlude crc32 field itself
    cal->crc32 = calibration_calculate_crc32((uint8_t *)cal,
                                             sizeof(calibration_data_t) - sizeof(uint32_t));

    // Update manager status
    g_calibration_manager.is_loaded = true;
    g_calibration_manager.is_valid = true;
    g_calibration_manager.last_error = CAL_ERROR_NONE;

    return HAL_OK;
}
HAL_StatusTypeDef power_set_defaults(void)
{
    calibration_data_t *cal = &g_calibration_manager.data;

    // Calibration Parameter Settings
    // calu_calibration_data();
    cal->da_data.vadj_n_gain = -5;
    cal->da_data.vadj_n_offset = -7326;

    cal->da_data.vadj_p_gain = -5.612;
    cal->da_data.vadj_p_offset = 25641;

    cal->da_data.v_level_shift_gain = -1.109;
    cal->da_data.v_level_shift_offset = 5087;
    cal->da_data.ref_freq_gain = 1;
    cal->da_data.ref_freq_offset = 0;
    cal->da_data.vcc_ref_gain = 0.2f;
    cal->da_data.vcc_ref_offset = 0.0f;
    cal->da_data.iovcc_ref_gain = 0.2f;
    cal->da_data.iovcc_ref_offset = 0.0f;
    cal->da_data.vsp_ref_gain = 0.2f;
    cal->da_data.vsp_ref_offset = 0.0f;
    cal->da_data.vsn_ref_gain = 0.2f;
    cal->da_data.vsn_ref_offset = 0.0f;
    cal->da_data.avdd_ref_gain = 0.2f;
    cal->da_data.avdd_ref_offset = 0.0f;
    cal->da_data.vdd_ref_gain = 0.2f;
    cal->da_data.vdd_ref_offset = 0.0f;
    cal->da_data.elvdd_ref_gain = 0.2f;
    cal->da_data.elvdd_ref_offset = 0.0f;
    cal->da_data.elvss_ref_gain = 0.2f;
    cal->da_data.elvss_ref_offset = 0.0f;

    cal->vcc_ref_last = 1000;
    cal->iovcc_ref_last = 1000;
    cal->vsp_ref_last = 1000;
    cal->vsn_ref_last = 1000;
    cal->avdd_ref_last = 1000;
    cal->vdd_ref_last = 1000;
    cal->elvdd_ref_last = 1000;
    cal->elvss_ref_last = 1000;
    cal->v_level_shift_last = 1800;
    cal->ref_freq_last = 980;
    cal->vadj_p_last = 10000;
    cal->vadj_n_last = -10000;

    return HAL_OK;
}
/**
 * @brief 从Flash加载校准数据
 */
HAL_StatusTypeDef calibration_load(void)
{
    calibration_data_t *cal = &g_calibration_manager.data;
    g_calibration_manager.load_attempts++;
    bsp_flash_read(cal_buffer, CALIBRATION_MAIN_ADDR, sizeof(calibration_data_t));
    memcpy(cal, cal_buffer, sizeof(calibration_data_t));
    if (cal->magic != CALIBRATION_MAGIC)
    {
        W25Q256JVEQ_ERROR("Magic number check failed: 0x%08lX (expected: 0x%08lX)\r\n", cal->magic, CALIBRATION_MAGIC);
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
    if (cal->version > CALIBRATION_MAX_VERSION)
    {
        W25Q256JVEQ_INFO("Version not compatible: %lu (max supported: %d)\r\n", cal->version, CALIBRATION_MAX_VERSION);
        g_calibration_manager.last_error = CAL_ERROR_VERSION;
        return HAL_ERROR;
    }
    if (calibration_verify_crc(cal) != HAL_OK)
    {
        W25Q256JVEQ_ERROR("CRC check failed, trying backup data\r\n");
        return calibration_restore_from_backup();
    }
    g_calibration_manager.is_loaded = true;
    g_calibration_manager.is_valid = true;
    g_calibration_manager.last_error = CAL_ERROR_NONE;
    W25Q256JVEQ_INFO("Calibration data loaded successfully (version: %lu, timestamp: %lu, CRC: 0x%08lX)\r\n",
                     cal->version, cal->timestamp, cal->crc32);
    return HAL_OK;
}

void print_all_calibration_data(void)
{
    calibration_data_t *cal = &g_calibration_manager.data;
    printf("\r\n==================== All Cali Value Print ====================\r\n");
    // 1. Header Info
    printf("[Header Info]\r\n");
    printf("  Magic:     0x%08X\r\n", cal->magic);
    printf("  Version:   %d\r\n", cal->version);
    printf("  Timestamp: %lu\r\n", cal->timestamp);
    printf("  CRC32:     0x%08X\r\n", cal->crc32);
    // 2. DA Calibration - Vadj, Level Shift, Ref Freq
    printf("\r\n[DA Data - Vadj & Level Shift & Freq]\r\n");
    printf("  vadj_n:        gain=%.4f, offset=%.4f\r\n", cal->da_data.vadj_n_gain, cal->da_data.vadj_n_offset);
    printf("  vadj_p:        gain=%.4f, offset=%.4f\r\n", cal->da_data.vadj_p_gain, cal->da_data.vadj_p_offset);
    printf("  v_level_shift: gain=%.4f, offset=%.4f\r\n", cal->da_data.v_level_shift_gain, cal->da_data.v_level_shift_offset);
    printf("  ref_freq:      gain=%.4f, offset=%.4f\r\n", cal->da_data.ref_freq_gain, cal->da_data.ref_freq_offset);
    // 3. DA Calibration - Power Refs
    printf("\r\n[DA Data - Power Refs]\r\n");
    printf("  VCC:    gain=%.4f, offset=%.4f\r\n", cal->da_data.vcc_ref_gain, cal->da_data.vcc_ref_offset);
    printf("  IOVCC:  gain=%.4f, offset=%.4f\r\n", cal->da_data.iovcc_ref_gain, cal->da_data.iovcc_ref_offset);
    printf("  VSP:    gain=%.4f, offset=%.4f\r\n", cal->da_data.vsp_ref_gain, cal->da_data.vsp_ref_offset);
    printf("  VSN:    gain=%.4f, offset=%.4f\r\n", cal->da_data.vsn_ref_gain, cal->da_data.vsn_ref_offset);
    printf("  AVDD:   gain=%.4f, offset=%.4f\r\n", cal->da_data.avdd_ref_gain, cal->da_data.avdd_ref_offset);
    printf("  VDD:    gain=%.4f, offset=%.4f\r\n", cal->da_data.vdd_ref_gain, cal->da_data.vdd_ref_offset);
    printf("  ELVDD:  gain=%.4f, offset=%.4f\r\n", cal->da_data.elvdd_ref_gain, cal->da_data.elvdd_ref_offset);
    printf("  ELVSS:  gain=%.4f, offset=%.4f\r\n", cal->da_data.elvss_ref_gain, cal->da_data.elvss_ref_offset);
    // 4. DA Calibration - Power Sets
    printf("\r\n[DA Data - Power Sets]\r\n");
    printf("  ELVSS:  gain=%.4f, offset=%.4f\r\n", cal->da_data.elvss_set_gain, cal->da_data.elvss_set_offset);
    printf("  VSN:    gain=%.4f, offset=%.4f\r\n", cal->da_data.vsn_set_gain, cal->da_data.vsn_set_offset);
    printf("  VCC:    gain=%.4f, offset=%.4f\r\n", cal->da_data.vcc_set_gain, cal->da_data.vcc_set_offset);
    printf("  IOVCC:  gain=%.4f, offset=%.4f\r\n", cal->da_data.iovcc_set_gain, cal->da_data.iovcc_set_offset);
    printf("  VSP:    gain=%.4f, offset=%.4f\r\n", cal->da_data.vsp_set_gain, cal->da_data.vsp_set_offset);
    printf("  AVDD:   gain=%.4f, offset=%.4f\r\n", cal->da_data.avdd_set_gain, cal->da_data.avdd_set_offset);
    printf("  VDD:    gain=%.4f, offset=%.4f\r\n", cal->da_data.vdd_set_gain, cal->da_data.vdd_set_offset);
    printf("  ELVDD:  gain=%.4f, offset=%.4f\r\n", cal->da_data.elvdd_set_gain, cal->da_data.elvdd_set_offset);
    // 5. Last Values (Voltages & Refs) - Cast to float for printing
    printf("\r\n[Last Values]\r\n");
    printf("  Voltages (mV): VSN=%.4f, VSP=%.4f, IOVCC=%.4f, VCC=%.4f, ELVSS=%.4f, ELVDD=%.4f, VDD=%.4f, AVDD=%.4f\r\n",
           (float)cal->vsn_last_voltage, (float)cal->vsp_last_voltage, (float)cal->iovcc_last_voltage, (float)cal->vcc_last_voltage,
           (float)cal->elvss_last_voltage, (float)cal->elvdd_last_voltage, (float)cal->vdd_last_voltage, (float)cal->avdd_last_voltage);
    printf("  Refs: VCC=%.4f, IOVCC=%.4f, VSP=%.4f, VSN=%.4f, AVDD=%.4f, VDD=%.4f, ELVDD=%.4f, ELVSS=%.4f\r\n",
           (float)cal->vcc_ref_last, (float)cal->iovcc_ref_last, (float)cal->vsp_ref_last, (float)cal->vsn_ref_last,
           (float)cal->avdd_ref_last, (float)cal->vdd_ref_last, (float)cal->elvdd_ref_last, (float)cal->elvss_ref_last);
    printf("  Others: v_level_shift=%.4f, ref_freq=%.4f, vadj_p=%.4f, vadj_n=%.4f\r\n",
           (float)cal->v_level_shift_last, (float)cal->ref_freq_last, (float)cal->vadj_p_last, (float)cal->vadj_n_last);
    // 6. AD Calibration - Ch0~Ch2 (Arrays)
    const char *ch0_name = "CH0 (Voltage)";
    const char *ch1_name = "CH1 (Current)";
    const char *ch2_name = "CH2 (Other)";
    printf("\r\n[AD Data - Channel 0/1/2]\r\n");
    for (uint8_t i = 0; i < 8; i++)
    {
        printf("  %s[%d]: gain=%.4f, offset=%.4f | ", ch0_name, i, cal->ad_data.ch0_gain[i], cal->ad_data.ch0_offset[i]);
        printf("%s[%d]: gain=%.4f, offset=%.4f | ", ch1_name, i, cal->ad_data.ch1_gain[i], cal->ad_data.ch1_offset[i]);
        printf("%s[%d]: gain=%.4f, offset=%.4f\r\n", ch2_name, i, cal->ad_data.ch2_gain[i], cal->ad_data.ch2_offset[i]);
    }
    // 7. AD Calibration - Ch3~Ch7 (Variables)
    printf("\r\n[AD Data - Channel 3 to 7]\r\n");
    printf("  CH3: gain=%.4f, offset=%.4f\r\n", cal->ad_data.ch3_gain, cal->ad_data.ch3_offset);
    printf("  CH4: gain=%.4f, offset=%.4f\r\n", cal->ad_data.ch4_gain, cal->ad_data.ch4_offset);
    printf("  CH5: gain=%.4f, offset=%.4f\r\n", cal->ad_data.ch5_gain, cal->ad_data.ch5_offset);
    printf("  CH6: gain=%.4f, offset=%.4f\r\n", cal->ad_data.ch6_gain, cal->ad_data.ch6_offset);
    printf("  CH7: gain=%.4f, offset=%.4f\r\n", cal->ad_data.ch7_gain, cal->ad_data.ch7_offset);
    printf("========================================================\r\n");
}
/**
 * @brief 保存校准数据到Flash
 */
HAL_StatusTypeDef calibration_save(void)
{
    calibration_data_t *cal = &g_calibration_manager.data;
    W25Q256JVEQ_INFO("Saving calibration data to Flash...\r\n");
    cal->timestamp = HAL_GetTick();
    cal->crc32 = calibration_calculate_crc32((uint8_t *)cal, sizeof(calibration_data_t) - sizeof(uint32_t));
    memcpy(cal_buffer, cal, sizeof(calibration_data_t));
    if (!bsp_flash_write(cal_buffer, CALIBRATION_MAIN_ADDR, sizeof(calibration_data_t)))
    {
        W25Q256JVEQ_ERROR(" Flash write failed\r\n");
        g_calibration_manager.last_error = CAL_ERROR_FLASH_WRITE;
        return HAL_ERROR;
    }
    bsp_flash_read(cal_buffer, CALIBRATION_MAIN_ADDR, sizeof(calibration_data_t));
    if (memcmp(cal_buffer, cal, sizeof(calibration_data_t)) != 0)
    {
        W25Q256JVEQ_ERROR("Flash write verification failed\r\n");
        g_calibration_manager.last_error = CAL_ERROR_FLASH_WRITE;
        return HAL_ERROR;
    }
    g_calibration_manager.save_count++;
    W25Q256JVEQ_INFO("Calibration data saved successfully (CRC: 0x%08lX, save count: %lu)\r\n",
                     cal->crc32, g_calibration_manager.save_count);
    if (calibration_backup() != HAL_OK)
    {
        W25Q256JVEQ_INFO("Backup creation failed, but main data saved\r\n");
    }
    return HAL_OK;
}

/**
 * @brief 备份校准数据
 */
HAL_StatusTypeDef calibration_backup(void)
{
    calibration_data_t *cal = &g_calibration_manager.data;
    W25Q256JVEQ_INFO("Creating calibration data backup...\r\n");
    memcpy(cal_buffer, cal, sizeof(calibration_data_t));
    if (!bsp_flash_write(cal_buffer, CALIBRATION_BACKUP1_ADDR, sizeof(calibration_data_t)))
    {
        W25Q256JVEQ_ERROR("Backup 1 write failed\r\n");
        g_calibration_manager.last_error = CAL_ERROR_BACKUP_FAILED;
        return HAL_ERROR;
    }
    return HAL_OK;
}

/**
 * @brief 从备份恢复校准数据
 */
HAL_StatusTypeDef calibration_restore_from_backup(void)
{
    calibration_data_t temp_cal;
    calibration_data_t *cal = &g_calibration_manager.data;
    W25Q256JVEQ_INFO(" Restoring calibration data from backup...\r\n");
    bsp_flash_read(cal_buffer, CALIBRATION_BACKUP1_ADDR, sizeof(calibration_data_t));
    memcpy(&temp_cal, cal_buffer, sizeof(calibration_data_t));
    if (temp_cal.magic == CALIBRATION_MAGIC && calibration_verify_crc(&temp_cal) == HAL_OK)
    {
        W25Q256JVEQ_INFO(" Restored successfully from backup 1\r\n");
        memcpy(cal, &temp_cal, sizeof(calibration_data_t));
        g_calibration_manager.is_loaded = true;
        g_calibration_manager.is_valid = true;
        return calibration_save();
    }
    W25Q256JVEQ_INFO(" All backup data corrupted, using default values\r\n");
    calibration_set_defaults();
    return calibration_save();
}

/**
 * @brief 恢复出厂设置
 */
HAL_StatusTypeDef calibration_factory_reset(void)
{
    W25Q256JVEQ_INFO("Performing factory reset...\r\n");
    bsp_flash_erase_sector(CALIBRATION_MAIN_ADDR);
    bsp_flash_erase_sector(CALIBRATION_BACKUP1_ADDR);
    calibration_set_defaults();
    return calibration_save();
}

/**
 * @brief 校准数据系统初始化
 */
HAL_StatusTypeDef calibration_init(void)
{
    MIPI_CMD_INFO("🚀 Initializing calibration data system...\r\n");
    memset(&g_calibration_manager, 0, sizeof(calibration_manager_t));
    if (calibration_flash_init() != HAL_OK)
    {
        W25Q256JVEQ_ERROR("Flash initialization failed\r\n");
        return HAL_ERROR;
    }
    if (calibration_load() == HAL_OK)
    {
        W25Q256JVEQ_INFO("Calibration data system initialized successfully\r\n");
        return HAL_OK;
    }
    W25Q256JVEQ_INFO("Unable to load valid calibration data, using default values\r\n");
    calibration_set_defaults();
    if (calibration_save() == HAL_OK)
    {
        W25Q256JVEQ_INFO("Default calibration data saved to Flash\r\n");
    }
    return HAL_OK;
}

/**
 * @brief 获取校准数据指针
 */
calibration_data_t *calibration_get_data(void)
{
    if (g_calibration_manager.is_valid)
    {
        return &g_calibration_manager.data;
    }
    return NULL;
}

/**
 * @brief 校准数据有效性检查
 */
bool calibration_is_valid(void)
{
    return g_calibration_manager.is_valid;
}

/**
 * @brief Flash数据转储（调试用）
 */
void calibration_dump_data(uint32_t addr, uint32_t size)
{
    if (size > sizeof(cal_buffer))
    {
        size = sizeof(cal_buffer);
    }
    bsp_flash_read(cal_buffer, addr, size);
    W25Q256JVEQ_INFO("\r\n=== Flash Data Dump (Addr: 0x%08lX, Size: %lu) ===\r\n", addr, size);
    for (uint32_t i = 0; i < size; i++)
    {
        if (i % 16 == 0)
        {
            W25Q256JVEQ_INFO("\r\n%08lX: ", addr + i);
        }
        W25Q256JVEQ_INFO("%02X ", cal_buffer[i]);
    }
    W25Q256JVEQ_INFO("\r\n");
}

/**
 * @brief CRC功能测试
 */
void calibration_test_crc(void)
{
    W25Q256JVEQ_INFO("\r\n=== CRC Function Test ===\r\n");
    uint8_t test_data[] = "Hello W25Q256 Calibration!";
    uint32_t test_len = strlen((char *)test_data);
    uint32_t crc = calibration_calculate_crc32(test_data, test_len);
    W25Q256JVEQ_INFO("Test data: %s\r\n", test_data);
    W25Q256JVEQ_INFO("CRC32: 0x%08lX\r\n", crc);
    calibration_data_t *cal = calibration_get_data();
    if (cal)
    {
        uint32_t calc_crc = calibration_calculate_crc32((uint8_t *)cal, sizeof(calibration_data_t) - sizeof(uint32_t));
        W25Q256JVEQ_INFO("Calibration data stored CRC: 0x%08lX\r\n", cal->crc32);
        W25Q256JVEQ_INFO("Calibration data calculated CRC: 0x%08lX\r\n", calc_crc);
        W25Q256JVEQ_INFO("CRC check: %s\r\n", (calc_crc == cal->crc32) ? " Pass" : "Fail");
    }
}

/*==================== 8. 静态私有函数实现 ====================*/

/**
 * @brief 计算校准参数
 */
static HAL_StatusTypeDef calu_calibration_data(void)
{
    da_calibration_data_t *da = &g_calibration_manager.data.da_data;
    float *gain_ptr[20] = {
        &da->vcc_set_gain, &da->iovcc_set_gain, &da->vsp_set_gain, &da->vsn_set_gain,
        &da->vcc_ref_gain, &da->iovcc_ref_gain, &da->vsp_ref_gain, &da->vsn_ref_gain,
        &da->avdd_set_gain, &da->vdd_set_gain, &da->elvdd_set_gain, &da->elvss_set_gain,
        &da->avdd_ref_gain, &da->vdd_ref_gain, &da->elvdd_ref_gain, &da->elvss_ref_gain,
        &da->v_level_shift_gain, &da->ref_freq_gain, &da->vadj_p_gain, &da->vadj_n_gain};
    float *offset_ptr[20] = {
        &da->vcc_set_offset, &da->iovcc_set_offset, &da->vsp_set_offset, &da->vsn_set_offset,
        &da->vcc_ref_offset, &da->iovcc_ref_offset, &da->vsp_ref_offset, &da->vsn_ref_offset,
        &da->avdd_set_offset, &da->vdd_set_offset, &da->elvdd_set_offset, &da->elvss_set_offset,
        &da->avdd_ref_offset, &da->vdd_ref_offset, &da->elvdd_ref_offset, &da->elvss_ref_offset,
        &da->v_level_shift_offset, &da->ref_freq_offset, &da->vadj_p_offset, &da->vadj_n_offset};
    for (uint8_t i = 0; i < 20; i++)
    {
        const float vi1 = vi[i][0];
        const float vi2 = vi[i][1];
        const float vo1 = vo[i][0];
        const float vo2 = vo[i][1];
        const float dvi = vi2 - vi1;
        if (fabsf(dvi) < 1e-6f)
        {
            MIPI_CMD_INFO("calu_calibration_data: group %u invalid, vi1==vi2\r\n", i);
            return HAL_ERROR;
        }
        const float k = (vo2 - vo1) / dvi;
        const float b = vo1 - k * vi1;
        *gain_ptr[i] = k;
        *offset_ptr[i] = b;
        MIPI_CMD_INFO("calu_calibration_data: group %u, k=%.6f, b=%.2f\r\n", i, k, b);
    }
    g_calibration_manager.data.timestamp = HAL_GetTick();
    g_calibration_manager.data.crc32 = calibration_calculate_crc32(
        (uint8_t *)&g_calibration_manager.data, sizeof(calibration_data_t) - sizeof(uint32_t));
    g_calibration_manager.is_loaded = true;
    g_calibration_manager.is_valid = true;
    g_calibration_manager.last_error = CAL_ERROR_NONE;
    return HAL_OK;
}
