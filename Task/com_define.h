//
// Created by 薛斌 on 24-8-19.
//

#ifndef COMMAND_DEFINE_H
#define COMMAND_DEFINE_H

#include "bsp.h"

#define FRAME_CMD_HEAD_RX   0xf8
#define FRAME_CMD_HEAD_TX   0x8f

#define FRAME_DATA_HEAD_RX  0xe7
#define FRAME_DATA_HEAD_TX  0x7e

#define FRAME_TAIL_RX       0xff
#define FRAME_TAIL_TX       0xff


#define FRAME_CMD_SUCCESS_0 0x55
#define FRAME_CMD_SUCCESS_1 0x66
#define FRAME_ERROR_CODE    0x77

typedef enum
{
    METER_CMD_MODE = 63,
    METER_DATA_MODE = 1023,
}METER_COM_MODE;

#define METER_CMD_LEN 64
#define METER_DATA_LEN 1024

#define METER_COM_DEBUG 0

void command_wait_sync();

#endif //COMMAND_DEFINE_H
