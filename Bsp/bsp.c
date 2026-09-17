#include <stdio.h>
#include <string.h>
#include "main.h"
#include "usart.h"
#include "usb_device.h"
#include "oled.h"
#include "i2c.h"

#include "crc.h"
#include "tim.h"
#include "bsp_lcd.h"
#include "dac.h"
/* drivers */
#include "retarget.h"
#include "delay.h"
#include "drv_ra_device.h"
/*usb driver*/
#include "usb_otg.h"
/*utils*/
#include "utils.h"
/*bsp*/
#include "bsp_mcp4728_ctl.h"
#include "bsp_i2c_bus.h"
#include "bsp.h"
#include "bsp_ads1256.h"
#include "bsp_channel_sel.h"
#include "bsp_dwt.h"
#include "bsp_mcp4728.h"
#include "bsp_spi_flash.h"
#include "bsp_calibration.h"
#include "bsp_power.h"
#include "task_sample.h"
#include "calibration_utils.h"
/* Private function prototypes */
static void bsp_print_version_info(void);
static HAL_StatusTypeDef bsp_init_adc_system(void);
static void bsp_test_spi_flash(void);
void test_pwm(void);
void test_ccp(void);

extern SPI_HandleTypeDef hspi2;
uint8_t id = 0x01;

/**
 * @brief Software version [Major.Minor.Patch.Build]. Part of the 16-byte boot
 *        metadata block; layout must match bootloader boot_version.h.
 */
__attribute__((section(".fw_version"), used)) uint8_t sw_version[4] = {0, 0, 0, 1};

/**
 * @brief Hardware version [Major.Minor.Patch.Build]
 */
__attribute__((section(".hw_version"), used)) uint8_t hw_version[4] = {0, 0, 0, 1};

/**
 * @brief Firmware CRC-32 over the app data area. Filled by the host before
 *        flashing and verified by the bootloader at boot.
 */
__attribute__((section(".boot_crc"), used)) uint8_t boot_crc[4] = {0, 0, 0, 0};

/**
 * @brief Reserved metadata bytes (kept zero).
 */
__attribute__((section(".boot_reserved"), used)) uint8_t boot_reserved[4] = {0, 0, 0, 0};

/**
 * @brief Magic number for firmware verification
 */
uint8_t magic_number[4] = {0x12, 0x34, 0xf8, 0x8f};

void bsp_init()
{
    bsp_retarget_init(&huart3);
    bsp_print_version_info();

    bsp_init_dwt();
    bsp_lcd_reset(&lcd);
    // TIME_DEBUG("test100: %lu ms\r\n", dwt_get_ms());
    // TIME_DEBUG("test100: %lu ms\r\n", dwt_get_ms());
    bsp_all_d_trigger_init();
    printf("------------------------------------------------------------------------------------05\r\n");
    printf("\r\n==================================================\r\n");
    printf(" all rly set ma.\r\n");
    printf("==================================================\r\n\r\n");
    bsp_rly_gear_set_all(GEAR_mA);
    printf("\r\n==================================================\r\n");
    printf(" calibration loading...\r\n");
    printf("==================================================\r\n\r\n");
    calibration_load();
    power_set_defaults();
    calibration_save();
    print_all_calibration_data();
    /*-------------ADC START---------------------------*/
    bsp_init_adc_system();
    /*-------------ADC END---------------------------*/

    /*-------------DAC and LIMIT CURRENT START----------*/
    bsp_dac_init();
    LEVEL_SHIFT_ENABLE();

    /*-------------DAC and LIMIT CURRENT END------------*/
    /*-------------PWM START----------------*/
    bsp_led_pwm_init(47);   // step1
    bsp_blasi_pwm_init(10); // step1
    enableTim1PWMOutput();  // step2
    enableTim2PWMOutput();  // step2

    bsp_lim_rst_set(1);
    /*-------------PWM END----------------*/
    /*-------------CCP START----------------*/
    // bsp_CCP_Init();
    // test_ccp();
    /*-------------CCP END----------------*/
    MIPI_CMD_INFO("------------- bsp init finish -------------\r\n");
}

