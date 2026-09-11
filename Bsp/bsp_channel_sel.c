//
// Created by xuebin on 24-8-19.
//

#include "bsp_channel_sel.h"

#include <math.h>

#include "bsp_ads1256.h"
#include "bsp_dwt.h"
#include "task_sample.h"
// The AIx mux-selected channel number is recorded in chx_flag
volatile uint8_t ch0_flag = 8;
volatile uint8_t ch1_flag = 8;
volatile uint8_t ch2_flag = 8;
volatile R_D_MODE r_d_mode = 0;
static volatile rly_gear_state_t s_rly_gear_state = {0};
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
// ANCHOR - Initialize all 8 D flip-flop channel selections to low
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

// ANCHOR - Select diode or resistor mode
//  datasheet:SW_R&D
//  @param mode mode = 0 for diode, 1 for resistor
void bsp_rd_select_mode(const R_D_MODE mode)
{
    if (mode == D_MODE || mode == R_MODE)
        bsp_d_trigger_set_channel(&d_8, 5, mode);
    r_d_mode = mode;
}

// Update the state struct while setting the hardware
void bsp_rly_gear_set(TEST_CUR_GEAR gear, RLY_INDEX rly_index)
{
    // Bounds check to prevent out-of-range access
    if (rly_index >= VSN_RLY && rly_index < RLY_INDEX_MAX)
    {
        // Perform the low-level hardware setting
        s_rly_gear_state.gear[rly_index] = gear;

        bsp_d_trigger_set_channel(&d_2, rly_index, gear);
    }
}

// Get the current gear state
TEST_CUR_GEAR bsp_rly_gear_get(RLY_INDEX rly_index)
{
    if (rly_index >= VSN_RLY && rly_index < RLY_INDEX_MAX)
    {
        TEST_CUR_GEAR gear = bsp_d_trigger_get_channel_state(&d_2, rly_index);
    }
    return GEAR_uA; // Default return
}
// Read function dedicated for interrupt context
TEST_CUR_GEAR bsp_rly_get_gear_isr(RLY_INDEX rly_index)
{
    // Bounds check
    if (rly_index >= VSN_RLY && rly_index < RLY_INDEX_MAX)
    {
        // Read the volatile variable directly, lock-free and very fast
        return s_rly_gear_state.gear[rly_index];
    }
    // Return the default value if out of range
    return GEAR_uA;
}
int16_t bsp_rly_get_last_voltage_isr(RLY_INDEX rly_index)
{
    // Bounds check
    if (rly_index >= VSN_RLY && rly_index < RLY_INDEX_MAX)
    {
        // Get the global struct pointer directly in the interrupt
        calibration_data_t *cal = &g_calibration_manager.data;

        // Get the corresponding last_voltage by enum index
        switch (rly_index)
        {
        case VSN_RLY:
            return cal->vsn_last_voltage;
        case ELVSS_RLY:
            return cal->elvss_last_voltage;
        case VCC_RLY:
            return cal->vcc_last_voltage;
        case IOVCC_RLY:
            return cal->iovcc_last_voltage;
        case VSP_RLY:
            return cal->vsp_last_voltage;
        case AVDD_RLY:
            return cal->avdd_last_voltage;
        case VDD_RLY:
            return cal->vdd_last_voltage;
        case ELVDD_RLY:
            return cal->elvdd_last_voltage;
        default:
            break;
        }
    }

    // Return the default value 0 if out of range
    return 0;
}
void bsp_rly_gear_set_all(TEST_CUR_GEAR gear)
{
    bsp_rly_gear_set(gear, VSN_RLY);
    bsp_rly_gear_set(gear, ELVSS_RLY);
    bsp_rly_gear_set(gear, VCC_RLY);
    bsp_rly_gear_set(gear, IOVCC_RLY);
    bsp_rly_gear_set(gear, VSP_RLY);
    bsp_rly_gear_set(gear, AVDD_RLY);
    bsp_rly_gear_set(gear, VDD_RLY);
    bsp_rly_gear_set(gear, ELVDD_RLY);
}

