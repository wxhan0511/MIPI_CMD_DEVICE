//
// Created by 17333 on 25-6-27.
//

#include "com_handle.h"
#include "com_define.h"


void com_handle_i2c(const uint8_t *rx, uint8_t *tx)
{

    //通过帧首判断模式
//     if (rx[0] == FRAME_CMD_HEAD_RX)
//     {
//         //DATA Mode
// #if METER_COM_DEBUG
//         printf("[cmd mode]master 0x%x 0x%x 0x%x 0x%x \r\n",rx[0],rx[1],rx[2],rx[3]);
//         printf("[cmd mode]slave 0x%x 0x%x 0x%x 0x%x \r\n",tx[0],tx[1],tx[2],tx[3]);
// #endif
//         task_sample_task_mutex_acquire();
//         {
//             //准备数据
//             meter_com_mode = METER_CMD_MODE;
//             if (rx[1] == 0x00)
//             {
//                 tx[1] = 0x00;
//                 command_00(rx, tx);
//             }else if (rx[1] == 0x10)
//             {
//                 tx[1] = 0x10;
//                 command_10(rx, tx);
//             }else if (rx[1] == 0x20)
//             {
//                 tx[1] = 0x20;
//                 command_20(rx, tx);
//             }else if (rx[1] == 0x30)
//             {
//                 tx[1] = 0x30;
//                 command_30(rx, tx);
//             }
//             meter_tx_buf[0] = FRAME_DATA_HEAD_TX;
//             bsp_meter_com_tx_rx(tx, (uint8_t*)rx, 64,0);
//             METER_INT_H
//         }
//         task_sample_task_mutex_release();
//     }
//     else if (rx[0] == FRAME_DATA_HEAD_RX)
//     {
// #if METER_COM_DEBUG
//         printf("[data mode]master 0x%x 0x%x 0x%x 0x%x \r\n",rx[0],rx[1],rx[2],rx[3]);
//         printf("[data mode]slave 0x%x 0x%x 0x%x 0x%x \r\n",tx[0],tx[1],tx[2],tx[3]);
// #endif
//         {
//             meter_com_mode = METER_DATA_MODE;
//             tx[0] = FRAME_CMD_HEAD_TX;
//             tx[1] = FRAME_CMD_SUCCESS_0;
//             tx[2] = FRAME_CMD_SUCCESS_1;
//             bsp_meter_com_tx_rx(tx, (uint8_t*)rx, 64,0);
//             METER_INT_H
//         }
//
//     }
//     else
//     {
//         tx[0] = FRAME_ERROR_CODE;
//         tx[METER_CMD_MODE] = FRAME_ERROR_CODE;
//         // bsp_meter_com_rx_tx(rx->data, tx.data,METER_CMD_LEN);
//         LOG_ERROR("meter com error %d \r\n",rx[0]);
//         bsp_meter_com_tx_rx(tx, (uint8_t*)rx, 64,0);
//
//         METER_INT_H
//         return BSP_ERROR;
//     }

    //printf("spi finish \r\n");
}
