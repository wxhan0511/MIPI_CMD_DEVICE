// //
// // Created by 17333 on 25-6-27.
// //

// #include "bsp_com.h"
// #include "i2c.h"
// #include "main.h"

// uint8_t meter_rx_buf[64];
// uint8_t meter_tx_buf[64];
// extern SPI_HandleTypeDef hspi_tp;
// __IO uint8_t meter_com_flag = 0;

// __IO METER_COM_MODE meter_com_mode = METER_CMD_MODE;

// void bsp_meter_com_init(void)
// {
//     meter_tx_buf[0] = FRAME_CMD_HEAD_TX;
//     meter_tx_buf[1] = FRAME_CMD_SUCCESS_0;
//     meter_tx_buf[2] = FRAME_CMD_SUCCESS_1;
//     meter_tx_buf[METER_CMD_LEN - 1] = FRAME_TAIL_TX;

//     METER_INT_H
//     METER_INT_L
//     METER_INT_H
//     METER_INT_L
//     METER_INT_H

//     if (HAL_I2C_EnableListen_IT(&hi2c1) != HAL_OK)
//     {
//         printf("meter spi init error \r\n");
//         Error_Handler(__FILE__, __LINE__);
//     }
//     while (meter_com_flag == 1) {}
//     meter_com_flag = 0;
// }

// void bsp_meter_com_tx_rx(uint8_t* tx, uint8_t* rx, const uint16_t len, const uint8_t wait_flag)
// {
//     const HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive_DMA(&hspi_tp, tx, rx, len);
//     if (ret != HAL_OK)
//     {
//         printf("meter spi rx tx error %d\r\n", ret);
//         Error_Handler(__FILE__, __LINE__);
//     }
//     if(wait_flag == 0)
//     {
//         return;
//     }
//     while (meter_com_flag == 0) {}
//     meter_com_flag = 0;
// }


// void bsp_meter_com_callback()
// {
// #if METER_COM_DEBUG
//     printf("[irq callback]\r\n");
//     //printf("--------- 0x%x 0x%x 0x%x 0x%x \r\n", meter_tx_buf[0], meter_tx_buf[1], meter_tx_buf[2], meter_tx_buf[3]);
// #endif
//     //通知准备下次数据
//     METER_INT_L
//     meter_com_flag = 1;
// }
// #ifdef I2C_MASTER

// #endif
// #ifdef I2C_SLAVE
// // void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
// // {
// //     if(hi2c->Instance == hi2c1.Instance)
// //     {       
// //         i2c_rx_done = 1;
// //         i2c_bus_busy = 0;


// //         // 关键：再次进入接收等待，支持主机多次写
// //         HAL_I2C_Slave_Receive_IT(&hi2c1, rx_buf, 256); // 256为数据长度，根据实际修改
// //     }
// // }
// // void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
// // {
// //     if(hi2c->Instance == hi2c1.Instance)
// //     {
// //         i2c_tx_done = 1;
// //         i2c_bus_busy = 0;

// //                 // 关键：再次进入发送等待，保证主机下次读还能响应
// //         HAL_I2C_Slave_Transmit_IT(&hi2c1, tx_buf, 256); // 256为数据长度，根据实际修改
// //     }
// // }
// #endif
