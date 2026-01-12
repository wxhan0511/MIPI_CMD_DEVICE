#include "tim.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f407xx.h"
extern TIM_HandleTypeDef htim2;

void MX_TIM2_Init(void)
{
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 41;           // 假设APB1时钟为84MHz，分频后为1MHz
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 0xFFFFFFFF;      // 最大计数，方便长时间延时
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
}

// 启动TIM2计时器
void TIM2_Start(void)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    HAL_TIM_Base_Start(&htim2);
}

void TIM2_Stop(void)
{
    HAL_TIM_Base_Stop(&htim2);
    //重置计数器
    __HAL_TIM_SET_COUNTER(&htim2, 0);
}

// timeout延时函数，单位ms
void timeout_ms(uint32_t ms)
{
    TIM2_Start();
    while (__HAL_TIM_GET_COUNTER(&htim2) < ms * 1000) {
        // 可加WFI省电或空循环
    }
    TIM2_Stop();
}
// 获取TIM2当前计数值（单位：us，如果定时器为1MHz）
uint32_t TIM2_GetCounter(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}
//重写HAL_GetTick
// uint32_t HAL_GetTick(void)
// {
//     return TIM2_GetCounter() / 1000;  // 将计数值转换为ms
// }
