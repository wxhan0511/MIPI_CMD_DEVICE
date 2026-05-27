#include "task_manage.h"

/* Definitions for show_mutex */
osThreadId_t pwmctrlTaskHandle;
const osThreadAttr_t pwmTask_attributes = {
    .name = "pwmTask",
    .stack_size = 1024,
    .priority = (osPriority_t)osPriorityNormal,
};
