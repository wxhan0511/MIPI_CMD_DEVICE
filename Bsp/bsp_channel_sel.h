//
// Created by 薛斌 on 24-8-19.
//

#ifndef BSP_CHANNEL_SEL_H
#define BSP_CHANNEL_SEL_H

#include "bsp.h"

void bsp_select_sample_gear(const uint8_t *gear);

#define GEAR_GPIO_Port GPIOC
#define GEAR_CUR_ELVDD_Pin GPIO_PIN_0
#define GEAR_CUR_ELVSS_Pin GPIO_PIN_1
#define GEAR_CUR_VABT_Pin GPIO_PIN_2

//#define GEAR_FUNCTION
typedef enum
{
    GEAR_mA = 0,
    GEAR_uA = 1,
}TEST_CUR_GEAR;

typedef enum
{
    VOL_TEST_V_BIG = 1,
    VOL_TEST_V_SMALL = 0,
}TEST_VOL_GEAR;

typedef enum
{
    CUR_AD_I_VBAT = 3,
    CUR_AD_I_ELVDD = 4,
    CUR_AD_I_ELVSS = 6,
}TEST_CUR_CHANNEL;

#endif //BSP_CHANNEL_SEL_H
