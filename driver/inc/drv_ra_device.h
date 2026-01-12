/**
 * @file       drv_ra_device.h
 * @brief      RA_PowerEX
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-10-30
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

#ifndef __DRV_RA_DEVICE_H
#define __DRV_RA_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

    #define RA_SGM3804_add           0x7c  //VSP,VSN
    #define RA_LP3907_add_1          0xc0  //SSD2828供电MVDD,VDDIO
    #define RA_LP3907_add_2          0xc2  //VCI,IOVCC
    #define RA_INA3221_add           0x80  //正压电流检测
    #define RA_INA3221_resistor      (0.2 / 10)   //VCI，IOVCC，VSP采样电阻值，单位Ω
    #define RA_PCA9554_add           0x70<<1  //小板上电时序控制芯片
    #define RA_ADC121C027_ADDR       0xa2   //VSN电流采样ADC
    #define RA_ADC121_resistor       0.04   //VSN采样电阻值，单位Ω

    // 新增宏定义（后续补充）
    #define RA_SGM3804_ADDRESS       0x7c
    #define RA_SGM3804_VSP_CMD       0x00
    #define RA_SGM3804_VSN_CMD       0x01
    #define RA_LP3907_1_ADDRESS      0xC2
    #define RA_LP3907_1_VCI_CMD      0x3A
    #define RA_LP3907_1_IOVCC_CMD    0x39
    #define RA_LP3907_2_ADDRESS      0xC0
    #define RA_LP3907_2_MVDD_CMD     0x39
    #define RA_LP3907_2_VDDIO_CMD    0x3A
    #define RA_LP3907_BKLDOEN_CMD    0x10
    #define RA_INA3221_ADDRESS       0x80
    #define RA_ADC121C027_ADDRESS    0xa2
    #define RA_TCA9554_POWER_OFF     0x70
    #define RA_XB_ADDRESS            0x80

    #define MAX_CHANNEL_NUM          7
    #define SUB_BOARD_MAX_NUM        8

    //------------------ 枚举类型定义 ------------------
typedef enum {
    RA_POWER_ELVSS,
    RA_POWER_VCC,
    RA_POWER_ELVDD,
    RA_POWER_IOVCC_1,
    RA_POWER_MAX
} ra_power_name;

typedef enum {
    RA_POWER_VSP = 1,
    RA_POWER_VSN,
    RA_POWER_VCI,
    RA_POWER_IOVCC,
    RA_POWER_MVDD,
    RA_POWER_VDDIO,
    RA_POWER_12V
} ra_power_name_alias;

typedef enum {
    RA_ERROR = 0,
    RA_SUCCESS,
    RA_ERROR_ARG,
    RA_NULL_RESULT
} ra_status;

typedef struct {
    uint8_t (*read)(uint8_t address, uint8_t command, int16_t *data);
    uint8_t (*write)(uint8_t address, uint8_t command, uint16_t data);
} ra_ina3221_t;

typedef struct {
    uint8_t (*read)(uint8_t address, uint8_t command, uint8_t *data);
    uint8_t (*write)(uint8_t address, uint8_t command, uint8_t data);
    uint8_t (*sel)(uint8_t address, uint8_t channel, uint8_t en); // address,channel,en
} ra_tca9554_t;

typedef struct {
    uint8_t (*write)(uint8_t address, uint8_t command, uint8_t data);
} ra_sgm3804_t;

typedef struct {
    uint8_t (*read)(uint8_t address, uint8_t command, uint8_t *data);
    uint8_t (*write)(uint8_t address, uint8_t command, uint8_t data);
} ra_lp3907_t;

typedef struct {
    uint8_t (*read)(uint8_t address, uint8_t command, int16_t *data);
    uint8_t (*write)(uint8_t address, uint8_t command, uint8_t data);
} ra_adc121c027_t;

typedef struct {
    ra_tca9554_t *tca9554;
    ra_ina3221_t *ina3221;
    ra_sgm3804_t *sgm3804;
    ra_lp3907_t *lp3907;
    ra_adc121c027_t *adc121c027;
} drv_ra_dev_t;

typedef struct {
    uint8_t (*sel_sub_board)(drv_ra_dev_t *dev, uint8_t sub);
    uint8_t (*init_power_on)(drv_ra_dev_t *dev, uint8_t seq);
    uint8_t (*set_power_vol)(drv_ra_dev_t *dev, uint8_t power_name, uint8_t power_value);
    uint8_t (*read_power_cur)(drv_ra_dev_t *dev, uint8_t power_name, int32_t *power_vol_value, int32_t *power_cur_value);
    uint8_t (*set_power_en)(drv_ra_dev_t *dev, uint8_t power_name, uint8_t en);
    uint8_t (*read_power_cur_ra_power_ex)(drv_ra_dev_t *dev, uint8_t power_name, int32_t *power_vol_value, int32_t *power_cur_value);
} drv_ra_ops_t;

typedef struct {
    uint8_t main_address;
    drv_ra_dev_t *dev;
    drv_ra_ops_t *ops;
} drv_ra_t;

//------------------ 外部变量声明 ------------------
extern drv_ra_ops_t ra_sub_ops;
extern drv_ra_dev_t ra_sub_dev;
extern drv_ra_t ra_dev_main_0;
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __DRV_RA_DEVICE_H */
