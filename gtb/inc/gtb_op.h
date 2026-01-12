//
// Created by Bobby on 2023/8/9.
//

#ifndef AE_TOOL_MOTHER_BOARD_GTB_OP_H
#define AE_TOOL_MOTHER_BOARD_GTB_OP_H

#include <stdbool.h>
#include "bsp_gtb.h"
void fw_mode_switch(tp_config_t *tp_config,uint8_t ic_type_index,bool interface,uint8_t mode);
//gloable variables initial
void gtb_global_var_init(tp_config_t *tp_config);

int32_t gtb_read_coordination_dynamic(tp_config_t *tp_config,uint8_t* read_data);

uint16_t gtb_get_demo_data_len(uint8_t ic_type_index);

int32_t gtb_read_raw_data(tp_config_t *tp_config,bool interface,uint8_t ic_type_index,uint16_t len);

uint16_t gtb_get_debug_data_index(uint8_t ic_type_index);

#endif //AE_TOOL_MOTHER_BOARD_GTB_OP_H