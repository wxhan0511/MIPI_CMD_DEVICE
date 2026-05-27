/*
 * task_manage.h
 *
 *  Created on: Jun 30, 2025
 *      Author: Wenxiao Han
 */

#ifndef TASK_MANAGE_H
#define TASK_MANAGE_H

#include "bsp.h"

extern osThreadId_t task_sample_handle;
extern const osThreadAttr_t task_sample_attributes;
extern osThreadId_t task_com_handle;
extern const osThreadAttr_t task_com_attributes;
extern osMutexId_t show_mutexHandle;
extern osStaticMutexDef_t show_mutex_control_block;
extern const osMutexAttr_t show_mutex_attributes;
extern osThreadId_t defaultTaskHandle;
extern const osThreadAttr_t defaultTask_attributes;
extern osThreadId_t pwmctrlTaskHandle;
extern const osThreadAttr_t pwmTask_attributes;
extern osTimerId_t led_timerHandle;
extern const osTimerAttr_t led_timer_attributes;

#endif /* TASK_MANAGE_H */
