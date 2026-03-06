/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/

#include <math.h>

#include "bsp_lcd.h"
#include "lcd.h"
#include "widget_main.h"
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp_template.h"
#include <stdbool.h>

/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES 320
#define MY_DISP_VER_RES 240

#ifndef MY_DISP_HOR_RES
#warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
#define MY_DISP_HOR_RES    320
#endif

#ifndef MY_DISP_VER_RES
#warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen height, default value 240 is used for now.
#define MY_DISP_VER_RES    240
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/


/**********************
 *   GLOBAL FUNCTIONS
 **********************/


void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t* disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);


    lv_display_set_flush_cb(disp, disp_flush);
    static lv_color_t buf_1_1[MY_DISP_HOR_RES * 10]; /*A buffer for 10 rows*/
    static lv_color_t buf_1_2[MY_DISP_HOR_RES * 10]; /*A buffer for 10 rows*/
    lv_display_set_buffers(disp, buf_1_1, buf_1_2, sizeof(buf_1_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    //disp_enable_update();

    widget_main_create();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
    // bsp_lcd_init();
    lcd_init();

}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/
void lcd_draw_fast_rgb_color(int16_t sx, int16_t sy, int16_t ex, int16_t ey, uint16_t* color)
{
    uint16_t w = ex - sx + 1;
    uint16_t h = ey - sy + 1;

    //lcd_set_window(sx, sy, w, h);
    uint32_t draw_size = w * h;
    // lcd_write_ram_prepare();
   lcd_set_cursor_address(sx, sy, ex, ey);
    for (uint32_t i = 0; i < draw_size; i++)
    {
        lcd_wr_data(color[i] >> 8);
        lcd_wr_data(color[i] & 0xff);

        // LCD->LCD_RAM = color[i] >> 8;
        // LCD->LCD_RAM = color[i] & 0xff;
    }
}

int cnt = 0;
static void disp_flush(lv_display_t* disp_drv, const lv_area_t* area, uint8_t* px_map)
{
    // if(disp_flush_enabled) {
    //     /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one*/
    //
    //     int32_t x;
    //     int32_t y;
    //     for(y = area->y1; y <= area->y2; y++) {
    //         for(x = area->x1; x <= area->x2; x++) {
    //             /*Put a pixel to the display. For example:*/
    //             /*put_px(x, y, *px_map)*/
    //             px_map++;
    //         }
    //     }
    // }
    //px_map += LV_HOR_RES * 20 * cnt * 3;
    //printf("%d %d %d %d ，%p\r\n",area->x1,area->y1,area->x2,area->y2,px_map);
    lcd_draw_fast_rgb_color(area->x1, area->y1, area->x2, area->y2, (uint16_t*)px_map);
    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_display_flush_ready(disp_drv);
    // cnt++;
    // if(cnt == 8)
    //  cnt = 0;
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