// ANCHOR - Select the resistance level
//  @param r_level Resistance level
void bsp_rd_select_r_level(const TEST_R_D_RES_LEVEL r_level)
{
    if (r_level >= OHM_10_M && r_level <= OHM_4_point_7_K)
    {
        bsp_d_trigger_set_channel(&d_8, 0, truth_table[r_level][2]); // A0
        bsp_d_trigger_set_channel(&d_8, 1, truth_table[r_level][1]); // A1
        bsp_d_trigger_set_channel(&d_8, 2, truth_table[r_level][0]); // A2
        bsp_d_trigger_set_channel(&d_8, 3, 1);                       // EN
    }
    M_SPI_DEBUG("selected r level: %d\r\n", r_level);
    dev_vol.sample_res_gear_rd = r_level;
}
void bsp_close_rd_select_channel()
{
    bsp_d_trigger_set_channel(&d_8, 3, 0); // EN
}
// ANCHOR -  ADS1256 AI0 mux-selected sampling channel
//  @param power_index Power index
void bsp_ads1256_ch0_select(const AI0_INDEX ai0_index)
{
    if (ai0_index >= 0 && ai0_index < 8)
    {
        bsp_d_trigger_set_channel(&d_1, 7, truth_table[ai0_index][2]); // A0
        bsp_d_trigger_set_channel(&d_4, 0, truth_table[ai0_index][1]); // A1
        bsp_d_trigger_set_channel(&d_4, 1, truth_table[ai0_index][0]); // A2
                                                                       // EN always high
        ch0_flag = ai0_index;
        printf("selected: A0=%d, A1=%d, A2=%d\r\n", truth_table[ai0_index][2], truth_table[ai0_index][1], truth_table[ai0_index][0]);
    }
}
// ANCHOR -  ADS1256 AI1 mux-selected sampling channel
//  @param power_index Power index
void bsp_ads1256_ch1_select(const AI1_INDEX ai1_index)
{
    if (ai1_index >= 0 && ai1_index < 8 && ai1_index != 1)
    {
        bsp_d_trigger_set_channel(&d_4, 2, truth_table[ai1_index][2]); // A0
        bsp_d_trigger_set_channel(&d_4, 3, truth_table[ai1_index][1]); // A1
        bsp_d_trigger_set_channel(&d_4, 4, truth_table[ai1_index][0]); // A2
                                                                       // EN always high
        ch1_flag = ai1_index;
    }
}
/* Clear the fault flag */
void bsp_limit_current_reset()
{
    bsp_d_trigger_set_channel(&d_5, 0, 0);
    bsp_delay_us(5);
    bsp_d_trigger_set_channel(&d_5, 0, 1);
}
// ANCHOR -  ADS1256 AI2 mux-selected sampling channel
//  @param power_index Power index
void bsp_ads1256_ch2_select(const AI2_INDEX ai2_index)
{
    if (ai2_index >= 0 && ai2_index < 8 && ai2_index != 1)
    {
        bsp_d_trigger_set_channel(&d_4, 5, truth_table[ai2_index][2]); // A0
        bsp_d_trigger_set_channel(&d_4, 6, truth_table[ai2_index][1]); // A1
        bsp_d_trigger_set_channel(&d_4, 7, truth_table[ai2_index][0]); // A2
                                                                       // EN always high
        ch2_flag = ai2_index;
    }
}

void bsp_lim_rst_set(uint8_t state)
{
    bsp_d_trigger_set_channel(&d_5, 0, state);
}

void bsp_24pin_select_pin(uint8_t pin_num)
{
}
void bsp_close_64pin_channel()
{
    for (uint8_t i = 0; i < 8; i++)
    {
        bsp_d_trigger_set_channel(&d_6, i, 0); // R&D+ EN1-EN8
        bsp_d_trigger_set_channel(&d_7, i, 0); // R&D+ EN1-EN8
    }
}

void bsp_sel_test_freq_ch(uint8_t state)
{
    bsp_d_trigger_set_channel(&d_1, 0, 0);
    bsp_d_trigger_set_channel(&d_1, 6, state);
}

