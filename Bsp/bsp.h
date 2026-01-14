//
// Created by 薛斌 on 24-8-16.
//

#ifndef BSP_H
#define BSP_H

#include <bsp_log.h>
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include "bsp_dwt.h"
#include "cmsis_os.h"

extern UART_HandleTypeDef huart1;
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi_tp;
extern I2C_HandleTypeDef hi2c1;
extern uint8_t id;
typedef StaticSemaphore_t osStaticMutexDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;


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

typedef enum
{
    BSP_OK,
    BSP_ERROR,
} BSP_STATUS;

extern uint8_t sw_version[4];
extern uint8_t magic_number[4];
extern uint8_t hw_version[4];

// func
void bsp_init();
void bsp_CCP_Init(void);
void bsp_led_pwm_init(void);
#endif // BSP_H
