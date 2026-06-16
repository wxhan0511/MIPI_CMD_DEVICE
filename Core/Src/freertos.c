/* ==================== 1. 头文件包含 ==================== */
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
#include "task_com.h"
#include "lcd.h"

/* ==================== 2. 宏定义 ==================== */
/* 无 */

/* ==================== 3. 类型定义（结构体、枚举、别名） ==================== */
/* 无 */

/* ==================== 4. 外部全局变量 ==================== */
/* 无 */

/* ==================== 5. 静态私有变量 ==================== */
/* 无 */

/* ==================== 6. 静态函数声明 ==================== */

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */
void StartDefaultTask(void *argument);

/* Definitions for show_mutex */
osMutexId_t show_mutexHandle;
osStaticMutexDef_t show_mutex_control_block;
const osMutexAttr_t show_mutex_attributes = {
    .name = "show_mutex",
    .cb_mem = &show_mutex_control_block,
    .cb_size = sizeof(show_mutex_control_block),
};
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
osTimerId_t led_timerHandle;
const osTimerAttr_t led_timer_attributes = {
    .name = "led_timer"};

/* ==================== 7. 外部可调用函数实现 ==================== */
/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
  show_mutexHandle = osMutexNew(&show_mutex_attributes);
  led_timerHandle = osTimerNew(led_timer_callback, osTimerPeriodic, NULL, &led_timer_attributes);

  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  widget_main_task_init(); // LVGL UI task
  // power_task_init();
  task_sample_init();
  task_com_init();
  // pwm_ctrl_task_init();
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  // osDelay(2000);

  printf("StartDefaultTask running\r\n");
  osTimerStart(led_timerHandle, 500);
  printf("StartDefaultTask running\r\n");
  uint8_t key_flag = 0;
  uint8_t key_flag_1 = 0;
  uint8_t page0_flag = 0;
  extern __IO uint8_t current_page;
  extern lv_obj_t *rotate_page1;
  extern lv_obj_t *rotate_page2;
  extern lv_obj_t *rotate_page3;
  extern lv_obj_t *page1;
  extern lv_obj_t *page2;
  extern lv_obj_t *page3;
  /* Infinite loop */
  for (;;)
  {
    osDelay(10);
    lv_tick_inc(10); // 同步推进 LVGL 的内部时钟 10ms
#if 1
    if (HAL_GPIO_ReadPin(PULSE_A_GPIO_Port, PULSE_A_Pin))
    {
      printf("PULSE_A_Pin pressed\r\n");
      // 临界区外获取信号量是安全的
      if (osMutexAcquire(show_mutexHandle, osWaitForever) == osOK)
      {
        if (current_page == PAGE_0)
        {
          lv_screen_load_anim(page1, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false);
          printf("Current page is PAGE_0\r\n");
        }
        else if (current_page == PAGE_1)
        {
          lv_screen_load_anim(page3, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false);
          printf("Current page is PAGE_1\r\n");
        }
        else if (current_page == PAGE_2)
        {
          lv_screen_load_anim(page2, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false);
          printf("Current page is PAGE_2\r\n");
        }
        printf("wait acquire show_mutex\r\n");

        lv_display_t *disp = lv_display_get_default();
        if (disp)
        {
          if (disp)
            lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0); // 90 / 270 依需求
          lcd_write_cmd_8bit(0x36);                               // 发送指令
          lcd_write_data_8bit(0x28);                              // 保持竖屏逻辑 BGR顺序
          if (current_page == PAGE_0)
            lv_obj_set_size(page1, 320, 240); // 更新页面尺寸以适应新的显示方向
          if (current_page == PAGE_1)
            lv_obj_set_size(page3, 320, 240); // 更新页面尺寸以适应新的显示方向
          if (current_page == PAGE_2)
            lv_obj_set_size(page2, 320, 240); // 更新页面尺寸以适应新的显示方向
          lv_obj_invalidate(lv_screen_active());
          lv_display_send_event(disp, LV_EVENT_REFR_REQUEST, NULL);
        }
        osMutexRelease(show_mutexHandle);
        key_flag_1 = 1;
        current_page = (current_page + 1) % 3;
        osDelay(500);
      }
    }
    if (key_flag == 1)
    {

      key_flag = 0;
    }
    if (HAL_GPIO_ReadPin(PULSE_B_GPIO_Port, PULSE_B_Pin))
    {
      osDelay(10);
      if (osMutexAcquire(show_mutexHandle, osWaitForever) == osOK)
      {
        if (current_page == 0)
          page0_flag = 1;
        else
          current_page = current_page - 1;
        page0_flag = 0;

        if (current_page == PAGE_0)
        {
          lv_screen_load_anim(rotate_page1, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false);
        }
        else if (current_page == PAGE_1)
        {
          lv_screen_load_anim(rotate_page3, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false);
        }
        else if (current_page == PAGE_2)
        {
          lv_screen_load_anim(rotate_page2, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false);
        }
        printf("wait acquire show_mutex\r\n");
        // 临界区外获取信号量是安全的

        lv_display_t *disp = lv_display_get_default();
        if (disp)
        {
          if (disp)
            lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90); // 90 / 270 依需求
          lcd_write_cmd_8bit(0x36);                                // 发送指令
          lcd_write_data_8bit(0x48);                               // 0x48 = 0x40 + 0x08 → MX=1，BGR=1，MV=0，MY=0。启用 X 轴镜像（左右翻转）并把颜色顺序设为 BG
          if (current_page == PAGE_0)
            lv_obj_set_size(rotate_page1, 240, 320); // 更新页面尺寸以适应新的显示方向
          if (current_page == PAGE_1)
            lv_obj_set_size(rotate_page3, 240, 320); // 更新页面尺寸以适应新的显示方向
          if (current_page == PAGE_2)
            lv_obj_set_size(rotate_page2, 240, 320); // 更新页面尺寸以适应新的显示方向
          lv_obj_invalidate(lv_screen_active());
          lv_display_send_event(disp, LV_EVENT_REFR_REQUEST, NULL);
        }
        osMutexRelease(show_mutexHandle);

        key_flag_1 = 1;

        if (page0_flag != 1)
          current_page = current_page + 1;
        osDelay(500);
      }
    }
    if (key_flag_1 == 1)
    {

      key_flag_1 = 0;
    }
#endif
    // else if(HAL_GPIO_ReadPin(KEY_2_GPIO_Port,KEY_2_Pin) == 1)
    // {
    //   widget_change(1);
    //   //printf("key 2 changed\r\n");
    // }else
    // {
    //   printf("key status %d\r\n",HAL_GPIO_ReadPin(KEY_2_GPIO_Port,KEY_2_Pin));
    // }
  }
  /* USER CODE END StartDefaultTask */
}

/* ==================== 8. 静态私有函数实现 ==================== */
/* 无 */

/* USER CODE END Application */
