//
// Created by 17333 on 25-6-26.
//

#include "bsp_led.h"

#include "main.h"
#include "stm32f4xx_hal_gpio.h"

#ifdef LED_FUNC
void bsp_led_on()
{
    HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,0);
}

void bsp_led_off()
{
    HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,1);
}
#endif