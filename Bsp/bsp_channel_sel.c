//
// Created by 薛斌 on 24-8-19.
//

#include "bsp_channel_sel.h"

#include <math.h>


#include "bsp_dwt.h"
#include "task_sample.h"
extern uint8_t freq_ch_vol_check_flag;
//aix 选通 num chx_flag通道
uint8_t ch0_flag = 8;
uint8_t ch1_flag = 8;
uint8_t ch2_flag = 8;

const static uint8_t truth_table[8][3] = {
    {0, 0, 0}, // A2 A1 A0
    {0, 0, 1},
    {0, 1, 0},
    {0, 1, 1},
    {1, 0, 0},
    {1, 0, 1},
    {1, 1, 0},
    {1, 1, 1},
};
// ANCHOR - 8个d触发器通道选择初始化为全低
void bsp_channel_sel_init(void)
{
    for (int i = 0; i < 8; i++)
        bsp_d_trigger_set_channel(&d_1, i, 0);

    for (int i = 0; i < 8; i++)
        bsp_d_trigger_set_channel(&d_2, i, 0);

    for (int i = 0; i < 8; i++)
        bsp_d_trigger_set_channel(&d_3, i, 0);

    for (int i = 0; i < 8; i++)
        bsp_d_trigger_set_channel(&d_4, i, 0);

    for (int i = 0; i < 8; i++)
        bsp_d_trigger_set_channel(&d_5, i, 0);

    for (int i = 0; i < 8; i++)
        bsp_d_trigger_set_channel(&d_6, i, 0);

    for (int i = 0; i < 8; i++)
        bsp_d_trigger_set_channel(&d_7, i, 0);

    for (int i = 0; i < 8; i++)
        bsp_d_trigger_set_channel(&d_8, i, 0);
}

// ANCHOR -  选通二极管或电阻模式
//  datasheet:SW_R&D
//  @param mode mode = 0 为 二极管 ，1为电阻
void bsp_rd_select_mode(const R_D_MODE mode)
{
    bsp_d_trigger_set_channel(&d_7, 5, mode);
}
//ANCHOR - 电流档位设置
// @param gear 0: mA档，1: uA档
// @param rly_index 继电器索引，0-7对应8个继电器
void bsp_rly_gear_set(TEST_CUR_GEAR gear ,RLY_INDEX rly_index)
{
    bsp_d_trigger_set_channel(&d_2, rly_index, gear);
}

// ANCHOR -  选通电阻级别
//  @param r_level 电阻级别
void bsp_rd_select_r_level(const TEST_R_D_RES_LEVEL r_level)
{
    if (r_level > OHM_NULL && r_level <= OHM_1)
    {
        bsp_d_trigger_set_channel(&d_7, 0, truth_table[r_level][2]); // A0
        bsp_d_trigger_set_channel(&d_7, 1, truth_table[r_level][1]); // A1
        bsp_d_trigger_set_channel(&d_7, 2, truth_table[r_level][0]); // A2
        bsp_d_trigger_set_channel(&d_7, 3, 1);                       // EN
    }
}

// ANCHOR -  ADS1256 AI0选通采样通道
//  @param power_index 电源索引
void bsp_ads1256_ch0_select(const AI0_INDEX ai0_index)
{
    if (ai0_index >= 0 && ai0_index < 8)
    {
        bsp_d_trigger_set_channel(&d_1, 7, truth_table[ai0_index][2]); // A0
        bsp_d_trigger_set_channel(&d_7, 0, truth_table[ai0_index][1]); // A1
        bsp_d_trigger_set_channel(&d_7, 1, truth_table[ai0_index][0]); // A2
                              // EN常高
        ch0_flag = ai0_index;
    }
}
// ANCHOR -  ADS1256 AI1选通采样通道
//  @param power_index 电源索引
void bsp_ads1256_ch1_select(const AI1_INDEX ai1_index)
{
    if (ai1_index >= 0 && ai1_index < 8&& ai1_index != 1)
    {
        bsp_d_trigger_set_channel(&d_4, 2, truth_table[ai1_index][2]); // A0
        bsp_d_trigger_set_channel(&d_4, 3, truth_table[ai1_index][1]); // A1
        bsp_d_trigger_set_channel(&d_4, 4, truth_table[ai1_index][0]); // A2
                              // EN常高
        ch1_flag = ai1_index;
    }
}
/*清故障标志*/
void bsp_limit_current_reset()
{
    bsp_d_trigger_set_channel(&d_5, 0, 0);
    bsp_delay_us(5);
    bsp_d_trigger_set_channel(&d_5, 0, 1);
}
// ANCHOR -  ADS1256 AI2选通采样通道
//  @param power_index 电源索引
void bsp_ads1256_ch2_select(const AI2_INDEX ai2_index)
{
    if (ai2_index >= 0 && ai2_index < 8&& ai2_index != 1)
    {
        bsp_d_trigger_set_channel(&d_4, 5, truth_table[ai2_index][2]); // A0
        bsp_d_trigger_set_channel(&d_4, 6, truth_table[ai2_index][1]); // A1
        bsp_d_trigger_set_channel(&d_4, 7, truth_table[ai2_index][0]); // A2
                              // EN常高
        ch2_flag = ai2_index;
    }
}

