#include <stdio.h>
#include <string.h>
#include "main.h"
#include "usart.h"
#include "usb_device.h"
#include "bsp.h"
#include "bsp_ads1256.h"
#include "bsp_channel_sel.h"
#include "bsp_dwt.h"
#include "bsp_mcp4728.h"
#include "bsp_spi_flash.h"
#include "bsp_calibration.h"
#include "bsp_power.h"
#include "oled.h"
#include "i2c.h"
#include "bsp_i2c_bus.h"
#include "crc.h"
#include "tim.h"
/* drivers */
#include "retarget.h"
#include "delay.h"
#include "drv_ra_device.h"
/*usb driver*/
#include "usb_otg.h"
/*utils*/
#include "utils.h"
/* Private function prototypes */
static void bsp_print_version_info(void);
static HAL_StatusTypeDef bsp_init_adc_system(void);
static void bsp_test_spi_flash(void);
static void bsp_test_latch(void);
static void bsp_init_power_control(void);
static void bsp_init_interrupts(void);
static void bsp_oe_set(uint8_t dir);
static void ra_xb_Power_Init(void);

/**
 * @brief Firmware name stored in specific section
 * @note Used for firmware identification and version management
 */
__attribute__((section(".fw_name"))) const char fw_name[40] = "MIPI CMD Board v0.0.1";

/**
 * @brief Software version [Major.Minor.Patch.Build]
 */
__attribute__((section(".fw_version"))) uint8_t sw_version[4] = {1, 0, 0, 1};

/**
 * @brief Hardware name
 */
__attribute__((section(".hw_name"))) const char hw_name[40] = "MIPI CMD Board";

/**
 * @brief Hardware version [Major.Minor.Patch.Build]
 */
__attribute__((section(".hw_version"))) uint8_t hw_version[4] = {1, 0, 0, 1};

/**
 * @brief Magic number for firmware verification
 */
uint8_t magic_number[4] = {0x12, 0x34, 0xf8, 0x8f};
// uint8_t id = 0;
// uint8_t io0 = 0;
// uint8_t io1 = 0;
// uint8_t io2 = 0;
// uint8_t io3 = 0;


/**
 * @brief BSP Board Support Package Initialization
 * @details Initialize all peripherals and function modules in sequence
 * @retval None
 */
void bsp_init()
{

    bsp_retarget_init(&huart1);
    bsp_init_dwt();
    // TIME_DEBUG("test100: %lu ms\r\n", dwt_get_ms());
    // HAL_Delay(100);
    // TIME_DEBUG("test100: %lu ms\r\n", dwt_get_ms());
    /* Print system version information */
   // bsp_print_version_info();
    
    // bsp_test_spi_flash();  //for test flash
    //calibration_load(); //If the calibration value does not exist, write the default value
    //bsp_init_power_control();  //initialize power switch

    //bsp_init_adc_system();
    //bsp_latch_init();
    //bsp_oe_set(enabled);//3 Latch and 2 level shifter
    
    //Moster mode and listening are mutually exclusive 
    HAL_I2C_DisableListen_IT(&hi2c1);
    //bsp_dac_init(&dac_dev);
    HAL_I2C_EnableListen_IT(&hi2c1);
    bsp_led_pwm_init();
    enableTim1PWMOutput();
    HAL_Delay(10);
    disableTim1PWMOutput();
    bsp_CCP_Init();
    enableTim1CaptureCompareInterrupt();
    HAL_Delay(10);
    disableTim1CaptureCompareInterrupt();
    
    //MX_USB_OTG_HS_PCD_Init();
    //MX_USB_DEVICE_Init();
    //use i2c2
    //ra_xb_Power_Init();
    RA_POWEREX_INFO("------------- bsp init finish -------------\r\n");
}

void bsp_led_pwm_init(void)
{
    TIM1_PWM_Init(2,150,1);//arr,psc f=168MHz/(arry*psc)   最大可用28MHZ TIM1_PWM_Init(2,3),比较值设置为1,__HAL_TIM_SET_COMPARE(&htim1, LED_PWM_IN_CHANNEL, 1);;
    TIM1_Generate_N_Pulses(11000);

   // RA_POWEREX_INFO("------------- bsp led pwm init finish -------------\r\n");
}
void bsp_CCP_Init(void)
{
    TIM1_CCP_Init();
    RA_POWEREX_INFO("------------- bsp tim1 ccp init finish -------------\r\n");
}
/**
 * @brief Initialize power control GPIOs and enable power rails
 * @retval None
 */
static void bsp_init_power_control(void)
{
    ELVSS_DISABLE();
    ELVDD_ENABLE();
    VCC_ENABLE();
    IOVCC_ENABLE();
    //On by default
    RA_POWEREX_INFO("------------- bsp init power control finish -------------\r\n");

}
/**
 * @brief Print system version information
 * @retval None
 */
