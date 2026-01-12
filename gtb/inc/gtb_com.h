//
// Created by Bobby on 2023/8/9.
//

#ifndef AE_TOOL_MOTHER_BOARD_GTB_COM_H
#define AE_TOOL_MOTHER_BOARD_GTB_COM_H

#include <stdbool.h>
// #include "main.h"
#include "stm32f4xx_hal.h"
#include "bsp_gtb.h"
#define GTB_HID 0x01
#define GTB_CDC 0x02
#define GTB_MIX 0x03

uint8_t gtb_fs_transmit(uint8_t* hid_data,uint32_t len,uint8_t com_mode);

uint32_t hid_hs_get_data(uint8_t *get_data,uint32_t data_num);

void send_error_code(bool mode,uint8_t *output,int32_t error_code,uint8_t com_mode);

void send_usb_trans_status(bool mode,uint8_t *output,uint8_t status,uint8_t com_mode);

void send_usb_trans_status_fspi(bool mode,uint8_t *output,uint16_t count,uint8_t result,uint8_t com_mode);

void gtb_fw_mode_com(tp_config_t *tp_config,uint8_t* arg,uint8_t *output,uint8_t com_mode);

void gtb_generic_com(tp_config_t *tp_config,uint8_t* arg,uint8_t *output,uint8_t com_mode);


#endif //AE_TOOL_MOTHER_BOARD_GTB_COM_H