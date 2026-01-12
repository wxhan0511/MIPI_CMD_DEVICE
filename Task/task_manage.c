#include "task_manage.h"

osThreadId_t task_sample_handle;
const osThreadAttr_t task_sample_attributes = {
    .name = "task_sample_task",
    .stack_size = 2048,
    .priority = (osPriority_t) osPriorityNormal,
  };