static void bsp_print_version_info(void)
{
    RA_POWEREX_INFO("================================================\r\n");
    RA_POWEREX_INFO("MIPI CMD DEVICE Board System Information\r\n");
    RA_POWEREX_INFO("================================================\r\n");
    RA_POWEREX_INFO("Firmware Name: %s\r\n", fw_name);
    RA_POWEREX_INFO("Software Version: %d.%d.%d.%d\r\n", 
                       sw_version[0], sw_version[1], sw_version[2], sw_version[3]);
    RA_POWEREX_INFO("Hardware Name: %s\r\n", hw_name);
    RA_POWEREX_INFO("Hardware Version: %d.%d.%d.%d\r\n", 
                       hw_version[0], hw_version[1], hw_version[2], hw_version[3]);


    // 解析版本号,ID的4位由读IO决定
    // io0 = HAL_GPIO_ReadPin(ADDR_RECON_PORT,ADDR0);
    // io1 = HAL_GPIO_ReadPin(ADDR_RECON_PORT,ADDR1);
    // io2 = HAL_GPIO_ReadPin(ADDR_RECON_PORT,ADDR2);
    // io3 = HAL_GPIO_ReadPin(ADDR_RECON_PORT,ADDR3);

    // // 构造字节：将 4 个 IO 状态左移到高 4 位
    // id = (io3 << 3) | (io2 << 2) | (io1 << 1) | io0;
    // // 低 4 位清零（可选，根据需求可修改）
    // id = id << 4;
    // RA_POWEREX_INFO("ID Setting from IO Pins: %d%d%d%d => ID: 0x%02X\r\n", io3, io2, io1, io0, id);
    RA_POWEREX_INFO("================================================\r\n");

}

/**
 * @brief Initialize ADC system (ADS1256)
 * @retval HAL_StatusTypeDef
 */
static HAL_StatusTypeDef bsp_init_adc_system(void)
{
    /* Configure ADS1256 device parameters */
    dev_h_m_interface_board.work_channel = 0;
    dev_h_m_interface_board.channel_en = 0b11111111; // sample all 8 channels
    dev_h_m_interface_board.auto_change_gear = 0;   /* Disable auto gear change */
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
            dev_h_m_interface_board.sample_res_gear[i] = GEAR_mA;
    }
#endif
    /* Initialize ADS1256 */
    bsp_ads1256_init(&dev_h_m_interface_board);
#ifdef IC_POWER_BOARD_ADS1256
    bsp_ads1256_init(&dev_ic_power_board);
#endif

#ifdef GEAR_FUNCTION
    /* Set sampling gear */
    bsp_select_sample_gear(dev_h_m_interface_board.sample_res_gear);
#endif
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_SetPriority(EXTI1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);//ADC_DRDY_2 PA1 , human machine interface board
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);//ADC_DRDY_1 PA11 , IC power board
#ifdef IC_POWER_BOARD_ADS1256
    RA_POWEREX_INFO("-------------  ic power board ads1256 ready -------------\r\n");
#else
    RA_POWEREX_INFO("------------- ic power board ads1256 not init -------------\r\n");
#endif
    RA_POWEREX_INFO("------------- bsp init ads1256 finish -------------\r\n");
}

