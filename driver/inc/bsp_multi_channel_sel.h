//
// Created by Bobby on 2023/12/20.
//

#ifndef AE_TOOL_MOTHER_BOARD_BSP_MULTI_CHANNEL_SEL_H
#define AE_TOOL_MOTHER_BOARD_BSP_MULTI_CHANNEL_SEL_H

#include "bsp_i2c_bus.h"

typedef enum {
    CHANNEL_0 = 0x01,
    CHANNEL_1 = 0x02,
    CHANNEL_2 = 0x04,
    CHANNEL_3 = 0x08,
    CHANNEL_4 = 0x10,
    CHANNEL_5 = 0x20,
    CHANNEL_6 = 0x40,
    CHANNEL_7 = 0x80,
}multi_channel;

typedef enum {
    MULTI_CHANNEL_OFF = 0x00,
    MULTI_CHANNEL_ON = 0x01
}multi_channel_en;



BSP_STATUS_T bsp_multi_channel_sel(bsp_i2c_hw_t* handle,uint8_t address,multi_channel channel,multi_channel_en en);

#endif //AE_TOOL_MOTHER_BOARD_BSP_MULTI_CHANNEL_SEL_H
