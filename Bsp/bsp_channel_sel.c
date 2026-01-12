//
// Created by 薛斌 on 24-8-19.
//

#include "bsp_channel_sel.h"
#include <math.h>


/// 0:mA档 1:uA档
/// 电流档位选择
/// @param gear 
#ifdef GEAR_FUNCTION
void bsp_select_sample_gear(const uint8_t *gear)
{

    HAL_GPIO_WritePin(GEAR_GPIO_Port,GEAR_CUR_VABT_Pin,gear[CUR_AD_I_VBAT]);
    HAL_GPIO_WritePin(GEAR_GPIO_Port,GEAR_CUR_ELVDD_Pin,gear[CUR_AD_I_ELVDD]);
    HAL_GPIO_WritePin(GEAR_GPIO_Port,GEAR_CUR_ELVSS_Pin,gear[CUR_AD_I_ELVSS]);
    
}
#endif