static void bsp_oe_set(uint8_t dir)
{
    if (dir)
    {
        HAL_GPIO_WritePin(OE_GPIO_Port, OE_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(OE_GPIO_Port, OE_Pin, GPIO_PIN_RESET);
    }
}

#if 0
static void ra_xb_Power_Init(void)
{   uint8_t temp;
    BSP_STATUS status;
    //Print the value set in the last configuration
    RA_POWEREX_INFO("xb_iovcc %fmv\r\n",g_calibration_manager.data.xb_iovcc_last_voltage);
    RA_POWEREX_INFO("vci %fmv\r\n",g_calibration_manager.data.vci_last_voltage);
    RA_POWEREX_INFO("vsp %fmv\r\n",g_calibration_manager.data.vsp_last_voltage);
    RA_POWEREX_INFO("vsn %fmv\r\n",g_calibration_manager.data.vsn_last_voltage);
#ifdef RA_POWERSUPPLY_FOR_IC_7272    
    //fix vsn as -5.5v,for download initial code
    // g_calibration_manager.data.vsp_last_voltage = 5900; //#1 #2屏
    g_calibration_manager.data.vsn_last_voltage = 6125; //铁哥给的屏
    RA_POWEREX_INFO("xb_vsp  fixed voltage %dmv supplies power to IC\r\n", g_calibration_manager.data.vsp_last_voltage);
#endif
    //Restore the last set voltage
    temp = float_to_uint8_round(g_calibration_manager.data.xb_iovcc_last_voltage / 100);
    status = ra_dev_main_0.ops->set_power_vol(ra_dev_main_0.dev,RA_POWER_IOVCC, temp);
    if(status != BSP_OK){
        RA_POWEREX_INFO("xb_iovcc set power failed\r\n");
    }
    temp = float_to_uint8_round(g_calibration_manager.data.vci_last_voltage / 100);
    status = ra_dev_main_0.ops->set_power_vol(ra_dev_main_0.dev,RA_POWER_VCI, temp);
    if(status != BSP_OK){
        RA_POWEREX_INFO("VCI set power failed\r\n");
    }
    temp = float_to_uint8_round(g_calibration_manager.data.vsp_last_voltage / 100);
    //status = ra_dev_main_0.ops->set_power_vol(ra_dev_main_0.dev,RA_POWER_VSP, temp);
    status = ra_dev_main_0.ops->set_power_vol(ra_dev_main_0.dev, RA_POWER_VSP, temp);
    if(status != BSP_OK){
        RA_POWEREX_INFO("VSP set power failed\r\n");
    }
    temp = float_to_uint8_round(g_calibration_manager.data.vsn_last_voltage / 100);
    status = ra_dev_main_0.ops->set_power_vol(ra_dev_main_0.dev,RA_POWER_VSN, temp);
    if(status != BSP_OK){
        RA_POWEREX_INFO("VSN set power failed\r\n");
    }
    //MVDD and VDDIO fixed and enabled , for 2828 power supply, no need to change
    status = ra_dev_main_0.ops->set_power_vol(ra_dev_main_0.dev,RA_POWER_MVDD, 12);//控2828
    if(status != BSP_OK){
        RA_POWEREX_INFO("MVDD set power failed\r\n");
    }
    status = ra_dev_main_0.ops->set_power_vol(ra_dev_main_0.dev,RA_POWER_VDDIO, 18); //控2828
    if(status != BSP_OK){
        RA_POWEREX_INFO("VDDIO set power failed\r\n");
    }
    //MVDD and VDDIO fixed and enabled , for 2828 power supply, no need to change
    ra_dev_main_0.dev->tca9554->read(RA_TCA9554_POWER_OFF,0x01,0);
    
    //VSN enable or not
    status = ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev,RA_POWER_VSN,1);
    if(status != BSP_OK){
        RA_POWEREX_INFO("[drv ra ops] main 0x%x vsn power off\r\n",ra_dev_main_0.main_address);
    }
    status = ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev,RA_POWER_12V,1);
    if(status != BSP_OK){
        RA_POWEREX_INFO("[drv ra ops] main 0x%x 12v power off\r\n",ra_dev_main_0.main_address);
    }
    //VSP enable or not
    status = ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev,RA_POWER_VSP,1);
    if(status != BSP_OK){
        RA_POWEREX_INFO("[drv ra ops] main 0x%x vsp power off\r\n",ra_dev_main_0.main_address);
    }
    RA_POWEREX_INFO("Enable all power supplies on the small board\r\n");
}
#endif
/* Optional: Flash test function (currently disabled) */
/**
 * @brief Test SPI Flash read/write/erase operations
 * @retval None
 */
static void bsp_test_spi_flash(void)
{
    RA_POWEREX_INFO("Starting SPI Flash test...\r\n");
    
    uint8_t write_data[256] = {0};
    uint8_t read_data[256] = {0};
    
    /* Initialize test data */
    for (uint32_t i = 0; i < 256; i++) {
        write_data[i] = i;
    }
    
    /* Erase chip */
    bsp_flash_erase_chip();
    printf("Flash chip erase completed\r\n");
    
    /* Read 256 bytes from SPI Flash */
    bsp_flash_read(read_data, 0x00000000, 256);
    printf("Flash read completed\r\n");
    
    /* Print read data */
    for (uint32_t i = 0; i < 256; i++) {
        printf("%02X ", read_data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\r\n");
        }
    }
    
    /* Write 256 bytes to SPI Flash */
    printf("Flash write start\r\n");
    BSP_STATUS status = bsp_flash_write(write_data, 0x00000000, 256);
    if (status != BSP_OK) {
        printf("Flash write failed\r\n");
    } else {
        printf("Flash write succeeded\r\n");
    }
    
    /* Read back and verify */
    bsp_flash_read(read_data, 0x00000000, 256);
    printf("Flash read verification completed\r\n");
    
    /* Print verification data */
    for (uint32_t i = 0; i < 256; i++) {
        printf("%02X ", read_data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\r\n");
        }
    }
}