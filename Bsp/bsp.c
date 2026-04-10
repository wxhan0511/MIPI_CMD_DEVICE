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
#include "tim.h"
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
/* Private function prototypes */
static void bsp_print_version_info(void);
static HAL_StatusTypeDef bsp_init_adc_system(void);
static void bsp_test_spi_flash(void);
static void bsp_test_latch(void);
extern SPI_HandleTypeDef hspi2;
uint8_t id = 0x01; 

/**
 * @brief Firmware name stored in specific section
 * @note Used for firmware identification and version management
 */
__attribute__((section(".fw_name"))) const char fw_name[40] = "MIPI CMD Board v0.0.1";

/**
 * @brief Software version [Major.Minor.Patch.Build]
 */
__attribute__((section(".fw_version"))) uint8_t sw_version[4] = {0, 0, 0, 1};

/**
 * @brief Hardware name
 */
__attribute__((section(".hw_name"))) const char hw_name[40] = "MIPI CMD Board";

/**
 * @brief Hardware version [Major.Minor.Patch.Build]
 */
__attribute__((section(".hw_version"))) uint8_t hw_version[4] = {0, 0, 0, 1};

/**
 * @brief Magic number for firmware verification
 */
uint8_t magic_number[4] = {0x12, 0x34, 0xf8, 0x8f};

void bsp_init()
{
    bsp_retarget_init(&huart3);
    bsp_print_version_info();

    bsp_init_dwt();
    // TIME_DEBUG("test100: %lu ms\r\n", dwt_get_ms());
    // app_delay(100);
    // TIME_DEBUG("test100: %lu ms\r\n", dwt_get_ms());

    bsp_d_trigger_init(d_1);
    bsp_d_trigger_init(d_2);
    bsp_d_trigger_init(d_3);
    bsp_d_trigger_init(d_4);
    bsp_d_trigger_init(d_5);
    bsp_d_trigger_init(d_6);
    bsp_d_trigger_init(d_7);
    bsp_d_trigger_init(d_8);
    bsp_d_trigger_set(enabled); //验证通过

    // bsp_lcd_reset(&lcd);
    
    //bsp_test_spi_flash();
    calibration_set_defaults();
    W25Q256JVEQ_INFO("Default calibration values have been set\r\n");
    //calibration_save();
    //calibration_load();
    
    // while(1);
    // M_CS_Pin_L();
    // HAL_SPI_Transmit(&hspi2, (uint8_t*)"Hello Flash", 11, HAL_MAX_DELAY);
    // M_CS_Pin_H();
    
/*-------------ADC START---------------------------*/
    //bsp_init_adc_system();
/*-------------ADC END---------------------------*/

    //Master mode and listening are mutually exclusive 
#ifdef I2C_SLAVE_I2C2_LISTEN
    HAL_I2C_DisableListen_IT(&hi2c2);
#endif
/*-------------DAC and LIMIT CURRENT START----------*/
    
    bsp_dac_init();
    
    
/*-------------DAC and LIMIT CURRENT END------------*/
#ifdef I2C_SLAVE_I2C2_LISTEN
    HAL_I2C_EnableListen_IT(&hi2c2);
#endif
    /*-------------PWM START----------------*/
    bsp_led_pwm_init();
    test_pwm();
    /*-------------PWM END----------------*/
    /*-------------CCP START----------------*/
    //bsp_CCP_Init();
    /*-------------CCP END----------------*/
    
    //MX_USB_OTG_HS_PCD_Init();
    //MX_USB_DEVICE_Init();
    MIPI_CMD_INFO("------------- bsp init finish -------------\r\n");
    
}

