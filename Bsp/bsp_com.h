//
// Created by 17333 on 25-6-27.
//

#ifndef BSP_COM_H
#define BSP_COM_H

#include <bsp.h>
#include "i2c.h"
#include "com_define.h"


// #define METER_INT_H HAL_GPIO_WritePin(MASTER_TX_GPIO_Port, MASTER_TX_Pin, GPIO_PIN_SET);
// #define METER_INT_L HAL_GPIO_WritePin(MASTER_TX_GPIO_Port, MASTER_TX_Pin, GPIO_PIN_RESET);

extern __IO METER_COM_MODE meter_com_mode;

extern uint8_t meter_tx_buf[64];

extern uint8_t meter_rx_buf[64];

void bsp_com_init(void);

void bsp_com_tx_rx(uint8_t* tx,uint8_t* rx, uint16_t len,uint8_t wait_flag);

void bsp_com_callback();

#endif //BSP_COM_H
