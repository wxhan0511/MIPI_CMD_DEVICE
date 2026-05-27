//
// Created by 薛斌 on 24-8-16.
//

#ifndef BSP_H
#define BSP_H
#include "stdint.h"
#include "stdbool.h"
typedef enum
{
    BSP_OK,
    BSP_ERROR,
} BSP_STATUS;
typedef enum
{
    COM_UART,
    COM_SPI,
} COM_ID;

typedef struct
{
    COM_ID id;
    uint8_t data[64];
} com_msg_t;

typedef enum
{
    COM_NORMAL,
    COM_IRQ,
    COM_DMA
} COM_MODE;

extern uint8_t sw_version[4];
extern uint8_t magic_number[4];
extern uint8_t hw_version[4];

#include <bsp_log.h>
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include "bsp_dwt.h"
#include "cmsis_os.h"
#include "bsp_mcp4728_ctl.h"
#include "bsp_ads1256_ctl.h"
#include "bsp_calibration.h"
#include "bsp_channel_sel.h"
#include "bsp_ads1256.h"
extern UART_HandleTypeDef huart3;
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi_tp;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern uint8_t id;
typedef StaticSemaphore_t osStaticMutexDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;

// func
void bsp_init();
void bsp_CCP_Init(void);
void bsp_led_pwm_init(uint8_t pulse);
void bsp_blasi_pwm_init(uint8_t pulse);
#endif // BSP_H
