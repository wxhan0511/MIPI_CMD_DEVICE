/**
 * @file       gtb.c
 * @brief      GTB  module implementation.
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-10-09
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

/* Includes ------------------------------------------------------------------*/
#include "gtb_task.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "cmsis_os.h"
#include "config.h"
#include "gtb_com.h"
#include "gtb_op.h"
#include "bsp_dwt.h"


/* USB Relative*/
#include "usb_device.h"
#include "usbd_hid_custom.h"
#include "usbd_cdc_acm_if.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
tp_config_t tp_config_hid;

const osThreadAttr_t server_gtb_attr = {
    .name = "ServerGtbTask",
    .stack_size = 1024 * 8,
    .priority = (osPriority_t)osPriorityNormal,
};
osThreadId_t thread_id_gtb;
bool hid_state_fs = 0;
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

extern USBD_HandleTypeDef hUsbDevice;

void server_gtb_init()
{
    thread_id_gtb = osThreadNew(server_gtb, NULL, &server_gtb_attr);
    if (thread_id_gtb == NULL)
    {
        printf("[error] server gtb \r\n");
    }
}

void server_gtb_suspend()
{
    if (thread_id_gtb != NULL)
    {
        osThreadSuspend(thread_id_gtb);
    }
    else
    {
        printf("[gtb log] gtb server is not active\r\n");
    }
}
void test_gtb_task(void)
{
#ifdef USE_OLED
    osTimerStart(led_timerHandle, 1000);
#endif
    GTB_INFO("[gtb task] active \r\n");

    hid_state_fs = 0;
    bsp_gtb_init(3);
    gtb_global_var_init(&tp_config_hid);
    ex_ti_initial(&tp_config_hid, DISABLE); //Please note that the TP-INT interrupt is turned off here
    __IO uint8_t com_mode = GTB_HID;
    for (;;)
    {
        //打印时间戳
        TIME_DEBUG("Tick1: %lu \r\n", dwt_get_ms());
        if(hUsbDevice.dev_state == USBD_STATE_CONFIGURED)
        {

            gtb_fw_mode_com(&tp_config_hid, send_data_fs, get_data_fs, GTB_HID);
            if (hid_state_fs)
            {
                TIME_DEBUG("Tick2: %lu \r\n", dwt_get_ms());
                if (0x40 == send_data_fs[0])
                {   TIME_DEBUG("Tick3: %lu \r\n", dwt_get_ms());
                    GTB_DEBUG("receive HID DATA:%x,%x,%x,%x, enter gtb_generic_com\r\n",send_data_fs[0],send_data_fs[1],send_data_fs[2],send_data_fs[3]);
                    TIME_DEBUG("start: %lu ms\r\n", dwt_get_ms());
                    gtb_generic_com(&tp_config_hid, send_data_fs, get_data_fs, GTB_HID);
                    TIME_DEBUG("end: %lu ms\r\n", dwt_get_ms());
                }
                hid_state_fs = 0;
            }
            osDelay(1);
        }
    }
}


void server_gtb(void *argument)
{
#ifdef USE_OLED
    osTimerStart(led_timerHandle, 1000);
#endif
    GTB_INFO("[gtb task] active \r\n");
    GTB_INFO("CDC need closed\r\n");
    hid_state_fs = 0;
    bsp_gtb_init(3);
    gtb_global_var_init(&tp_config_hid);
    ex_ti_initial(&tp_config_hid, DISABLE);
    __IO uint8_t com_mode = GTB_HID;
    for (;;)
    {

        gtb_fw_mode_com(&tp_config_hid, send_data_fs, get_data_fs, GTB_HID);
        //TIME_DEBUG("ssssstart: %lu ms\r\n", dwt_get_ms());
        if (hid_state_fs)
        {
            if (0x40 == send_data_fs[0])
            {   GTB_DEBUG("receive HID DATA:%x,%x,%x,%x, enter gtb_generic_com\r\n",send_data_fs[0],send_data_fs[1],send_data_fs[2],send_data_fs[3]);
                TIME_DEBUG("start: %lu ms\r\n", dwt_get_ms());
                gtb_generic_com(&tp_config_hid, send_data_fs, get_data_fs, GTB_HID);
                TIME_DEBUG("end: %lu ms\r\n", dwt_get_ms());
            }
            hid_state_fs = 0;
        }
        osDelay(1);
    }
}
    /* Exported functions --------------------------------------------------------*/
