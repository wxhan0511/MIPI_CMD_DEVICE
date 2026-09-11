/**
 * @file       drv_ra_device.h
 * @brief      MIPI_CMD
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

    #define RA_SGM3804_ADDRESS       0x7c  // VSP/VSN DAC
    #define RA_SGM3804_VSP_CMD       0x00
    #define RA_SGM3804_VSN_CMD       0x01
    #define RA_LP3907_1_ADDRESS      0xC2  // VCI, IOVCC regulator
    #define RA_LP3907_1_VCI_CMD      0x3A
    #define RA_LP3907_1_IOVCC_CMD    0x39
    #define RA_LP3907_2_ADDRESS      0xC0  // MVDD, VDDIO regulator (SSD2828 supply)
    #define RA_LP3907_2_MVDD_CMD     0x39
    #define RA_LP3907_2_VDDIO_CMD    0x3A
    #define RA_LP3907_BKLDOEN_CMD    0x10
    #define RA_INA3221_ADDRESS       0x80  // positive-rail current sensor
    #define RA_INA3221_resistor      (0.2 / 10)  // VCI/IOVCC/VSP sense resistor value in ohms
    #define RA_ADC121C027_ADDRESS    0xa2  // VSN current-sampling ADC
    #define RA_ADC121_resistor       0.04  // VSN sense resistor value in ohms
    #define RA_TCA9554_POWER_OFF     0x70  // sub-board power sequencing controller

    //------------------ Enum type definitions ------------------
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

//------------------ External variable declarations ------------------
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
