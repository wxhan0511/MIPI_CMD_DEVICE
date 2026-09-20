/* ==================== 1. Includes ==================== */
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
#include "retarget.h"

/* ==================== 2. Macros ==================== */
#define KEY1_DEBOUNCE_MS 50U
#define KEY1_LONG_PRESS_MS 3000U
#define KEY1_STARTUP_WINDOW_MS 10000U

/* ==================== 3. Type definitions (structs, enums, aliases) ==================== */
/* None */

/* ==================== 4. External global variables ==================== */
/* None */

/* ==================== 5. Static private variables ==================== */
/* None */

/* ==================== 6. Static function declarations ==================== */

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */
void StartDefaultTask(void *argument);
static void key1_short_press_action(void);

/* Definitions for show_mutex */
osMutexId_t show_mutexHandle;
osStaticMutexDef_t show_mutex_control_block;
const osMutexAttr_t show_mutex_attributes = {
    .name = "show_mutex",
    .attr_bits = osMutexPrioInherit,
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

static void key1_short_press_action(void)
{
  extern __IO uint8_t current_page;
  extern lv_obj_t *page1;
  extern lv_obj_t *page2;
  extern lv_obj_t *page3;

  if (osMutexAcquire(show_mutexHandle, 100U) != osOK)
    return;

  lv_obj_t *target_page = NULL;
  if (current_page == PAGE_0)
    target_page = page1;
  else if (current_page == PAGE_1)
    target_page = page3;
  else if (current_page == PAGE_2)
    target_page = page2;

  lv_display_t *disp = lv_display_get_default();
  if (disp && target_page)
  {
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);
    lcd_write_cmd_8bit(0x36);
    lcd_write_data_8bit(0x28);
    lv_obj_set_size(target_page, 320, 240);
    lv_screen_load_anim(target_page, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    lv_obj_invalidate(target_page);
    lv_refr_now(disp);
  }

  osMutexRelease(show_mutexHandle);
  current_page = (current_page + 1U) % 3U;
}

/* ==================== 7. Public function implementations ==================== */
/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
  bsp_retarget_rtos_init();
  show_mutexHandle = osMutexNew(&show_mutex_attributes);
  led_timerHandle = osTimerNew(led_timer_callback, osTimerPeriodic, NULL, &led_timer_attributes);

  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  widget_main_task_init(); // LVGL UI task
  //  power_task_init();
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
  osDelay(2000);

  printf("StartDefaultTask running\r\n");
  osTimerStart(led_timerHandle, 500);
  printf("StartDefaultTask running\r\n");
  uint8_t key1_raw_low = 0U;
  uint8_t key1_pressed = 0U;
  uint8_t key1_long_triggered = 0U;
  uint8_t key1_boot_candidate = 0U;
  uint32_t key1_low_tick = 0U;
  const uint32_t key1_boot_window_end = osKernelGetTickCount() + KEY1_STARTUP_WINDOW_MS;
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
#if 1
    const uint32_t key_now = osKernelGetTickCount();
    if (!HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin))
    {
      if (!key1_raw_low)
      {
        key1_raw_low = 1U;
        key1_low_tick = key_now;
      }
      else if (!key1_pressed &&
               (uint32_t)(key_now - key1_low_tick) >= KEY1_DEBOUNCE_MS)
      {
        key1_pressed = 1U;
        key1_boot_candidate =
            ((int32_t)(key1_low_tick - key1_boot_window_end) <= 0) ? 1U : 0U;
      }

      if (key1_pressed && key1_boot_candidate && !key1_long_triggered &&
          (uint32_t)(key_now - key1_low_tick) >= KEY1_LONG_PRESS_MS)
      {
        static const uint8_t no_ip[4] = {0U, 0U, 0U, 0U};
        key1_long_triggered = 1U;
        task_sample_panel_event_post(PANEL_EVENT_NEXT_WIFI);
        ui_set_network_info(&lcd_show, NETWORK_STATE_CONNECTING, no_ip);
        printf("KEY1 long press: request next WiFi\r\n");
      }
    }
    else
    {
      key1_raw_low = 0U;
      if (key1_pressed && !key1_long_triggered)
        key1_short_press_action();
      key1_pressed = 0U;
      key1_long_triggered = 0U;
      key1_boot_candidate = 0U;
    }
    if (!HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin))
    {
#ifdef calibration_board_mode
      HAL_GPIO_WritePin(TSPI_CS_GPIO_Port, TSPI_CS_Pin, 0);
      osDelay(1);
      HAL_GPIO_WritePin(TSPI_CS_GPIO_Port, TSPI_CS_Pin, 1);
#endif

      osDelay(10);
      if (osMutexAcquire(show_mutexHandle, 100U) == osOK)
      {
        if (current_page == 0)
          page0_flag = 1;
        else
          current_page = current_page - 1;
        page0_flag = 0;

        lv_obj_t *target_page = NULL;
        if (current_page == PAGE_0)
        {
          target_page = rotate_page1;
        }
        else if (current_page == PAGE_1)
        {
          target_page = rotate_page3;
        }
        else if (current_page == PAGE_2)
        {
          target_page = rotate_page2;
        }
        lv_display_t *disp = lv_display_get_default();
        if (disp && target_page)
        {
          lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90); // 90 / 270 as needed
          lcd_write_cmd_8bit(0x36);                                // Send command
          lcd_write_data_8bit(0x48);                               // 0x48 = 0x40 + 0x08 -> MX=1, BGR=1, MV=0, MY=0: enable X mirroring (horizontal flip) and set the color order to BGR
          lv_obj_set_size(target_page, 240, 320);
          lv_screen_load_anim(target_page, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
          lv_obj_invalidate(target_page);
          lv_refr_now(disp);
        }
        osMutexRelease(show_mutexHandle);

        if (page0_flag != 1)
          current_page = current_page + 1;
        osDelay(500);
      }
    }
#endif
  }
  /* USER CODE END StartDefaultTask */
}

/* ==================== 8. Static private function implementations ==================== */
/* None */

/* USER CODE END Application */
