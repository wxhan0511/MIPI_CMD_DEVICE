#ifndef BSP_CHANNEL_SEL_H
#define BSP_CHANNEL_SEL_H
/* Includes ------------------------------------------------------------------*/
#include "bsp.h"
#include "bsp_d_trigger.h"
/* Exported types ------------------------------------------------------------*/
typedef enum
{
    R_D = 0,
    BIAS_P,
    BIAS_1V,
    VOL,
    FREQ,
    NULL_6,
    NULL_7,
    NULL_8,
} TEST_MODE;

typedef enum
{
    BIAS_P_MODE = 0,
    BIAS_1V_MODE,
    BIAS_N_MODE,
}BIAS_MODE;

typedef enum
{
    D_MODE = 0,
    R_MODE = 1,
    R_D_MODE_NULL = 2,
}R_D_MODE;

typedef enum
{
    TEST_GROUP_1 = 0,
    TEST_GROUP_2,
    TEST_GROUP_3,
    TEST_GROUP_4,
    TEST_GROUP_5,
    TEST_GROUP_6,
    TEST_GROUP_7,
    TEST_GROUP_8,
} TEST_PIN_GROUP;

typedef enum
{
    OHM_NULL = 0,
    OHM_10_M,
    OHM_1_M,
    OHM_100_K,
    OHM_10_K,
    OHM_1_K,
    OHM_91,
    OHM_1,
} TEST_R_D_RES_LEVEL;

typedef enum
{
    // //AD_TESTV
    CUR_BIAS_N = 0,
    CUR_BIAS_P = 1,
    CUR_VDD = 2,
    CUR_IOVCC = 3,
    CUR_VCC = 4,
    CUR_VSP = 5,
    CUR_VSN = 6,
    VOL_S = 7,
}TEST_CUR_CHANNEL;

typedef enum
{
    GEAR_uA = 0,
    GEAR_mA = 1,
}TEST_CUR_GEAR;

typedef enum
{
    VOL_TEST_V_BIG = 1,
    VOL_TEST_V_SMALL = 0,
}TEST_VOL_GEAR;

typedef enum
{
    VSN_RLY = 0,
    ELVSS_RLY,
    VCC_RLY,
    IOVCC_RLY,
    VSP_RLY,
    AVDD_RLY,
    VDD_RLY,
    ELVDD_RLY,
}RLY_INDEX;

//ADS1256选通宏
typedef enum
{
    VCC_V = 0,
    IOVCC_V,
    ELVSS_V,
    VSN_V,
    VDD_V,
    ELVDD_V,
    AVDD_V,
    VSP_V,

}AI0_INDEX;
typedef enum
{
    AD_V = 0,
    ELVDD_I=2,
    ELVSS_I=3,
    AD_I_BLAS_I=4,
    AD_I_BLAS_V=5,
    AD_I_BL=6,
    AVDD_I=7,
}AI1_INDEX;
typedef enum
{
    AD_R_D = 0,
    AD_24PinV=2,
    AD_V_VBAT=3,
    AD_V_BLAS_I=4,
    BLAS_V=5,
    AD_V_BL=6,
    AD_V_N=7,
}AI2_INDEX;

extern uint8_t ch0_flag;
extern uint8_t ch1_flag;
extern uint8_t ch2_flag;

/* Exported macro ------------------------------------------------------------*/

//ANCHOR - 电源控制宏
#define VSN_ENABLE_POWEREN_N_1()    bsp_d_trigger_set_channel(&d_3, 0, 0);
#define VSN_DISABLE_POWEREN_N_1()   bsp_d_trigger_set_channel(&d_3, 0, 1);
#define ELVSS_ENABLE_POWEREN_N_2()    bsp_d_trigger_set_channel(&d_3, 1, 0);
#define ELVSS_DISABLE_POWEREN_N_2()   bsp_d_trigger_set_channel(&d_3, 1, 1);
#define ELVDD_ENABLE_POWEREN_P_6()    bsp_d_trigger_set_channel(&d_3, 2 , 1);
#define ELVDD_DISABLE_POWEREN_P_6()   bsp_d_trigger_set_channel(&d_3, 2 , 0);
#define VDD_ENABLE_POWEREN_P_5()    bsp_d_trigger_set_channel(&d_3, 3, 1);
#define VDD_DISABLE_POWEREN_P_5()   bsp_d_trigger_set_channel(&d_3, 3, 0);
#define AVDD_ENABLE_POWEREN_P_4()    bsp_d_trigger_set_channel(&d_3, 4, 1);
#define AVDD_DISABLE_POWEREN_P_4()   bsp_d_trigger_set_channel(&d_3, 4, 0);
#define VSP_ENABLE_POWEREN_P_3()    bsp_d_trigger_set_channel(&d_3, 5, 1);
#define VSP_DISABLE_POWEREN_P_3()   bsp_d_trigger_set_channel(&d_3, 5,0);
#define IOVCC_ENABLE_POWEREN_P_2()    bsp_d_trigger_set_channel(&d_3, 6, 1);
#define IOVCC_DISABLE_POWEREN_P_2()   bsp_d_trigger_set_channel(&d_3, 6, 0);
#define VCC_ENABLE_POWEREN_P_1()    bsp_d_trigger_set_channel(&d_3, 7, 1);
#define VCC_DISABLE_POWEREN_P_1()   bsp_d_trigger_set_channel(&d_3, 7, 0);
//ANCHOR - level shifter控制宏
#define LEVEL_SHIFT_ENABLE()    bsp_d_trigger_set_channel(&d_8, 4, 1);
#define LEVEL_SHIFT_DISABLE()   bsp_d_trigger_set_channel(&d_8, 4, 0);

/* Exported functions prototypes ---------------------------------------------*/

void bsp_channel_sel_init(void);
//电阻二极管
void bsp_rd_select_mode(const R_D_MODE mode);
void bsp_rd_select_r_level(const TEST_R_D_RES_LEVEL r_level);
void bsp_rd_select_pin(const uint16_t pin_p, const uint16_t pin_n);
//电压
static void bsp_vol_select_pin_8_to_1(const uint16_t pin_group);
void bsp_vol_select_pin_64_to_1(const uint16_t pin, const uint8_t en);
void bsp_bias_p_select_pin(const uint8_t group, const uint16_t pin);
void bsp_bias_n_select_pin(const uint8_t group, const uint16_t pin);
//测试模式选择
void bsp_test_select_mode(const TEST_MODE mode);
void bsp_limit_current_reset();

void bsp_rly_gear_set(TEST_CUR_GEAR gear ,RLY_INDEX rly_index);
void bsp_select_24pin_channel(uint16_t pin);
void bsp_ads1256_ch0_select(const AI0_INDEX ai0_index);
void bsp_ads1256_ch1_select(const AI1_INDEX ai1_index);
void bsp_ads1256_ch2_select(const AI2_INDEX ai2_index);
#endif //BSP_CHANNEL_SEL_H
