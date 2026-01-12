/**
 * @file       led_task.h
 * @brief      RA_PowerEX
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-10-11
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

#ifndef __LED_TASK_H
#define __LED_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os2.h"
/* Exported types ------------------------------------------------------------*/
extern osTimerId_t led_timerHandle;
extern const osTimerAttr_t led_timer_attributes;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

#ifdef __cplusplus
}
#endif

void led_timer_callback(void *argument);

#endif /* __LED_TASK_H */


