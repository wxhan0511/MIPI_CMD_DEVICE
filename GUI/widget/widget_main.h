//
// Created by 薛斌 on 24-8-26.
//

#ifndef WIDGET_MAIN_H
#define WIDGET_MAIN_H

#include <lvgl.h>

void widget_main_task_init(void);

void widget_main_create(void);

void widget_main_update_show_buf_vol(const double *vol);

void widget_main_update_show_buf_cur(const double *cur);



#endif //WIDGET_MAIN_H
