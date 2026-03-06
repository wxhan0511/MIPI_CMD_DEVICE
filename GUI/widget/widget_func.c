//
// Created by 薛斌 on 24-8-26.
//

#include "widget_func.h"

#include <math.h>
#include <stdio.h>

#include "bsp.h"
#include "cmsis_os2.h"


static lv_obj_t* label_power_name[10];

static const char* power_name[10] = { // 增加长度
    "    Name", "      vsn", "      vsp", "      vcc", "      iovcc",
    "      vdd", "      led", "    elvss", "     avdd", "    elvdd"
};

char* protocol_name[20] =
{
    "MIPI_DPHY_VIDEO      ",
    "MIPI_DPHY_CMD        ",
    "MIPI_CPHY_VIDEO      ",
    "MIPI_CPHY_CMD        ",
    "SPI_3wire_9bit_I     ",
    "SPI_3wire_9bit_II    ",
    "SPI_4wire_8bit_I     ",
    "SPI_4wire_8bit_II    ",
    "MCU_8bit             ",
    "MCU_9bit             ",
    "MCU_16bit            ",
    "MCU_18bit            ",
    "MCU_24bit            ",
    "QSPI                 ",
    "RGB+SPI_3wire_9bit_I ",
    "RGB+SPI_3wire_9bit_II",
    "RGB+SPI_4wire_8bit_I ",
    "RGB+SPI_4wire_8bit_II",
    "RGB+SPI_SSD2828      ",
    "NULL                 ",
};

char* lcd_state[5] = {
    "Power Off",
    "Power On",
    "Sleep Out",
    "Display On",
    "Wait Key",
};

uint8_t fw_version_label[4][4] = {  {0xff,0xff,0xff,0xff},{0xff,0xff,0xff,0xff},{0xff,0xff,0xff,0xff},{0xff,0xff,0xff,0xff}};

static lv_point_t line_h_points[] = {{0, 0}, {0, 150},};
static lv_point_t line_v_points[] = {{0, 0}, {320, 0},};

const int label_height = 20;
const int x_line_0 = 5;
const int y_line_0 = 10;
const int data_step = 80;
const int x_line_1 = 5;
const int y_line_1 = 10 + label_height;
const int x_line_2 = 5;
const int y_line_2 = 10 + label_height * 2;
const int x_line_3 = 5;
const int y_line_3 = 10 + label_height * 3;
const int x_line_4 = 5;
const int y_line_4 = 10 + label_height * 4;
const int x_line_5 = 5;
const int y_line_5 = 10 + label_height * 5;
extern osMutexId_t show_mutexHandle;

static lv_obj_t *page0 = NULL;
static lv_obj_t *page1 = NULL;
static lv_obj_t *page2 = NULL;
lv_obj_t * act_scr = NULL;

#define MAX_PAGE 2

void ui_init_style(lv_style_t* style)
{
    if (style->prop_cnt > 1)
        lv_style_reset(style);
    else
        lv_style_init(style);
}