// ANCHOR - Select one of the 64x64 test paths, pin_p to V_R&D, pin_n to ground
//  @param pin_p Positive channel 0-63
//  @param pin_n Negative channel 0-63
void bsp_rd_select_pin(uint16_t pin_p, uint16_t pin_n, uint8_t en)
{
    pin_p = pin_p - 1;
    pin_n = pin_n - 1; // attention: the user uses 1-based numbering, converted to 0-based here

    if (pin_p >= 64 || pin_n >= 64 || pin_p == pin_n || pin_p < 0 || pin_n < 0)
    {
        printf("Invalid pin number. pin_p and pin_n should be between 0 and 63.\r\n");
        return;
    }
    const uint16_t _pin_p_group = pin_p / 8;
    const uint16_t _pin_p = pin_p % 8;
    const uint16_t _pin_n_group = pin_n / 8;
    const uint16_t _pin_n = pin_n % 8;
    // Note: only disable EN here, do not touch A0/A1/A2, otherwise the channel cannot be opened
    if (en == 0)
    {
        if (_pin_p_group == 0)
            bsp_d_trigger_set_channel(&d_6, _pin_p_group, 0);
        if (_pin_n_group == 1)
            bsp_d_trigger_set_channel(&d_7, _pin_n_group, 0);
        return;
    }

    // printf("p group = %u p pin %u : n group = %u n pin %u \r\n ", _pin_p_group,_pin_p,_pin_n_group,_pin_n);
    if (_pin_p_group == 0)
    {
        bsp_d_trigger_set_channel(&d_5, 2, truth_table[_pin_p][2]); // R&D+A0  64 pins in 8 groups; the pin number within the group selects the channel connected to V_R&D
        bsp_d_trigger_set_channel(&d_5, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_5, 4, truth_table[_pin_p][0]); // R&D+A2
        // printf("_pin_p: %d\r\n",_pin_p);
        // printf( "A0:%d, A1:%d, A2:%d\r\n",truth_table[_pin_p][2], truth_table[_pin_p][1], truth_table[_pin_p][0]);
        // bsp_d_trigger_set_channel(&d_6, 0, en); // R&D+ EN1
    }
    else if (_pin_p_group == 1)
    {
        bsp_d_trigger_set_channel(&d_5, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_5, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_5, 4, truth_table[_pin_p][0]); // R&D+A2

        // bsp_d_trigger_set_channel(&d_6, 1, en); // R&D+ EN2
    }
    else if (_pin_p_group == 2)
    {
        bsp_d_trigger_set_channel(&d_5, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_5, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_5, 4, truth_table[_pin_p][0]); // R&D+A2

        // bsp_d_trigger_set_channel(&d_6, 2, en); // R&D+ EN3
    }
    else if (_pin_p_group == 3)
    {
        bsp_d_trigger_set_channel(&d_5, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_5, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_5, 4, truth_table[_pin_p][0]); // R&D+A2

        // bsp_d_trigger_set_channel(&d_6, 3, en); // R&D+ EN4
    }
    else if (_pin_p_group == 4)
    {
        bsp_d_trigger_set_channel(&d_5, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_5, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_5, 4, truth_table[_pin_p][0]); // R&D+A2

        // bsp_d_trigger_set_channel(&d_6, 4, en); // R&D+ EN5
    }
    else if (_pin_p_group == 5)
    {
        bsp_d_trigger_set_channel(&d_5, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_5, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_5, 4, truth_table[_pin_p][0]); // R&D+A2

        // bsp_d_trigger_set_channel(&d_6, 5, en); // R&D+ EN6
    }
    else if (_pin_p_group == 6)
    {
        bsp_d_trigger_set_channel(&d_5, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_5, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_5, 4, truth_table[_pin_p][0]); // R&D+A2

        // bsp_d_trigger_set_channel(&d_6, 6, en); // R&D+ EN7
    }
    else if (_pin_p_group == 7)
    {
        bsp_d_trigger_set_channel(&d_5, 2, truth_table[_pin_p][2]); // R&D+A0
        bsp_d_trigger_set_channel(&d_5, 3, truth_table[_pin_p][1]); // R&D+A1
        bsp_d_trigger_set_channel(&d_5, 4, truth_table[_pin_p][0]); // R&D+A2

        // bsp_d_trigger_set_channel(&d_6, 7, en); // R&D+ EN8
    }

    if (_pin_n_group == 0)
    {
        bsp_d_trigger_set_channel(&d_5, 5, truth_table[_pin_n][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_5, 6, truth_table[_pin_n][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_5, 7, truth_table[_pin_n][0]); // R&D-A2
        // printf("_pin_n: %d\r\n",_pin_n);
        // printf( "A0:%d, A1:%d, A2:%d\r\n",truth_table[_pin_n][2], truth_table[_pin_n][1], truth_table[_pin_n][0]);

        // bsp_d_trigger_set_channel(&d_7, 0, en); // R&D- EN1
    }
    else if (_pin_n_group == 1)
    {
        bsp_d_trigger_set_channel(&d_5, 5, truth_table[_pin_n][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_5, 6, truth_table[_pin_n][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_5, 7, truth_table[_pin_n][0]); // R&D-A2

        // bsp_d_trigger_set_channel(&d_7, 1, en); // R&D- EN2
    }
    else if (_pin_n_group == 2)
    {
        bsp_d_trigger_set_channel(&d_5, 5, truth_table[_pin_n][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_5, 6, truth_table[_pin_n][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_5, 7, truth_table[_pin_n][0]); // R&D-A2

        // bsp_d_trigger_set_channel(&d_7, 2, en); // R&D- EN3
    }
    else if (_pin_n_group == 3)
    {
        bsp_d_trigger_set_channel(&d_5, 5, truth_table[_pin_n][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_5, 6, truth_table[_pin_n][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_5, 7, truth_table[_pin_n][0]); // R&D-A2

        // bsp_d_trigger_set_channel(&d_7, 3, en); // R&D- EN4
    }
    else if (_pin_n_group == 4)
    {
        bsp_d_trigger_set_channel(&d_5, 5, truth_table[_pin_n][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_5, 6, truth_table[_pin_n][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_5, 7, truth_table[_pin_n][0]); // R&D-A2

        // bsp_d_trigger_set_channel(&d_7, 4, en); // R&D- EN5
    }
    else if (_pin_n_group == 5)
    {
        bsp_d_trigger_set_channel(&d_5, 5, truth_table[_pin_n][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_5, 6, truth_table[_pin_n][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_5, 7, truth_table[_pin_n][0]); // R&D-A2

        // bsp_d_trigger_set_channel(&d_7, 5, en); // R&D- EN6
    }
    else if (_pin_n_group == 6)
    {
        bsp_d_trigger_set_channel(&d_5, 5, truth_table[_pin_n][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_5, 6, truth_table[_pin_n][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_5, 7, truth_table[_pin_n][0]); // R&D-A2

        // bsp_d_trigger_set_channel(&d_7, 6, en); // R&D- EN7
    }
    else if (_pin_n_group == 7)
    {
        bsp_d_trigger_set_channel(&d_5, 5, truth_table[_pin_n][2]); // R&D-A0
        bsp_d_trigger_set_channel(&d_5, 6, truth_table[_pin_n][1]); // R&D-A1
        bsp_d_trigger_set_channel(&d_5, 7, truth_table[_pin_n][0]); // R&D-A2

        // bsp_d_trigger_set_channel(&d_7, 7, en); // R&D- EN8
    }
    for (uint8_t i = 0; i < 8; i++)
    {
        if (i == _pin_p_group)
            bsp_d_trigger_set_channel(&d_6, _pin_p_group, en);
        else
            bsp_d_trigger_set_channel(&d_6, i, 0); // R&D
        if (i == _pin_n_group)
            bsp_d_trigger_set_channel(&d_7, _pin_n_group, en);
        else
            bsp_d_trigger_set_channel(&d_7, i, 0); // R&D
    }
}

// ANCHOR - Frequency test channel selection
// datasheet:408EN1~4
//@param pin 0~23
// NOTE - Before enabling the frequency test, perform a voltage measurement on the selected channel. EN is allowed only for 0~5V, otherwise the protection device may be damaged - the comparator Ref_Freq needs to be set
void bsp_select_24pin_channel(uint16_t pin, uint8_t en)
{
    uint16_t _pin_group = pin / 8;
    uint16_t _pin = pin % 8;
    if (en == 0)
    {
        if (_pin_group == 0)
            bsp_d_trigger_set_channel(&d_1, 3, 0);
        else if (_pin_group == 1)
            bsp_d_trigger_set_channel(&d_1, 4, 0);
        else if (_pin_group == 2)
            bsp_d_trigger_set_channel(&d_1, 5, 0);
    }
    if (_pin_group == 0 && _pin > 3)
    {
        _pin_group = 1;
        _pin = _pin - 4;
    }
    if (_pin_group == 1 && _pin < 4)
    {
        _pin_group = 0;
        _pin = _pin + 4;
    }
    if (pin >= 0 && pin <= 23 && _pin_group <= 2)
    {
        bsp_d_trigger_set_channel(&d_1, 0, truth_table[_pin][2]); // 408A0  24 pins in 8 groups; the pin number within the group selects the channel to connect
        bsp_d_trigger_set_channel(&d_1, 1, truth_table[_pin][1]); // 408A1
        bsp_d_trigger_set_channel(&d_1, 2, truth_table[_pin][0]); // 408A2

        if (_pin_group == 0)
        {
            bsp_d_trigger_set_channel(&d_1, 3, 1); // 408EN1
            bsp_d_trigger_set_channel(&d_1, 4, 0);
            bsp_d_trigger_set_channel(&d_1, 5, 0);
        }
        else if (_pin_group == 1)
        {
            bsp_d_trigger_set_channel(&d_1, 3, 0); // 408EN1
            bsp_d_trigger_set_channel(&d_1, 4, 1);
            bsp_d_trigger_set_channel(&d_1, 5, 0);
        }
        else if (_pin_group == 2)
        {
            bsp_d_trigger_set_channel(&d_1, 3, 0); // 408EN1
            bsp_d_trigger_set_channel(&d_1, 4, 0);
            bsp_d_trigger_set_channel(&d_1, 5, 1);
        }
    }
    else
    {
        printf("pin or pin_group out of range \r\n");
    }
}

void bsp_close_24pin_channel(void)
{
    bsp_d_trigger_set_channel(&d_1, 3, 0); // 408EN1
    bsp_d_trigger_set_channel(&d_1, 4, 0); // 408EN2
    bsp_d_trigger_set_channel(&d_1, 5, 0); // 408EN3
}
void bsp_close_40pin_channel(void)
{
    bsp_d_trigger_set_channel(&d_6, 0, 0);
    bsp_d_trigger_set_channel(&d_6, 1, 0);
    bsp_d_trigger_set_channel(&d_6, 2, 0);
    bsp_d_trigger_set_channel(&d_6, 3, 0);
    bsp_d_trigger_set_channel(&d_6, 4, 0);
    bsp_d_trigger_set_channel(&d_6, 5, 0);
    bsp_d_trigger_set_channel(&d_6, 6, 0);
    bsp_d_trigger_set_channel(&d_6, 7, 0);
    bsp_d_trigger_set_channel(&d_7, 0, 0);
    bsp_d_trigger_set_channel(&d_7, 1, 0);
    bsp_d_trigger_set_channel(&d_7, 2, 0);
    bsp_d_trigger_set_channel(&d_7, 3, 0);
    bsp_d_trigger_set_channel(&d_7, 4, 0);
    bsp_d_trigger_set_channel(&d_7, 5, 0);
    bsp_d_trigger_set_channel(&d_7, 6, 0);
    bsp_d_trigger_set_channel(&d_7, 7, 0);
}