// ANCHOR -  选通64×64条测试路径,pin_p接V_R&D,pin_n接地
//  @param pin_p 正极通道 0-63
//  @param pin_n 负极通道 0-63
void bsp_rd_select_pin(const uint16_t pin_p, const uint16_t pin_n)
{
    const uint16_t _pin_p_group = pin_p / 8;
    const uint16_t _pin_p = pin_p % 8;
    const uint16_t _pin_n_group = pin_n / 8;
    const uint16_t _pin_n = pin_n % 8;
    // printf("p group = %u p pin %u : n group = %u n pin %u \r\n ", _pin_p_group,_pin_p,_pin_n_group,_pin_n);
    if (_pin_p_group == 0)
    {
        bsp_d_trigger_set_channel(&d_4, 2, truth_table[_pin_p][2]); // R&D+A0  64pin分8组,该组的pin几打开几通道连接V_r&D
        bsp_d_trigger_set_channel(&d_4, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_4, 4, truth_table[_pin_p][0]); // R&D+A2

        bsp_d_trigger_set_channel(&d_5, 0, 1); // R&D+ EN1
    }
    else if (_pin_p_group == 1)
    {
        bsp_d_trigger_set_channel(&d_4, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_4, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_4, 4, truth_table[_pin_p][0]); // R&D+A2

        bsp_d_trigger_set_channel(&d_5, 1, 1); // R&D+ EN2
    }
    else if (_pin_p_group == 2)
    {
        bsp_d_trigger_set_channel(&d_4, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_4, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_4, 4, truth_table[_pin_p][0]); // R&D+A2

        bsp_d_trigger_set_channel(&d_5, 2, 1); // R&D+ EN3
    }
    else if (_pin_p_group == 3)
    {
        bsp_d_trigger_set_channel(&d_4, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_4, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_4, 4, truth_table[_pin_p][0]); // R&D+A2

        bsp_d_trigger_set_channel(&d_5, 3, 1); // R&D+ EN4
    }
    else if (_pin_p_group == 4)
    {
        bsp_d_trigger_set_channel(&d_4, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_4, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_4, 4, truth_table[_pin_p][0]); // R&D+A2

        bsp_d_trigger_set_channel(&d_5, 4, 1); // R&D+ EN5
    }
    else if (_pin_p_group == 5)
    {
        bsp_d_trigger_set_channel(&d_4, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_4, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_4, 4, truth_table[_pin_p][0]); // R&D+A2

        bsp_d_trigger_set_channel(&d_5, 5, 1); // R&D+ EN6
    }
    else if (_pin_p_group == 6)
    {
        bsp_d_trigger_set_channel(&d_4, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_4, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_4, 4, truth_table[_pin_p][0]); // R&D+A2

        bsp_d_trigger_set_channel(&d_5, 6, 1); // R&D+ EN7
    }
    else if (_pin_p_group == 7)
    {
        bsp_d_trigger_set_channel(&d_4, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_4, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_4, 4, truth_table[_pin_p][0]); // R&D+A2

        bsp_d_trigger_set_channel(&d_5, 7, 1); // R&D+ EN8
    }

    if (_pin_n_group == 0)
    {
        bsp_d_trigger_set_channel(&d_4, 5, truth_table[_pin_p][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_4, 6, truth_table[_pin_p][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_4, 7, truth_table[_pin_p][0]); // R&D-A2

        bsp_d_trigger_set_channel(&d_6, 0, 1); // R&D- EN1
    }
    else if (_pin_n_group == 1)
    {
        bsp_d_trigger_set_channel(&d_4, 5, truth_table[_pin_p][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_4, 6, truth_table[_pin_p][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_4, 7, truth_table[_pin_p][0]); // R&D-A2

        bsp_d_trigger_set_channel(&d_6, 1, 1); // R&D- EN2
    }
    else if (_pin_n_group == 2)
    {
        bsp_d_trigger_set_channel(&d_4, 5, truth_table[_pin_p][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_4, 6, truth_table[_pin_p][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_4, 7, truth_table[_pin_p][0]); // R&D-A2

        bsp_d_trigger_set_channel(&d_6, 2, 1); // R&D- EN3
    }
    else if (_pin_n_group == 3)
    {
        bsp_d_trigger_set_channel(&d_4, 5, truth_table[_pin_p][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_4, 6, truth_table[_pin_p][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_4, 7, truth_table[_pin_p][0]); // R&D-A2

        bsp_d_trigger_set_channel(&d_6, 3, 1); // R&D- EN4
    }
    else if (_pin_n_group == 4)
    {
        bsp_d_trigger_set_channel(&d_4, 5, truth_table[_pin_p][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_4, 6, truth_table[_pin_p][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_4, 7, truth_table[_pin_p][0]); // R&D-A2

        bsp_d_trigger_set_channel(&d_6, 4, 1); // R&D- EN5
    }
    else if (_pin_n_group == 5)
    {
        bsp_d_trigger_set_channel(&d_4, 5, truth_table[_pin_p][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_4, 6, truth_table[_pin_p][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_4, 7, truth_table[_pin_p][0]); // R&D-A2

        bsp_d_trigger_set_channel(&d_6, 5, 1); // R&D- EN6
    }
    else if (_pin_n_group == 6)
    {
        bsp_d_trigger_set_channel(&d_4, 5, truth_table[_pin_p][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_4, 6, truth_table[_pin_p][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_4, 7, truth_table[_pin_p][0]); // R&D-A2

        bsp_d_trigger_set_channel(&d_6, 6, 1); // R&D- EN7
    }
    else if (_pin_n_group == 7)
    {
        bsp_d_trigger_set_channel(&d_4, 5, truth_table[_pin_p][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_4, 6, truth_table[_pin_p][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_4, 7, truth_table[_pin_p][0]); // R&D-A2

        bsp_d_trigger_set_channel(&d_6, 7, 1); // R&D- EN8
    }
}
#if 0
//电压测量次级8选1
/// 电压模式8选1
/// @param pin_group 0-7
static void bsp_vol_select_pin_8_to_1(const uint16_t pin_group)
{
    bsp_d_trigger_set_channel(&d_15, 4, truth_table[pin_group][2]); //A0
    bsp_d_trigger_set_channel(&d_15, 5, truth_table[pin_group][1]); //A1
    bsp_d_trigger_set_channel(&d_15, 6, truth_table[pin_group][0]); //A2
    bsp_d_trigger_set_channel(&d_15, 7, 1); //EN
}

//电压测量末级8选1
/// 电压模式64选1
/// @param pin 电压测试pin 0-63
/// @param en 电压测试en
void bsp_vol_select_pin_64_to_1(const uint16_t pin, const uint8_t en)
{
    const uint16_t _pin_group = pin / 8;
    const uint16_t _pin = pin % 8;

    if (_pin_group == 0)
    {
        bsp_d_trigger_set_channel(&d_1, 7, truth_table[_pin][2]); //A2
        bsp_d_trigger_set_channel(&d_1, 6, truth_table[_pin][1]); //A1
        bsp_d_trigger_set_channel(&d_1, 5, truth_table[_pin][0]); //A0
        bsp_d_trigger_set_channel(&d_1, 4, 1); //EN
    }
    else if (_pin_group == 1)
    {
        bsp_d_trigger_set_channel(&d_1, 3, truth_table[_pin][2]); //A2
        bsp_d_trigger_set_channel(&d_1, 2, truth_table[_pin][1]); //A1
        bsp_d_trigger_set_channel(&d_1, 1, truth_table[_pin][0]); //A0
        bsp_d_trigger_set_channel(&d_1, 0, 1); //EN
    }
    else if (_pin_group == 2)
    {
        bsp_d_trigger_set_channel(&d_2, 7, truth_table[_pin][2]); //A0
        bsp_d_trigger_set_channel(&d_2, 6, truth_table[_pin][1]); //A1
        bsp_d_trigger_set_channel(&d_2, 5, truth_table[_pin][0]); //A2
        bsp_d_trigger_set_channel(&d_2, 4, 1); //EN
    }
    else if (_pin_group == 3)
    {
        bsp_d_trigger_set_channel(&d_2, 3, truth_table[_pin][2]); //A0
        bsp_d_trigger_set_channel(&d_2, 2, truth_table[_pin][1]); //A1
        bsp_d_trigger_set_channel(&d_2, 1, truth_table[_pin][0]); //A2
        bsp_d_trigger_set_channel(&d_2, 0, 1); //EN
    }
    else if (_pin_group == 4)
    {
        bsp_d_trigger_set_channel(&d_3, 7, truth_table[_pin][2]); //A0
        bsp_d_trigger_set_channel(&d_3, 6, truth_table[_pin][1]); //A1
        bsp_d_trigger_set_channel(&d_3, 5, truth_table[_pin][0]); //A2
        bsp_d_trigger_set_channel(&d_3, 4, 1); //EN
    }
    else if (_pin_group == 5)
    {
        bsp_d_trigger_set_channel(&d_3, 3, truth_table[_pin][2]); //A0
        bsp_d_trigger_set_channel(&d_3, 2, truth_table[_pin][1]); //A1
        bsp_d_trigger_set_channel(&d_3, 1, truth_table[_pin][0]); //A2
        bsp_d_trigger_set_channel(&d_3, 0, 1); //EN
    }
    else if (_pin_group == 6)
    {
        bsp_d_trigger_set_channel(&d_4, 7, truth_table[_pin][0]); //A0
        bsp_d_trigger_set_channel(&d_4, 6, truth_table[_pin][1]); //A1
        bsp_d_trigger_set_channel(&d_4, 5, truth_table[_pin][2]); //A2
        bsp_d_trigger_set_channel(&d_4, 4, 1); //EN
    }
    else if (_pin_group == 7)
    {
        bsp_d_trigger_set_channel(&d_4, 3, truth_table[_pin][0]); //A0
        bsp_d_trigger_set_channel(&d_4, 2, truth_table[_pin][1]); //A1
        bsp_d_trigger_set_channel(&d_4, 1, truth_table[_pin][2]); //A2
        bsp_d_trigger_set_channel(&d_4, 0, 1); //EN
    }

    if (en == 1)
        bsp_vol_select_pin_8_to_1(_pin_group);
}
#endif
#if 0
// discarded

/// 外灌正电压通道选择
/// @param group
/// 0: 40-63pin
/// 1:0-63 pin
/// 2:电源 vsn vsp vcc iovcc
/// @param pin
/// 40-63
/// T++ 0-63
/// 电源 vsn vsp vcc iovcc
void bsp_bias_p_select_pin(const uint8_t group, const uint16_t pin)
{
    if (group == 0)
    {
        if (pin < 40)
        {
            printf("pin out of range \r\n");
            return;
        }
        const uint16_t new_pin = pin - 40;
        const uint16_t _pin_group = new_pin / 8;
        const uint16_t _pin = new_pin % 8;
        //printf("pin map %d %d\r\n", _pin_group, _pin);

        if (_pin_group == 0)
        {
            bsp_d_trigger_set_channel(&d_13, 3, truth_table[_pin][2]);  //TEST_V1 A0
            bsp_d_trigger_set_channel(&d_13, 2, truth_table[_pin][1]);  //TEST_V2 A1
            bsp_d_trigger_set_channel(&d_13, 1, truth_table[_pin][0]);  //TEST_V3 A2
            bsp_d_trigger_set_channel(&d_13, 0, 1);                     //TEST_V4 EN

            bsp_d_trigger_set_channel(&d_15, 0, truth_table[0][2]); //TEST_V13 A0
            bsp_d_trigger_set_channel(&d_15, 1, truth_table[0][1]); //TEST_V14 A1
            bsp_d_trigger_set_channel(&d_15, 2, truth_table[0][0]); //TEST_V15 A2
            bsp_d_trigger_set_channel(&d_15, 3, 1); //TEST_V16 EN
        }
        else if (_pin_group == 1)
        {
            bsp_d_trigger_set_channel(&d_14, 0, truth_table[_pin][2]); //TEST_V5 A0
            bsp_d_trigger_set_channel(&d_14, 1, truth_table[_pin][1]); //TEST_V6 A1
            bsp_d_trigger_set_channel(&d_14, 2, truth_table[_pin][0]); //TEST_V7 A2
            bsp_d_trigger_set_channel(&d_14, 3, 1); //TEST_V8 EN

            bsp_d_trigger_set_channel(&d_15, 0, truth_table[1][2]); //TEST_V13 A0
            bsp_d_trigger_set_channel(&d_15, 1, truth_table[1][1]); //TEST_V14 A1
            bsp_d_trigger_set_channel(&d_15, 2, truth_table[1][0]); //TEST_V15 A2
            bsp_d_trigger_set_channel(&d_15, 3, 1); //TEST_V16 EN
        }
        else if (_pin_group == 2)
        {
            bsp_d_trigger_set_channel(&d_14, 4, truth_table[_pin][2]); //TEST_V09 A0
            bsp_d_trigger_set_channel(&d_14, 5, truth_table[_pin][1]); //TEST_V10 A1
            bsp_d_trigger_set_channel(&d_14, 6, truth_table[_pin][0]); //TEST_V11 A2
            bsp_d_trigger_set_channel(&d_14, 7, 1); //TEST_V12 EN

            bsp_d_trigger_set_channel(&d_15, 0, truth_table[2][2]); //TEST_V13 A0
            bsp_d_trigger_set_channel(&d_15, 1, truth_table[2][1]); //TEST_V14 A1
            bsp_d_trigger_set_channel(&d_15, 2, truth_table[2][0]); //TEST_V15 A2
            bsp_d_trigger_set_channel(&d_15, 3, 1); //TEST_V16 EN
        }
    }
    else if (group == 1)
    {
        //T++
        bsp_vol_select_pin_64_to_1(pin, 1);

        //末级8选1
        bsp_d_trigger_set_channel(&d_15, 0, truth_table[3][2]); //TEST_V13 A0
        bsp_d_trigger_set_channel(&d_15, 1, truth_table[3][1]); //TEST_V14 A1
        bsp_d_trigger_set_channel(&d_15, 2, truth_table[3][0]); //TEST_V15 A2
        bsp_d_trigger_set_channel(&d_15, 3, 1); //TEST_V16 EN
    }
    else if (group == 2)
    {
        if (pin == 0)
        {
            //TE1_0
            bsp_d_trigger_set_channel(&d_15, 0, truth_table[4][2]); //TEST_V13 A0
            bsp_d_trigger_set_channel(&d_15, 1, truth_table[4][1]); //TEST_V14 A1
            bsp_d_trigger_set_channel(&d_15, 2, truth_table[4][0]); //TEST_V15 A2
            bsp_d_trigger_set_channel(&d_15, 3, 1); //TEST_V16 EN
        }
        else if (pin == 1)
        {
            //TE1_1
            bsp_d_trigger_set_channel(&d_15, 0, truth_table[5][2]); //TEST_V13 A0
            bsp_d_trigger_set_channel(&d_15, 1, truth_table[5][1]); //TEST_V14 A1
            bsp_d_trigger_set_channel(&d_15, 2, truth_table[5][0]); //TEST_V15 A2
            bsp_d_trigger_set_channel(&d_15, 3, 1); //TEST_V16 EN
        }
        else if (pin == 2)
        {
            //TE1_2
            bsp_d_trigger_set_channel(&d_15, 0, truth_table[6][2]); //TEST_V13 A0
            bsp_d_trigger_set_channel(&d_15, 1, truth_table[6][1]); //TEST_V14 A1
            bsp_d_trigger_set_channel(&d_15, 2, truth_table[6][0]); //TEST_V15 A2
            bsp_d_trigger_set_channel(&d_15, 3, 1); //TEST_V16 EN
        }
        else if (pin == 3)
        {
            //TE1_3
            bsp_d_trigger_set_channel(&d_15, 0, truth_table[7][2]); //TEST_V13 A0
            bsp_d_trigger_set_channel(&d_15, 1, truth_table[7][1]); //TEST_V14 A1
            bsp_d_trigger_set_channel(&d_15, 2, truth_table[7][0]); //TEST_V15 A2
            bsp_d_trigger_set_channel(&d_15, 3, 1); //TEST_V16 EN
        }
    }
}

/// 外灌负电压通道选择
/// @param group
/// @param pin 40-63 T++ 104 电源 111 118 125 132
void bsp_bias_n_select_pin(const uint8_t group, const uint16_t pin)
{
    if (group == 0)
    {
        if (pin < 40)
        {
            printf("pin out of range \r\n");
            return;
        }
        const uint16_t new_pin = pin - 40;
        const uint16_t _pin_group = new_pin / 8;
        const uint16_t _pin = new_pin % 8;
        printf("n pin map %d %d\r\n", _pin_group, _pin);

        if (_pin_group == 0)
        {
            bsp_d_trigger_set_channel(&d_16, 0, truth_table[_pin][2]);  //S_BIAS1 A0
            bsp_d_trigger_set_channel(&d_16, 1, truth_table[_pin][1]);  //S_BIAS2 A1
            bsp_d_trigger_set_channel(&d_16, 2, truth_table[_pin][0]);  //S_BIAS3 A2
            bsp_d_trigger_set_channel(&d_16, 3, 1);                     //S_BIAS4 EN

            bsp_d_trigger_set_channel(&d_17, 4, truth_table[0][2]);  //S_BIAS13 A0
            bsp_d_trigger_set_channel(&d_17, 5, truth_table[0][1]);  //S_BIAS14 A1
            bsp_d_trigger_set_channel(&d_17, 6, truth_table[0][0]);  //S_BIAS15 A2
            bsp_d_trigger_set_channel(&d_17, 7, 1);                     //S_BIAS16 EN
        }
        else if (_pin_group == 1)
        {
            bsp_d_trigger_set_channel(&d_16, 4, truth_table[_pin][2]);  //S_BIAS5 A0
            bsp_d_trigger_set_channel(&d_16, 5, truth_table[_pin][1]);  //S_BIAS6 A1
            bsp_d_trigger_set_channel(&d_16, 6, truth_table[_pin][0]);  //S_BIAS7 A2
            bsp_d_trigger_set_channel(&d_16, 7, 1);                     //S_BIAS8 EN

            bsp_d_trigger_set_channel(&d_17, 4, truth_table[1][2]);  //S_BIAS13 A0
            bsp_d_trigger_set_channel(&d_17, 5, truth_table[1][1]);  //S_BIAS14 A1
            bsp_d_trigger_set_channel(&d_17, 6, truth_table[1][0]);  //S_BIAS15 A2
            bsp_d_trigger_set_channel(&d_17, 7, 1);                     //S_BIAS16 EN
        }
        else if (_pin_group == 2)
        {
            bsp_d_trigger_set_channel(&d_17, 0, truth_table[_pin][2]);  //S_BIAS09 A0
            bsp_d_trigger_set_channel(&d_17, 1, truth_table[_pin][1]);  //S_BIAS10 A1
            bsp_d_trigger_set_channel(&d_17, 2, truth_table[_pin][0]);  //S_BIAS11 A2
            bsp_d_trigger_set_channel(&d_17, 3, 1);                     //S_BIAS12 EN

            bsp_d_trigger_set_channel(&d_17, 4, truth_table[2][2]);  //S_BIAS13 A0
            bsp_d_trigger_set_channel(&d_17, 5, truth_table[2][1]);  //S_BIAS14 A1
            bsp_d_trigger_set_channel(&d_17, 6, truth_table[2][0]);  //S_BIAS15 A2
            bsp_d_trigger_set_channel(&d_17, 7, 1);                     //S_BIAS16 EN
        }
    }
    else if (group == 1)
    {
        //T++
        bsp_vol_select_pin_64_to_1(pin, 1);

        //末级8选1
        bsp_d_trigger_set_channel(&d_17, 4, truth_table[3][0]);  //S_BIAS13 A0
        bsp_d_trigger_set_channel(&d_17, 5, truth_table[3][1]);  //S_BIAS14 A1
        bsp_d_trigger_set_channel(&d_17, 6, truth_table[3][2]);  //S_BIAS15 A2
        bsp_d_trigger_set_channel(&d_17, 7, 1);                  //S_BIAS16 EN
    }
    else if (group == 2)
    {
        if (pin == 0)
        {
            //TE1_0
            bsp_d_trigger_set_channel(&d_17, 4, truth_table[4][2]);  //S_BIAS13 A0
            bsp_d_trigger_set_channel(&d_17, 5, truth_table[4][1]);  //S_BIAS14 A1
            bsp_d_trigger_set_channel(&d_17, 6, truth_table[4][0]);  //S_BIAS15 A2
            bsp_d_trigger_set_channel(&d_17, 7, 1);                  //S_BIAS16 EN
        }
        else if (pin == 1)
        {
            //TE1_1
            bsp_d_trigger_set_channel(&d_17, 4, truth_table[5][2]);  //S_BIAS13 A0
            bsp_d_trigger_set_channel(&d_17, 5, truth_table[5][1]);  //S_BIAS14 A1
            bsp_d_trigger_set_channel(&d_17, 6, truth_table[5][0]);  //S_BIAS15 A2
            bsp_d_trigger_set_channel(&d_17, 7, 1);                  //S_BIAS16 EN
        }
        else if (pin == 2)
        {
            //TE1_2
            bsp_d_trigger_set_channel(&d_17, 4, truth_table[6][2]);  //S_BIAS13 A0
            bsp_d_trigger_set_channel(&d_17, 5, truth_table[6][1]);  //S_BIAS14 A1
            bsp_d_trigger_set_channel(&d_17, 6, truth_table[6][0]);  //S_BIAS15 A2
            bsp_d_trigger_set_channel(&d_17, 7, 1);                  //S_BIAS16 EN
        }
        else if (pin == 3)
        {
            //TE1_3
            bsp_d_trigger_set_channel(&d_17, 4, truth_table[7][2]);  //S_BIAS13 A0
            bsp_d_trigger_set_channel(&d_17, 5, truth_table[7][1]);  //S_BIAS14 A1
            bsp_d_trigger_set_channel(&d_17, 6, truth_table[7][0]);  //S_BIAS15 A2
            bsp_d_trigger_set_channel(&d_17, 7, 1);                  //S_BIAS16 EN
        }
    }
}
#endif
#if 0
/// 选择测试模式
/// @param mode BIAS_P BIAS_N VOL FREQ
void bsp_test_select_mode(const TEST_MODE mode)
{
    if (mode >= BIAS_P && mode <= FREQ)
    {
        bsp_d_trigger_set_channel(&d_18, 2, 0); //EN
        bsp_d_trigger_set_channel(&d_18, 7, 0); //EN
        bsp_d_trigger_set_channel(&d_18, 6, 0); //EN
        bsp_d_trigger_set_channel(&d_18, 5, truth_table[mode][2]); //A0
        bsp_d_trigger_set_channel(&d_18, 4, truth_table[mode][1]); //A1

        bsp_d_trigger_set_channel(&d_18, 3, truth_table[mode][0]); //A2
        bsp_d_trigger_set_channel(&d_18, 2, 1); //EN
        bsp_d_trigger_set_channel(&d_18, 1, 0); //EN
        //bsp_d_trigger_set_channel(&d_18, 0, 0); //EN
    }
    else
    {
        printf("out of range \r\n");
    }
}
#endif

//ANCHOR -  频率测试选通
//datasheet:408EN1~4
//@param pin 0~23
//NOTE - 开启频率测试前，先对选择通道执行电压测量。电压为0~5V才可EN，否则有概率损坏保护器件 -比较器的Ref_Freq 需要设置
void bsp_select_freq_channel(uint16_t pin)
{

    if(!freq_ch_vol_check_flag)
    {
        printf("freq channel vol exceed +-5V, can not enable freq channel\r\n");
        return;
    }
    freq_ch_vol_check_flag = 0;
    uint16_t _pin_p_group = pin / 8;
    uint16_t _pin_p = pin % 8;
    //only one freq channel enable
    bsp_d_trigger_set_channel(&d_1, 3, 0); // 408EN1
    bsp_d_trigger_set_channel(&d_1, 4, 0); // 408EN2
    bsp_d_trigger_set_channel(&d_1, 5, 0); // 408EN3
    if(_pin_p_group == 0&&_pin_p > 3) 
    {
        _pin_p_group = 1;
        _pin_p = _pin_p - 4;
    }
    if(_pin_p_group == 1&&_pin_p < 4) 
    {
        _pin_p_group = 0;
        _pin_p = _pin_p + 4;
    }
    if (pin >= 0 && pin <= 23&& _pin_p_group <=2)
    {
        bsp_d_trigger_set_channel(&d_1, 0, truth_table[_pin_p][2]); // 408A0  24pin分8组,该组的pin几打开几通道连接
        bsp_d_trigger_set_channel(&d_1, 1, truth_table[_pin_p][1]); // 408A1
        bsp_d_trigger_set_channel(&d_1, 2, truth_table[_pin_p][0]); // 408A2
        if (_pin_p_group == 0)  bsp_d_trigger_set_channel(&d_1, 3, 1); // 408EN1
        else if (_pin_p_group == 1) bsp_d_trigger_set_channel(&d_1, 4, 1); // 408EN2
        else if (_pin_p_group == 2) bsp_d_trigger_set_channel(&d_1, 5, 1); // 408EN3
    }
    else
    {
        printf("pin or pin_group out of range \r\n");
    }
}
#if 0
///
/// @param enable
void bsp_select_pwm_mode(const uint8_t enable)
{
    bsp_d_trigger_set_channel(&d_18, 1, enable);
}

///
/// @param enable
void bsp_select_freq_mode(const uint8_t enable)
{
    bsp_d_trigger_set_channel(&d_18, 7, enable);
}

/// 电压电流档位选择
/// @param pin
/// @param enable 0：大档 1：小档
void bsp_select_vol_cur_gear(const TEST_CUR_CHANNEL pin, const uint8_t enable)
{
    printf("[bsp change gear] channel %d gear %d\r\n", pin, enable);
    cfg_set_power_cur_gear(pin, enable);
    bsp_d_trigger_set_channel(&d_19, CUR_BIAS_P, dev_cur.sample_res_gear[7]);
    bsp_d_trigger_set_channel(&d_19, CUR_BIAS_N, dev_cur.sample_res_gear[6]);
    bsp_d_trigger_set_channel(&d_19, CUR_VDD, dev_cur.sample_res_gear[4]);
    bsp_d_trigger_set_channel(&d_19, CUR_IOVCC, dev_cur.sample_res_gear[3]);
    bsp_d_trigger_set_channel(&d_19, CUR_VCC, dev_cur.sample_res_gear[2]);
    bsp_d_trigger_set_channel(&d_19, CUR_VSP, dev_cur.sample_res_gear[1]);
    bsp_d_trigger_set_channel(&d_19, CUR_VSN, dev_cur.sample_res_gear[0]);
}

/// 电压电流档位选择
/// @param gear bias - bias+ vdd iovcc vcc vsp vsn
void bsp_select_sample_gear(const uint8_t *gear)
{
    bsp_d_trigger_set_channel(&d_19, CUR_BIAS_P, dev_cur.sample_res_gear[7]);
    bsp_d_trigger_set_channel(&d_19, CUR_BIAS_N, dev_cur.sample_res_gear[6]);
    bsp_d_trigger_set_channel(&d_19, CUR_VDD, dev_cur.sample_res_gear[4]);
    bsp_d_trigger_set_channel(&d_19, CUR_IOVCC, dev_cur.sample_res_gear[3]);
    bsp_d_trigger_set_channel(&d_19, CUR_VCC, dev_cur.sample_res_gear[2]);
    bsp_d_trigger_set_channel(&d_19, CUR_VSP, dev_cur.sample_res_gear[1]);
    bsp_d_trigger_set_channel(&d_19, CUR_VSN, dev_cur.sample_res_gear[0]);
}

/// 频率采样档位选择
/// @param level 小档 0 (0-6V) 大档 1 (0-21V)
void bsp_test_select_freq_level(const uint8_t level)
{
    bsp_d_trigger_set_channel(&d_18, 1, level); // A0
}

///
/// @param mode 0 bias+ 1 bias +1V 2:bias-
/// @param p_vol
/// @param p_group 0: 40-63pin 1:0-63 pin 2:电源 vsn vsp vcc iovcc
/// @param p_pin
/// @param n_vol
/// @param n_group
/// @param n_pin
void bsp_bias_vol_output(
    const BIAS_MODE mode, float p_vol, const uint8_t p_group, const uint8_t p_pin,
    const float n_vol, const uint8_t n_group, const uint8_t n_pin)
{
    if (mode == BIAS_P_MODE)
    {
        const uint16_t v_dac = (uint16_t)bsp_dac_convert_bias_to_dac(p_vol);
        printf("!!! +  %d %f \r\n", v_dac, p_vol);
        const BSP_STATUS status = bsp_dac_single_voltage_set(&dac_dev, 1, v_dac, 0);
        if (status != BSP_OK)
        {
            printf("bsp_dac_single_voltage_set failed \r\n");
            return;
        }
        bsp_bias_p_select_pin(p_group, p_pin);
        bsp_test_select_mode(BIAS_P);
    }
    else if (mode == BIAS_1V_MODE)
    {
        bsp_bias_p_select_pin(p_group, p_pin);
        bsp_test_select_mode(BIAS_1V);
    }
    else if (mode == BIAS_N_MODE)
    {
        // DAC输出负压
        const uint16_t v_dac = (uint16_t)bsp_dac_convert_bias_to_dac(fabsf(n_vol));
        const BSP_STATUS status = bsp_dac_single_voltage_set(&dac_dev, 2, v_dac, 0);
        if (status != BSP_OK)
        {
            printf("bsp_dac_single_voltage_set failed \r\n");
            return;
        }

        // 负压选通
        bsp_bias_n_select_pin(n_group, n_pin);
    }
}
#endif
