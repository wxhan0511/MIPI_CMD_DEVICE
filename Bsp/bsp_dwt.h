//
// Created by 薛斌 on 24-8-16.
//

#ifndef BSP_DWT_H
#define BSP_DWT_H

#include <bsp.h>

#define  DWT_CYCCNT  *(volatile unsigned int *)0xE0001004
#define  DWT_CR      *(volatile unsigned int *)0xE0001000
#define  DEM_CR      *(volatile unsigned int *)0xE000EDFC
#define  DBGMCU_CR   *(volatile unsigned int *)0xE0042004


void bsp_init_dwt(void);
void bsp_delay_dwt(uint32_t time);
void bsp_delay_us(uint32_t time);
void bsp_delay_ms(uint32_t time);
uint32_t dwt_get_ms(void);
#endif //BSP_DWT_H