void bsp_led_pwm_init(uint8_t pulse)
{
    HAL_TIM_IC_DeInit(&htim1);
    HAL_TIM_PWM_DeInit(&htim1);
    uint16_t arr = 1050; // Period; duty cycle resolution: 1 / 1050 = 0.0952% (better than 0.1%)
    uint16_t psc = 15;   // Prescaler
    // Recommended duty cycle: 1% (50% is especially bright)
    uint16_t pulses_num = 11000;
    printf("pulse value: %d\r\n", pulse);
    TIM1_PWM_Init(arr, psc, pulse); // arr,psc,pulse f=168MHz/(arr+1)*(psc+1)    Max usable 28MHz: TIM1_PWM_Init(2,3), compare value set to 1, __HAL_TIM_SET_COMPARE(&htim1, LED_PWM_IN_CHANNEL, 1);;
    printf("TIM1 PWM Init with ARR=%d, PSC=%d, Pulse=%d, freq = %lu Hz\r\n", arr, psc, pulse, 168000000 / ((arr + 1) * (psc + 1)));
    // TIM1_Generate_N_Pulses(pulses_num); // Not enabled
    printf("generate %d pulses\r\n", pulses_num);
    // Recommended 10 kHz, duty cycle resolution 0.1%
}
void bsp_blasi_pwm_init(uint8_t pulse)
{
    HAL_TIM_IC_DeInit(&htim2);
    HAL_TIM_PWM_DeInit(&htim2);
    uint16_t arr = 1050; // Period; duty cycle resolution: 1 / 1050 = 0.0952% (better than 0.1%)
    uint16_t psc = 15;   // Prescaler
    uint16_t pulses_num = 11000;
    TIM2_PWM_Init(arr, psc, pulse); // arr,psc,pulse f=168MHz/(arr+1)*(psc+1)    Max usable 28MHz: TIM1_PWM_Init(2,3), compare value set to 1, __HAL_TIM_SET_COMPARE(&htim1, LED_PWM_IN_CHANNEL, 1);;
    printf("TIM2 PWM Init with ARR=%d, PSC=%d, Pulse=%d, freq = %lu Hz\r\n", arr, psc, pulse, 168000000 / ((arr + 1) * (psc + 1)));
    // Recommended 10 kHz, duty cycle resolution 0.1%
}
void bsp_CCP_Init(void)
{
    TIM1_CCP_Init();
    MIPI_CMD_INFO("------------- bsp tim1 ccp init finish -------------\r\n");
}

/**
 * @brief Print system version information
 * @retval None
 */
static void bsp_print_version_info(void)
{
    MIPI_CMD_INFO("================================================\r\n");
    MIPI_CMD_INFO("MIPI CMD DEVICE Board System Information\r\n");
    MIPI_CMD_INFO("================================================\r\n");
    MIPI_CMD_INFO("Software Version: %d.%d.%d.%d\r\n",
                  sw_version[0], sw_version[1], sw_version[2], sw_version[3]);
    MIPI_CMD_INFO("Hardware Version: %d.%d.%d.%d\r\n",
                  hw_version[0], hw_version[1], hw_version[2], hw_version[3]);
}

/**
 * @brief Initialize ADC system (ADS1256)
 * @retval HAL_StatusTypeDef
 */
static HAL_StatusTypeDef bsp_init_adc_system(void)
{
    /* Configure ADS1256 device parameters */
    dev_vol.work_channel = 0;
    dev_vol.channel_en = 0b11111111; // sample all 8 channels
    dev_vol.auto_change_gear = 0;    /* Disable auto gear change */
#ifdef IC_POWER_BOARD_ADS1256
    dev_ic_power_board.work_channel = 0;
    dev_ic_power_board.channel_en = 0b11111111;
    dev_ic_power_board.auto_change_gear = 0;
#endif

#ifdef GEAR_FUNCTION
    /* Configure ADS1256 gear settings for 8 channels */
    for (uint8_t i = 0; i < 8; i++)
    {
        if (i == 3 || i == 4 || i == 6)
            dev_vol.sample_res_gear[i] = GEAR_mA;
    }
#endif
    /* Initialize ADS1256 */
    bsp_ads1256_init(&dev_vol);

#ifdef GEAR_FUNCTION
    /* Set sampling gear */
    bsp_select_sample_gear(dev_vol.sample_res_gear);
#endif
    HAL_NVIC_SetPriority(EXTI2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn); // ADC_DRDY_1 PA2
    MIPI_CMD_INFO("------------- bsp init ads1256 finish -------------\r\n");
    return HAL_OK;
}

