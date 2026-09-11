#include "led_task.h"
#include "main.h" // User variables
/* led_timer_callback function */
/**
 * @brief  Function implementing the led_timer.
 * @param  argument: Not used
 * @retval None
 */
void led_timer_callback(void *argument)
{
  HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_3);
}