void bsp_led_pwm_init(void)
{
    TIM1_PWM_Init(2,150,1);//arr,psc,pulse f=168MHz/(arry+1)*(psc+1)    最大可用28MHZ TIM1_PWM_Init(2,3),比较值设置为1,__HAL_TIM_SET_COMPARE(&htim1, LED_PWM_IN_CHANNEL, 1);;
    TIM1_Generate_N_Pulses(11000);//非使能
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
    MIPI_CMD_INFO("Firmware Name: %s\r\n", fw_name);
    MIPI_CMD_INFO("Software Version: %d.%d.%d.%d\r\n", 
                       sw_version[0], sw_version[1], sw_version[2], sw_version[3]);
    MIPI_CMD_INFO("Hardware Name: %s\r\n", hw_name);
    MIPI_CMD_INFO("Hardware Version: %d.%d.%d.%d\r\n", 
                       hw_version[0], hw_version[1], hw_version[2], hw_version[3]);
    MIPI_CMD_INFO("================================================\r\n");

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
    dev_vol.auto_change_gear = 0;   /* Disable auto gear change */
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
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);//ADC_DRDY_1 PA2
    MIPI_CMD_INFO("------------- bsp init ads1256 finish -------------\r\n");
}

//ANCHOR - FLASH STRESS TEST
static void bsp_test_spi_flash(void)
{
    MIPI_CMD_INFO("=== SPI Flash Stress Test Start ===\r\n");
    MIPI_CMD_INFO("Target: 32MB Flash, 4KB Sector-based Reliability Test\r\n");
    
    uint32_t test_cycles = 20;      // 测试循环次数
    uint32_t sector_count = 10;     // 每轮随机选择的扇区数
    static uint8_t write_buf[Flash_SectorSize];
    static uint8_t read_buf[Flash_SectorSize];
    uint32_t total_errors = 0;
    uint32_t start_time = HAL_GetTick();

    for (uint32_t cycle = 1; cycle <= test_cycles; cycle++) {
        MIPI_CMD_INFO("Cycle [%lu/%lu] running...\r\n", cycle, test_cycles);
        
        for (uint32_t s = 0; s < sector_count; s++) {
            // 随机选择一个扇区地址 (必须是 4KB 对齐)
            // 使用 HAL_GetTick() 作为简单的随机源
            uint32_t random_val = (HAL_GetTick() * (s + 1) * cycle);
            uint32_t sector_addr = (random_val % (Flash_TotalSize / Flash_SectorSize)) * Flash_SectorSize;
            
            // 1. 擦除扇区
            bsp_flash_erase_sector(sector_addr);
            
            // 2. 准备随机/变化的数据模式
            for (uint32_t i = 0; i < Flash_SectorSize; i++) {
                write_buf[i] = (uint8_t)(cycle + s + i);
            }
            
            // 3. 写入数据
            // 注意：bsp_flash_write 的第三个参数是 uint16_t, Flash_SectorSize 是 4096 (OK)
            bsp_flash_write(write_buf, sector_addr, Flash_SectorSize);
            
            // 4. 读取校验
            memset(read_buf, 0, Flash_SectorSize);
            bsp_flash_read(read_buf, sector_addr, Flash_SectorSize);
            
            // 5. 比对数据
            uint32_t sector_errors = 0;
            for (uint32_t i = 0; i < Flash_SectorSize; i++) {
                if (read_buf[i] != write_buf[i]) {
                    sector_errors++;
                }
            }
            
            if (sector_errors > 0) {
                total_errors += sector_errors;
                MIPI_CMD_ERROR("Error at Sector 0x%08lX: %lu bytes mismatch!\r\n", sector_addr, sector_errors);
            }
        }
    }

    uint32_t end_time = HAL_GetTick();
    MIPI_CMD_INFO("=== Stress Test Finished ===\r\n");
    MIPI_CMD_INFO("Total Duration: %lu ms\r\n", end_time - start_time);
    MIPI_CMD_INFO("Total Tested Sectors: %lu\r\n", test_cycles * sector_count);
    if (total_errors == 0) {
        MIPI_CMD_INFO("Result: PASS (No errors detected)\r\n");
    } else {
        MIPI_CMD_ERROR("Result: FAIL (%lu total byte errors)\r\n", total_errors);
    }
}


//ANCHOR - DEMO PWM TEST FUNCTIONS
void test_pwm(void)
{
    bsp_led_pwm_init();//step1
    enableTim1PWMOutput();//step2
    app_delay(5000);
    disableTim1PWMOutput();//step3
}
//ANCHOR - DEMO CCP TEST FUNCTIONS
void test_ccp(void)
{
    bsp_CCP_Init();//step1
    enableTim1CaptureCompareInterrupt();//step2
    app_delay(5000);
    disableTim1CaptureCompareInterrupt();//step3
}