void ui_label_init(lv_obj_t* label, const int32_t pos_x, const int32_t pos_y, const int32_t width, const int32_t height,
                   const char* text)
{
    lv_label_set_text(label, text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(label, pos_x, pos_y);
    lv_obj_set_size(label, width, height);

    //Write style for first_screen_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    // lv_obj_set_style_border_width(label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_radius(label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_text_opa(label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_text_letter_space(label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_text_line_space(label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_bg_opa(label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_pad_top(label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_pad_right(label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_pad_bottom(label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_pad_left(label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    // lv_obj_set_style_shadow_width(label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    static lv_style_t style1;
    lv_style_init(&style1);
    lv_style_set_text_font(&style1, &lv_font_montserrat_12);
    lv_obj_add_style(label, &style1, LV_STATE_DEFAULT);
}

void ui_label_set_text(lv_obj_t* label, const char* text)
{
    lv_label_set_text(label, text);
}

void ui_main_protocol_group_init(lv_obj_t* page,lcd_protocol_label_group_t* label_group)
{
    //协议标签创建
    lv_obj_t* label_protocol = lv_label_create(page);
    ui_label_init(label_protocol, x_line_0, y_line_0, 80, label_height, "	- Protocol");

    //协议名称标签创建
    label_group->label_protocol = lv_label_create(page);
    ui_label_init(label_group->label_protocol, x_line_0 + data_step, y_line_0, 150, label_height, protocol_name[19]);

    //速度标签创建
    lv_obj_t* label_speed = lv_label_create(page);
    ui_label_init(label_speed, x_line_1, y_line_1, 80, label_height, "	- Speed");

    //Pixel Clock 标签创建
    lv_obj_t* label_pclk = lv_label_create(page);
    ui_label_init(label_pclk, x_line_1 + data_step, y_line_1, 60, label_height, "PCLK");

    //Pixel Clock 数据标签创建
    label_group->label_speed_pclk = lv_label_create(page);
    ui_label_init(label_group->label_speed_pclk, x_line_1 + data_step + 50, y_line_1, 80, label_height, "NULL");

    //MIPI HS 标签创建
    lv_obj_t* label_hs = lv_label_create(page);
    ui_label_init(label_hs, x_line_2 + data_step, y_line_2, 80, label_height, "HS");

    //MIPI HS 数据标签创建
    label_group->label_speed_hs = lv_label_create(page);
    ui_label_init(label_group->label_speed_hs, x_line_2 + data_step + 50, y_line_2, 80, label_height, "1025 Mbps");

    //MIPI LP 标签创建
    lv_obj_t* label_lp = lv_label_create(page);
    ui_label_init(label_lp, x_line_2 + data_step + 130, y_line_2, 80, label_height, "LP");

    //MIPI LP 数据标签创建
    label_group->label_speed_lp = lv_label_create(page);
    ui_label_init(label_group->label_speed_lp, x_line_2 + data_step + 160, y_line_2, 80, label_height, "48.00 Mhz");

    //点屏状态标签创建
    lv_obj_t* label_state = lv_label_create(page);
    ui_label_init(label_state, x_line_3, y_line_3, 80, label_height, "	- State");

    //点屏状态数据标签创建
    label_group->label_state = lv_label_create(page);
    ui_label_init(label_group->label_state, x_line_3 + data_step, y_line_3, 150, label_height, "Power off");
}

void ui_main_sample_data_group_init(lv_obj_t* page,sample_data_label_group_t* label_group)
{
#define STYLE_0 0
# define STYLE_1 1
#if STYLE_0
    static lv_obj_t* line_v[7];
    static lv_obj_t* line_h[7];
    for (int i = 0; i < 7; i++)
    {
        //分割横线
        line_v[i] = lv_line_create(lv_screen_active());
        lv_line_set_points(line_v[i], (lv_point_precise_t*)line_v_points, 2);
        lv_obj_set_pos(line_v[i], 5, 90 + 20 * i);
        lv_obj_set_size(line_v[i], 300, 4);
        lv_obj_set_style_line_color(line_v[i], lv_color_hex(0x000000), LV_PART_MAIN);

        //电压名称标签
        label_power_name[i] = lv_label_create(lv_screen_active());
        ui_label_init(label_power_name[i], 5, 92 + 20 * i, 80, label_height, power_name[i]);
    }

    //名称标签
    lv_obj_t* vol_name_label = lv_label_create(lv_screen_active());
    ui_label_init(vol_name_label, 5 + 89, 92, 80, label_height, "    Vol");

    lv_obj_t* cur_name_label = lv_label_create(lv_screen_active());
    ui_label_init(cur_name_label, 5 + 85 * 2, 92, 80, label_height, "    Cur");

    lv_obj_t* threshold_name_label = lv_label_create(lv_screen_active());
    ui_label_init(threshold_name_label, 5 + 80 * 3, 92, 80, label_height, "Threshold");

    //分割竖线
    for (int i = 0; i < 3; i++)
    {
        line_h[i] = lv_line_create(lv_screen_active());
        lv_line_set_points(line_h[i], (lv_point_precise_t*)line_h_points, 2);
        lv_obj_set_pos(line_h[i], 80 * (i + 1), 90);
        lv_obj_set_size(line_h[i], 4, 140);
        lv_obj_set_style_line_color(line_h[i], lv_color_hex(0x000000), LV_PART_MAIN);
    }

    //采样数据标签
    for (int i = 0; i < 6; i++)
    {
        label_group->label_power_vol[i] = lv_label_create(lv_screen_active());
        ui_label_init(label_group->label_power_vol[i], 10 + 86, 94 + 20 * (i + 1), 76, 16, "null");
        label_group->label_power_cur[i] = lv_label_create(lv_screen_active());
        ui_label_init(label_group->label_power_cur[i], 10 + 83 * 2, 94 + 20 * (i + 1), 76, 16, "null");
    }
#elif STYLE_1
    static lv_obj_t* line_v[7];
    static lv_obj_t* line_h[7];
    for (int i = 0; i < 7; i++)
    {
        //分割横线
        line_v[i] = lv_line_create(page);
        lv_line_set_points(line_v[i], (lv_point_precise_t*)line_v_points, 2);
        lv_obj_set_pos(line_v[i], 5, 90 + 20 * i);
        lv_obj_set_size(line_v[i], 300, 4);
        lv_obj_set_style_line_color(line_v[i], lv_color_hex(0x000000), LV_PART_MAIN);

        //电压名称标签
        label_power_name[i] = lv_label_create(page);
        ui_label_init(label_power_name[i], 5, 92 + 20 * i, 80, label_height, power_name[i]);
    }
    // 在 STYLE_1 的第一个 for 循环 (i < 7) 之后追加
    for (int i = 7; i < 10; i++)
    {
        line_v[i] = lv_line_create(page);
        lv_line_set_points(line_v[i], (lv_point_precise_t*)line_v_points, 2);
        lv_obj_set_pos(line_v[i], 5, 90 + 20 * i);
        lv_obj_set_size(line_v[i], 300, 4);
        lv_obj_set_style_line_color(line_v[i], lv_color_hex(0x000000), LV_PART_MAIN);
        

        // 初始化新增的 3 行电源名称标签 (elvss, avdd, elvdd)
        label_power_name[i] = lv_label_create(page);
        ui_label_init(label_power_name[i], 5, 92 + 20 * i, 80, label_height, power_name[i]);
    }

    //名称标签
    lv_obj_t* vol_name_label = lv_label_create(page);
    ui_label_init(vol_name_label, 80+30, 92, 120, label_height, "    Vol");

    lv_obj_t* cur_name_label = lv_label_create(page);
    ui_label_init(cur_name_label, 200+30, 92, 120, label_height, "    Cur");

    line_h[0] = lv_line_create(page);
    lv_line_set_points(line_h[0], (lv_point_precise_t*)line_h_points, 2);
    lv_obj_set_pos(line_h[0], 80, 90);
    lv_obj_set_size(line_h[0], 4, 140);
    lv_obj_set_style_line_color(line_h[0], lv_color_hex(0x000000), LV_PART_MAIN);

    line_h[1] = lv_line_create(page);
    lv_line_set_points(line_h[1], (lv_point_precise_t*)line_h_points, 2);
    lv_obj_set_pos(line_h[1], 120+80, 90);
    lv_obj_set_size(line_h[1], 4, 140);
    lv_obj_set_style_line_color(line_h[0], lv_color_hex(0x000000), LV_PART_MAIN);

    //采样数据标签
    for (int i = 0; i < 6; i++)
    {
        label_group->label_power_vol[i] = lv_label_create(page);
        ui_label_init(label_group->label_power_vol[i], 80+30, 94 + 20 * (i + 1), 76, 16, "null");
        label_group->label_power_cur[i] = lv_label_create(page);
        ui_label_init(label_group->label_power_cur[i], 200+30, 94 + 20 * (i + 1), 76, 16, "null");
    }
    // 在 STYLE_1 处理采样数据的 for (int i = 0; i < 6; i++) 循环之后追加
    for (int i = 6; i < 9; i++)
    {
        // 初始化新增的 3 路电压值 Label
        label_group->label_power_vol[i] = lv_label_create(page);
        ui_label_init(label_group->label_power_vol[i], 80 + 30, 94 + 20 * (i + 1), 76, 16, "null");

        // 初始化新增的 3 路电流值 Label
        label_group->label_power_cur[i] = lv_label_create(page);
        ui_label_init(label_group->label_power_cur[i], 200 + 30, 94 + 20 * (i + 1), 76, 16, "null");
    }
#endif

}

void ui_refresh_protocol(const lcd_protocol_label_group_t* label_group, const lcd_show_t* lcd_protocol)
{
    ui_label_set_text(label_group->label_protocol, lcd_protocol->protocol);
    ui_label_set_text(label_group->label_speed_pclk, lcd_protocol->speed_pclk);
    ui_label_set_text(label_group->label_speed_hs, lcd_protocol->speed_hs);
    ui_label_set_text(label_group->label_speed_lp, lcd_protocol->speed_lp);
    ui_label_set_text(label_group->label_state, lcd_protocol->state);
}

void ui_refresh_firmware_version(const fw_version_label_group_t* version_group, const lcd_show_t* version_data)
{
    static char temp_str[100];

    sprintf(temp_str, "v %x.%x.%x.%x", version_data->version[0]>>24&0xff,
        version_data->version[0]>>16&0xff, version_data->version[0]>>8&0xff, version_data->version[0]&0xff);
    ui_label_set_text(version_group->label_meter_bl_version,temp_str);

    sprintf(temp_str, "v %x.%x.%x.%x", version_data->version[1]>>24&0xff,
    version_data->version[1]>>16&0xff, version_data->version[1]>>8&0xff, version_data->version[1]&0xff);
    ui_label_set_text(version_group->label_meter_version,temp_str);

    sprintf(temp_str, "v %x.%x.%x.%x", version_data->version[2]>>24&0xff,
    version_data->version[2]>>16&0xff, version_data->version[2]>>8&0xff, version_data->version[2]&0xff);
    ui_label_set_text(version_group->label_main_bl_version,temp_str);

    sprintf(temp_str, "v %x.%x.%x.%x", version_data->version[3]>>24&0xff,
    version_data->version[3]>>16&0xff, version_data->version[3]>>8&0xff, version_data->version[3]&0xff);
    ui_label_set_text(version_group->label_main_version,temp_str);

    sprintf(temp_str, "v %x.%x.%x.%x", version_data->version[4]>>24&0xff,
    version_data->version[4]>>16&0xff, version_data->version[4]>>8&0xff, version_data->version[4]&0xff);
    ui_label_set_text(version_group->label_meter_hw_num,temp_str);

    sprintf(temp_str, "v %x.%x.%x.%x", version_data->version[5]>>24&0xff,
    version_data->version[5]>>16&0xff, version_data->version[5]>>8&0xff, version_data->version[5]&0xff);
    ui_label_set_text(version_group->label_main_hw_num,temp_str);
}

void ui_main_protocol_init(lcd_show_t* lcd_protocol)
{
    lcd_protocol->protocol = protocol_name[19];
    lcd_protocol->speed_hs = "NULL";
    lcd_protocol->speed_lp = "NULL";
    lcd_protocol->speed_pclk = "NULL";
    lcd_protocol->state = "Power off";
}

void ui_refresh_sample_data(const sample_data_label_group_t *label_group, const lcd_show_t* lcd_protocol)
{
    //osMutexAcquire(&show_mutexHandle,0);
    static char temp_str[100];
    for (int i = 0; i < 6; i++)
    {
        if (fabs(lcd_protocol->voltage[i]) >= 1000)
        {
            sprintf(temp_str, "%.3f V", lcd_protocol->voltage[i] / 1000);
        }
        else if (fabs(lcd_protocol->voltage[i]) >= 1)
            {
            sprintf(temp_str, "%d mV", (uint32_t)lcd_protocol->voltage[i]);
        }
        ui_label_set_text(label_group->label_power_vol[i], temp_str);
        // if(i == 0)
        //     printf("vsn voltage %f \r\n",lcd_protocol->voltage[i]);
    }

    for (int i = 0; i < 6; i++)
    {
        if (fabs(lcd_protocol->current[i]) >= 1000000)
        {
            sprintf(temp_str, "%.2f A", lcd_protocol->current[i] / 1000000);
        }
        else if (fabs(lcd_protocol->current[i]) >= 1000)
        {
            sprintf(temp_str, "%.2f mA", lcd_protocol->current[i] / 1000);
        }
        else
        {
            sprintf(temp_str, "%.2f uA", lcd_protocol->current[i]);
        }
        ui_label_set_text(label_group->label_power_cur[i], temp_str);
    }
    //osMutexRelease(&show_mutexHandle);
}

void ui_set_protocol(lcd_show_t* lcd_show,char *protocol,char* pclk,char* hs,char* lp,char* state)
{
    osMutexAcquire(&show_mutexHandle,0);
    if(protocol != NULL)
        lcd_show->protocol = protocol;
    if(pclk != NULL)
        lcd_show->speed_pclk = pclk;
    if(hs != NULL)
        lcd_show->speed_hs = hs;
    if(lp != NULL)
        lcd_show->speed_lp = lp;
    if(state != NULL)
        lcd_show->state = state;

    osMutexRelease(&show_mutexHandle);
}

void ui_set_sample_voltage(lcd_show_t* lcd_show, const double* voltage)
{
    osMutexAcquire(&show_mutexHandle,0);

    memcpy(lcd_show->voltage,voltage,sizeof(double) * 6);

    osMutexRelease(&show_mutexHandle);
}

void ui_set_sample_current(lcd_show_t* lcd_show, const double* current)
{
    osMutexAcquire(&show_mutexHandle,0);

    memcpy(lcd_show->voltage,current,sizeof(double) * 6);

    osMutexRelease(&show_mutexHandle);
}

void ui_set_sample_threshold(lcd_show_t* lcd_show, const double* threshold)
{
    osMutexAcquire(&show_mutexHandle,0);

    memcpy(lcd_show->voltage,threshold,sizeof(double) * 6);

    osMutexRelease(&show_mutexHandle);
}

void ui_clean_screen(lv_obj_t* page)
{
    lv_obj_t * act_scr = page;
    lv_obj_clean(act_scr);
}

void ui_open_machine(open_machine_group_t *label_group)
{
    page0 = lv_obj_create(NULL);
    // lv_obj_set_style_bg_color(page0, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    // lv_obj_set_style_text_color(page0, lv_color_hex(0x000000), LV_PART_MAIN);

    label_group->label_logo = lv_label_create(page0);
    lv_label_set_text(label_group->label_logo, "GC4.0");
    lv_label_set_long_mode(label_group->label_logo, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(label_group->label_logo, 90, 100);
    lv_obj_set_size(label_group->label_logo, 200, 50);
    static lv_style_t style1;
    lv_style_init(&style1);
    lv_style_set_text_font(&style1, &lv_font_montserrat_48);

    lv_obj_add_style(label_group->label_logo, &style1, LV_STATE_DEFAULT);


    lv_screen_load_anim(page0, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
}

void ui_main_init(lcd_show_t* lcd_protocol,lcd_protocol_label_group_t* p_group,sample_data_label_group_t* s_group)
{
    //背景主题
    page1 = lv_obj_create(NULL);
    lv_obj_set_size(page1, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(page1, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_color(page1, lv_color_hex(0x000000), LV_PART_MAIN);

    //main界面组建初始化

    ui_main_protocol_init(lcd_protocol);
    ui_main_protocol_group_init(page1,p_group);
    ui_main_sample_data_group_init(page1,s_group);
}

void ui_sub_init(fw_version_label_group_t *version_group)
{
    page2 = lv_obj_create(NULL);
    //lv_obj_add_flag(page2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(page2, LV_HOR_RES, LV_VER_RES);
    // lv_obj_set_style_bg_color(page1, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    // lv_obj_set_style_text_color(page1, lv_color_hex(0x000000), LV_PART_MAIN);

    // ui_main_protocol_init(&lcd_show);
    // ui_main_protocol_group_init(page2,&lcd_protocol_group);
    // ui_main_sample_data_group_init(page2,&sample_data_group);

    //lv_screen_load_anim(page2, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);

    //协议标签创建
    lv_obj_t* label_0 = lv_label_create(page2);
    ui_label_init(label_0, x_line_0, y_line_0, 160, label_height, "	- Meter BL Version");
    lv_obj_t* label_1 = lv_label_create(page2);
    ui_label_init(label_1, x_line_0, y_line_1, 160, label_height, "	- Meter Version");
    lv_obj_t* label_2 = lv_label_create(page2);
    ui_label_init(label_2, x_line_0, y_line_2, 160, label_height, "	- Main BL Version");
    lv_obj_t* label_3 = lv_label_create(page2);
    ui_label_init(label_3, x_line_0, y_line_3, 160, label_height, "	- Main Version");
    lv_obj_t* label_4 = lv_label_create(page2);
    ui_label_init(label_4, x_line_0, y_line_4, 160, label_height, "	- Meter Hardware Num");
    lv_obj_t* label_5 = lv_label_create(page2);
    ui_label_init(label_5, x_line_0, y_line_5, 160, label_height, "	- Main Hardware Num");

    version_group->label_meter_bl_version = lv_label_create(page2);
    ui_label_init(version_group->label_meter_bl_version, x_line_0 + 160, y_line_0, 160, label_height, "-.-.-.-");
    version_group->label_meter_version = lv_label_create(page2);
    ui_label_init(version_group->label_meter_version, x_line_1 + 160, y_line_1, 160, label_height, "-.-.-.-");
    version_group->label_main_bl_version = lv_label_create(page2);
    ui_label_init(version_group->label_main_bl_version, x_line_2 + 160, y_line_2, 160, label_height, "-.-.-.-");
    version_group->label_main_version = lv_label_create(page2);
    ui_label_init(version_group->label_main_version, x_line_3 + 160, y_line_3, 160, label_height, "-.-.-.-");
    version_group->label_meter_hw_num = lv_label_create(page2);
    ui_label_init(version_group->label_meter_hw_num, x_line_4 + 160, y_line_4, 160, label_height, "-.-.-.-");
    version_group->label_main_hw_num = lv_label_create(page2);
    ui_label_init(version_group->label_main_hw_num, x_line_5 + 160, y_line_5, 160, label_height, "-.-.-.-");
}

void widget_change()
{
    taskENTER_CRITICAL();
    extern __IO uint8_t current_page;
    uint8_t temp = current_page;
    temp += 1;
    if(temp >= MAX_PAGE)
    {
        temp = 0;
    }
    //printf("current_page = %d\r\n",temp);
    if (temp == PAGE_0)
    {
        lv_screen_load_anim(page1, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false);
    }
    else if (temp == PAGE_1)
    {
        lv_screen_load_anim(page2, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false);
    }
    current_page = temp;
    taskEXIT_CRITICAL();
}

void ui_load_page_1()
{
    lv_screen_load_anim(page1, LV_SCR_LOAD_ANIM_OVER_LEFT, 100, 100, false);
}