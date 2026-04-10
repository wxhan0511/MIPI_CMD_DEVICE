/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "i2c_task.h"
#include "gtb_task.h"
#include "led_task.h"
#include "power_task.h"
/*lvgl*/
#include "widget_main.h"
#include "lv_port_disp_template.h"
#include "widget_func.h"
#include "task_sample.h"

/* Private function prototypes -----------------------------------------------*/

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
#ifdef GTB
  //server_gtb_init();
#endif

  //slave_rx_task_init();
  //slave_tx_task_init();

  //master_tx_task_init();
  //master_rx_task_init();

  //power_task_init();
  task_sample_init();
  //widget_main_task_init(); //LVGL UI task


}

/* USER CODE END Application */



