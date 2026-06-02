//
// Created by 薛斌 on 24-8-26.
//

#include "widget_main.h"
#include <stdio.h>
#include "bsp.h"
#include "bsp_in_flash.h"
#include "bsp_log.h"
#include "cmsis_os2.h"
#include "lv_port_disp_template.h"
#include "widget_func.h"

extern osMutexId_t show_mutexHandle;

osThreadId_t widget_main_flush_task_handle;
const osThreadAttr_t widget_main_flush_task_attributes = {
	.name = "widget_main_flush_task_handle",
	.stack_size = 1024 * 4,
	.priority = (osPriority_t)osPriorityNormal1,
};

open_machine_group_t open_machine_group;
lcd_protocol_label_group_t lcd_protocol_group;
lcd_protocol_label_group_t lcd_protocol_group_page3;
sample_data_label_group_t sample_data_group;
sample_data_label_group_t sample_data_group_roate;
sample_data_page3_label_group_t sample_data_group_page3;
sample_data_page3_label_group_t sample_data_group_page3_roate;
fw_version_label_group_t fw_version_group;
// lcd_show_t lcd_show;
lcd_show_t lcd_show_page3;

__IO uint8_t current_page = 0;
__IO uint8_t open_en = 0;

lv_timer_t *open_machine_task;
static lv_timer_t *update_data_task;

extern lv_obj_t *act_scr;
lcd_show_t lcd_show = {
	.protocol = "MIPI-DSI",
	.speed_pclk = "320MHz",
	.speed_hs = "850Mbps",
	.speed_lp = "10Mbps",
	.state = "RUNNING",
	.current = {0.005, 0.012, 0.000, 0.025, 0.007, 0.003, 0.018, 0.010, 0.001, 0.002, 0.004},
	.current_gear = {0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0},
	.voltage = {3.300, 1.800, 2.500, 5.000, 3.300, 1.200, 4.200, 2.800, 0.000, 3.300, 1.800, 2.500, 5.000, 3.300, 1.200},
	.voltage_gear = {1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0},
	.version = {0x01020300, 0x01020301, 0x01020302, 0x01020303, 0x01020304, 0x01020305, 0x01020306, 0x01020307, 0x01020308}};
void widget_main_create(void)
{
	ui_open_machine(&open_machine_group);
	ui_main_init(&lcd_show, &lcd_protocol_group, &sample_data_group);
	ui_main_rotate_init(&lcd_show, &lcd_protocol_group, &sample_data_group_roate);
	ui_page3_init(&lcd_show_page3, &lcd_protocol_group_page3, &sample_data_group_page3);
	ui_page3_rotate_init(&lcd_show_page3, &lcd_protocol_group_page3, &sample_data_group_page3_roate);
	ui_sub_init(&fw_version_group);
	// lcd_show.version[0] = {0xff,0xff,0xff,0xff};
	uint32_t boot_version = 0;
	mem_read(0x0801FD7D, &boot_version, 1);
	const uint32_t cpu_sn_0 = *(__IO uint32_t *)(0x1FFF7A10);
	lcd_show.version[0] = boot_version;
	lcd_show.version[1] = sw_version[0] << 24 | sw_version[1] << 16 | sw_version[2] << 8 | sw_version[3];
	lcd_show.version[2] = 0xf1f2f3f4;
	lcd_show.version[3] = 0xf1f2f3f4;
	lcd_show.version[4] = cpu_sn_0;
	lcd_show.version[5] = 0xf1f2f3f4;
	current_page = PAGE_0;
}

void widget_flush_timer_cb(const lv_timer_t *timer)
{
	taskENTER_CRITICAL();
	// printf("widget flush timer called\r\n");
	const lcd_show_t *show_buf = timer->user_data;
	if (open_en == 1)
	{
		printf("widget flush timer called, open_en: %d\r\n", open_en);
		if (current_page == 0)
		{
			// printf("flush page 0\r\n");
			// printf("show_buf protocol: %s\r\n", show_buf->protocol);
			// for (int i = 0; i < 9; i++)
			// {
			// 	printf("show_buf voltage[%d]: %f\r\n", i, show_buf->voltage[i]);
			// }
			// 刷新协议
			ui_refresh_protocol(&lcd_protocol_group, show_buf);

			// 刷新采样数据````````
			ui_refresh_sample_data(&sample_data_group, show_buf);
			ui_refresh_sample_data(&sample_data_group_roate, show_buf);
		}
		else if (current_page == 2)
		{
			// printf("flush page 2\r\n");
			// printf("show_buf protocol: %s\r\n", show_buf->protocol);
			// for (int i = 0; i < 9; i++)
			// {
			// 	printf("show_buf voltage[%d]: %f\r\n", i, show_buf->voltage[i]);
			// }
			// 刷新协议
			ui_refresh_protocol(&lcd_protocol_group_page3, show_buf);
			// 刷新采样数据````````
			ui_refresh_sample_data_page3(&sample_data_group_page3, show_buf);
			ui_refresh_sample_data_page3(&sample_data_group_page3_roate, show_buf);
		}
		else if (current_page == 1)
		{
			// printf("flush page 1\r\n");
			ui_refresh_firmware_version(&fw_version_group, show_buf);
		}
	}
	// printf("widget flush timer called end\r\n");
	taskEXIT_CRITICAL();
}

void open_machine_widget_jump(const lv_timer_t *timer)
{
	taskENTER_CRITICAL();
	printf("open machine jump\r\n");
	open_en = 1;
	lv_timer_delete(open_machine_task);
	extern void ui_load_page_1();
	ui_load_page_1();
	taskEXIT_CRITICAL();
}

void lvgl_timer_task_entry(void *params)
{
	osDelay(1000);
	lv_init();
	lv_port_disp_init();
	printf("lvgl_timer_task_entry running\r\n");
	// 创建开机画面转跳定时器

	open_machine_task = lv_timer_create((lv_timer_cb_t)open_machine_widget_jump, 3000, &lcd_show);
	lv_timer_enable(true);
	// lv_timer_ready(open_machine_task);
	// lv_timer_delete(update_data_task);

	// 创建页面刷新定时器
	update_data_task = lv_timer_create((lv_timer_cb_t)widget_flush_timer_cb, 100, &lcd_show);
	lv_timer_enable(true);
	lv_timer_ready(update_data_task);
	while (1)
	{
		if (osMutexAcquire(show_mutexHandle, osWaitForever) == osOK)
		{
			printf("lvgl_timer_task_entry acquired show_mutex\r\n");
			lv_timer_handler();
			osMutexRelease(show_mutexHandle);
		}
		osDelay(10);
	}
}

void widget_main_task_init()
{
	widget_main_flush_task_handle = osThreadNew(lvgl_timer_task_entry, NULL, &widget_main_flush_task_attributes);
	if (widget_main_flush_task_handle == NULL)
	{
		// LOG_ERROR("widget init Error!");
	}
}
