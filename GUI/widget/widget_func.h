//
// Created by 薛斌 on 24-8-26.
//

#ifndef WIDGET_FUNC_H
#define WIDGET_FUNC_H

#include "lvgl.h"

typedef struct
{
    char *protocol;
    char *speed_pclk;
    char *speed_hs;
    char *speed_lp;
    char *state;

    double current[6];
    uint8_t current_gear[6];
    double voltage[6];
    uint8_t voltage_gear[6];
    //double threshold[6];
    uint32_t version[6];
} lcd_show_t;

extern lcd_show_t lcd_show;

typedef struct
{
    lv_obj_t* label_protocol;
    lv_obj_t* label_speed_pclk;
    lv_obj_t* label_speed_hs;
    lv_obj_t* label_speed_lp;
    lv_obj_t* label_state;
} lcd_protocol_label_group_t;

typedef struct
{
    lv_obj_t* label_power_vol[9];
    lv_obj_t* label_power_cur[9];
    lv_obj_t* label_power_th[9];
} sample_data_label_group_t;

typedef struct
{
    lv_obj_t* label_meter_version;
    lv_obj_t* label_meter_bl_version;
    lv_obj_t* label_main_version;
    lv_obj_t* label_main_bl_version;
    lv_obj_t* label_meter_hw_num;
    lv_obj_t* label_main_hw_num;
} fw_version_label_group_t;


typedef struct
{
    lv_obj_t* label_logo;
} open_machine_group_t;
typedef enum
{
    PAGE_0,
    PAGE_1,
}PAGE_ID;

typedef struct
{
    uint8_t page_id;
    lv_obj_t* root;
}page_t;

extern char* protocol_name[20];

void ui_init_style(lv_style_t* style);

void ui_label_init(lv_obj_t* label, const int32_t pos_x, const int32_t pos_y, const int32_t width, const int32_t height,
                   const char* text);

void ui_main_protocol_init(lcd_show_t* lcd_protocol);

void ui_main_protocol_group_init(lv_obj_t* page,lcd_protocol_label_group_t* label_group);

void ui_main_sample_data_group_init(lv_obj_t* page,sample_data_label_group_t* label_group);

void ui_refresh_firmware_version(const fw_version_label_group_t* version_group, const lcd_show_t* version_data);

void ui_refresh_protocol(const lcd_protocol_label_group_t* label_group, const lcd_show_t* lcd_protocol);

void ui_refresh_sample_data(const sample_data_label_group_t *label_group, const lcd_show_t* lcd_protocol);

void ui_set_protocol(lcd_show_t* lcd_show,char *protocol,char* pclk,char* hs,char* lp,char* state);

void ui_set_sample_voltage(lcd_show_t* lcd_show, const double* voltage);

void ui_set_sample_current(lcd_show_t* lcd_show, const double* current);

void ui_set_sample_threshold(lcd_show_t* lcd_show, const double* threshold);

void ui_clean_screen();

void ui_open_machine(open_machine_group_t *label_group);

void ui_main_init(lcd_show_t* lcd_protocol,lcd_protocol_label_group_t* p_group,sample_data_label_group_t* s_group);

void ui_sub_init(fw_version_label_group_t *version_group);

void widget_change();

#endif //WIDGET_FUNC_H
