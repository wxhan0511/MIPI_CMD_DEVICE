//
// Created by 薛斌 on 24-8-17.
//

#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <bsp.h>
enum {
    P11_FLASH_ADDR = 0xC0,
    P2_FLASH_ADDR  = 0xC2,
    P3_FLASH_ADDR = 0xC4
};
enum {
    P11_FLASH = 0,
    P2_FLASH  = 1,
    P3_FLASH = 2
};
typedef struct
{
    void* handle;
    uint8_t dev_addr[4];
} i2c_dev_t;

#endif //BSP_I2C_H