// ANCHOR - FLASH STRESS TEST
static void bsp_test_spi_flash(void)
{
    MIPI_CMD_INFO("=== SPI Flash Stress Test Start ===\r\n");
    MIPI_CMD_INFO("Target: 32MB Flash, 4KB Sector-based Reliability Test\r\n");

    uint32_t test_cycles = 20;  // Number of test cycles
    uint32_t sector_count = 10; // Number of randomly selected sectors per cycle
    static uint8_t write_buf[Flash_SectorSize];
    static uint8_t read_buf[Flash_SectorSize];
    uint32_t total_errors = 0;
    uint32_t start_time = HAL_GetTick();

    for (uint32_t cycle = 1; cycle <= test_cycles; cycle++)
    {
        MIPI_CMD_INFO("Cycle [%lu/%lu] running...\r\n", cycle, test_cycles);

        for (uint32_t s = 0; s < sector_count; s++)
        {
            // Randomly pick a sector address (must be 4KB aligned)
            // Use HAL_GetTick() as a simple random source
            uint32_t random_val = (HAL_GetTick() * (s + 1) * cycle);
            uint32_t sector_addr = (random_val % (Flash_TotalSize / Flash_SectorSize)) * Flash_SectorSize;

            // 1. Erase the sector
            bsp_flash_erase_sector(sector_addr);

            // 2. Prepare a random/varying data pattern
            for (uint32_t i = 0; i < Flash_SectorSize; i++)
            {
                write_buf[i] = (uint8_t)(cycle + s + i);
            }

            // 3. Write the data
            // Note: the third parameter of bsp_flash_write is uint16_t; Flash_SectorSize is 4096 (OK)
            bsp_flash_write(write_buf, sector_addr, Flash_SectorSize);

            // 4. Read back and verify
            memset(read_buf, 0, Flash_SectorSize);
            bsp_flash_read(read_buf, sector_addr, Flash_SectorSize);

            // 5. Compare the data
            uint32_t sector_errors = 0;
            for (uint32_t i = 0; i < Flash_SectorSize; i++)
            {
                if (read_buf[i] != write_buf[i])
                {
                    sector_errors++;
                }
            }

            if (sector_errors > 0)
            {
                total_errors += sector_errors;
                MIPI_CMD_ERROR("Error at Sector 0x%08lX: %lu bytes mismatch!\r\n", sector_addr, sector_errors);
            }
        }
    }

    uint32_t end_time = HAL_GetTick();
    MIPI_CMD_INFO("=== Stress Test Finished ===\r\n");
    MIPI_CMD_INFO("Total Duration: %lu ms\r\n", end_time - start_time);
    MIPI_CMD_INFO("Total Tested Sectors: %lu\r\n", test_cycles * sector_count);
    if (total_errors == 0)
    {
        MIPI_CMD_INFO("Result: PASS (No errors detected)\r\n");
    }
    else
    {
        MIPI_CMD_ERROR("Result: FAIL (%lu total byte errors)\r\n", total_errors);
    }
}

// ANCHOR - DEMO PWM TEST FUNCTIONS
void test_pwm(void)
{
    bsp_led_pwm_init(10); // step1
    bsp_blasi_pwm_init(10);
    enableTim1PWMOutput(); // step2
    enableTim2PWMOutput();
    app_delay(5000);
    disableTim1PWMOutput();
    disableTim2PWMOutput();
    // 1% duty cycle for 5 s

    HAL_TIM_IC_DeInit(&htim1);
    HAL_TIM_PWM_DeInit(&htim1);
    HAL_TIM_IC_DeInit(&htim2);
    HAL_TIM_PWM_DeInit(&htim2);
    uint16_t arr = 1050; // Period; duty cycle resolution: 1 / 1050 = 0.0952% (better than 0.1%)
    uint16_t psc = 15;   // Prescaler
    uint16_t pulse = 30; // Compare value
    uint16_t pulses_num = 11000;
    TIM1_PWM_Init(arr, psc, pulse);
    TIM2_PWM_Init(arr, psc, pulse); // arr,psc,pulse f=168MHz/(arr+1)*(psc+1)    Max usable 28MHz: TIM1_PWM_Init(2,3), compare value set to 1, __HAL_TIM_SET_COMPARE(&htim1, LED_PWM_IN_CHANNEL, 1);;
    printf("TIM2 PWM Init with ARR=%d, PSC=%d, Pulse=%d, freq = %lu Hz\r\n", arr, psc, pulse, 168000000 / ((arr + 1) * (psc + 1)));
    enableTim1PWMOutput(); // step2
    enableTim2PWMOutput();
    app_delay(5000);
    disableTim1PWMOutput();
    disableTim2PWMOutput();
    // 3% duty cycle for 5 s

    HAL_TIM_IC_DeInit(&htim1);
    HAL_TIM_PWM_DeInit(&htim1);
    HAL_TIM_IC_DeInit(&htim2);
    HAL_TIM_PWM_DeInit(&htim2);
    arr = 1050; // Period; duty cycle resolution: 1 / 1050 = 0.0952% (better than 0.1%)
    psc = 15;   // Prescaler
    pulse = 50; // Compare value
    pulses_num = 11000;
    TIM1_PWM_Init(arr, psc, pulse);
    TIM2_PWM_Init(arr, psc, pulse); // arr,psc,pulse f=168MHz/(arr+1)*(psc+1)    Max usable 28MHz: TIM1_PWM_Init(2,3), compare value set to 1, __HAL_TIM_SET_COMPARE(&htim1, LED_PWM_IN_CHANNEL, 1);;
    printf("TIM2 PWM Init with ARR=%d, PSC=%d, Pulse=%d, freq = %lu Hz\r\n", arr, psc, pulse, 168000000 / ((arr + 1) * (psc + 1)));
    enableTim1PWMOutput(); // step2
    enableTim2PWMOutput();
    app_delay(5000);
    disableTim1PWMOutput();
    disableTim2PWMOutput();
    // 5% duty cycle for 5 s
}
// ANCHOR - DEMO CCP TEST FUNCTIONS
void test_ccp(void)
{
    bsp_CCP_Init();                      // step1
    enableTim1CaptureCompareInterrupt(); // step2
    app_delay(5000);
    disableTim1CaptureCompareInterrupt(); // step3
}

