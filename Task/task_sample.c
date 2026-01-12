/*
 * task_sample.c
 *
 *  Created on: Jun 30, 2025
 *      Author: Wenxiao Han
 */

#include "task_sample.h"
#include "usart.h"
#include "i2c.h"
osMutexId_t sample_mutex;
osStaticMutexDef_t sample_mutex_control_block;
const osMutexAttr_t sample_mutex_attributes = {
    .name = "show_mutex",
    .cb_mem = &sample_mutex_control_block,
    .cb_size = sizeof(sample_mutex_control_block),
  };

void task_sample_run()
{
    for(;;)
    {
        osDelay(100);
    }

}



void task_sample_init(void)
{
    task_sample_handle = osThreadNew(task_sample_run,NULL, &task_sample_attributes);
    if (task_sample_handle == NULL)
    {
    }
}
