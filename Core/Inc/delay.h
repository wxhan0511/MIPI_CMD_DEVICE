#ifndef __DELAY_H
#define __DELAY_H

#include <stdint.h>



void MX_TIM2_Init(void);
void TIM2_Start(void);
void TIM2_Stop(void);
void timeout_ms(uint32_t ms);
uint32_t TIM2_GetCounter(void);
uint32_t HAL_GetTick(void);

#endif