void cali_zero(void)
{
    printf("\r\n==================================================\r\n");
    printf(" Start self-test (Zero Calibration)...\r\n");
    printf(" Please wait...\r\n");
    printf("==================================================\r\n");

    uint8_t ads1256_ch_index;
    uint8_t d_trigger_ch_index;
    extern const uint8_t sample_vol_map[15][2];
    extern const uint8_t sample_cur_map[11][2];
    float gain, offset;

    uint8_t data_type = 1; // cur
    for (int8_t gear = 1; gear >= 0; gear--)
    {
        bsp_rly_gear_set_all(gear);
        for (uint8_t usr_idx = 0; usr_idx < 8; usr_idx++)
        {
            if (data_type == 0) // Voltage
            {
                ads1256_ch_index = sample_vol_map[usr_idx][0];
                d_trigger_ch_index = sample_vol_map[usr_idx][1];
            }
            else if (data_type == 1) // Current
            {
                ads1256_ch_index = sample_cur_map[usr_idx][0];
                d_trigger_ch_index = sample_cur_map[usr_idx][1];
            }
            HAL_NVIC_DisableIRQ(EXTI2_IRQn); // Temporarily disable the sampling IRQ while switching the sample channel
            if (ads1256_ch_index == 0 && d_trigger_ch_index != 0xff)
                bsp_ads1256_ch0_select(d_trigger_ch_index);
            else if (ads1256_ch_index == 1 && d_trigger_ch_index != 0xff)
                bsp_ads1256_ch1_select(d_trigger_ch_index);
            else if (ads1256_ch_index == 2 && d_trigger_ch_index != 0xff)
                bsp_ads1256_ch2_select(d_trigger_ch_index);
            HAL_NVIC_EnableIRQ(EXTI2_IRQn); // Re-enable the sampling IRQ after switching the sample channel
            if (ads1256_ch_index < 3 && d_trigger_ch_index != 0xff)
            {
                uint32_t t0 = HAL_GetTick();
                while (latest_sample_ch_sel[ads1256_ch_index] != d_trigger_ch_index)
                {
                    if ((HAL_GetTick() - t0) >= 2000U) // Wait at most 2 s
                    {
                        M_SPI_INFO("SINGLE_VOL_GET timeout\r\n");
                        break;
                    }
                    bsp_delay_ms(1);
                }
            }
            for (uint8_t i = 0; i < 8; i++)
                wait_adc_one_round(200);     // One sampling round takes about 140 ms
            HAL_NVIC_DisableIRQ(EXTI2_IRQn); // Disable the sampling IRQ while updating calibration values
            sel_cali_param(ads1256_ch_index, d_trigger_ch_index, &offset, &gain);
            offset = -latest_sample_raw_data[ads1256_ch_index] * gain;
            set_cali_param(ads1256_ch_index, d_trigger_ch_index, offset, gain);
            HAL_NVIC_EnableIRQ(EXTI2_IRQn); // Re-enable the sampling IRQ after updating calibration values
        }
    }
    bsp_rly_gear_set_all(GEAR_mA);
    printf("\r\n==================================================\r\n");
    printf(" Self-test finished. System ready.\r\n");
    printf("==================================================\r\n\r\n");
}
