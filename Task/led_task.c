#include "led_task.h"
#include "main.h"

#ifdef USE_OLED
osTimerId_t led_timerHandle;
const osTimerAttr_t led_timer_attributes = {
    .name = "LEDTimer",
};
#endif
#ifdef USE_OLED
/* led_timer_callback function */
/**
 * @brief  Function implementing the led_timer.
 * @param  argument: Not used
 * @retval None
 */
void led_timer_callback(void *argument)
{
  for (;;)
  {
    osDelay(100);
    osDelay(100);
  }
}
#endif