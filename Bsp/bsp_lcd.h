//
// Created by 薛斌 on 24-8-16.
//

#ifndef BSP_LCD_H
#define BSP_LCD_H

#include <stm32f4xx_hal.h>

#define LCD_FSMC_NEX         1              //CS
#define LCD_FSMC_AX          16             //RS



typedef struct
{
    __IO uint16_t reg;
    __IO uint16_t ram;

}lcd_dev_t;

#define LCD_BASE        (uint32_t)((0x60000000 + (0x4000000 * (LCD_FSMC_NEX - 1))) | (((1 << LCD_FSMC_AX) * 2) -2))
#define LCD             ((lcd_dev_t *) LCD_BASE)

extern lcd_dev_t lcd;

void bsp_lcd_init(lcd_dev_t *dev);

void bsp_lcd_reset(lcd_dev_t *dev);

void bsl_lcd_backlight(lcd_dev_t *dev,uint8_t en);

void bsp_lcd_write_cmd(lcd_dev_t *dev,uint16_t cmd);

void bsp_lcd_write_data(lcd_dev_t *dev,uint16_t data);

uint16_t bsp_lcd_read_data(lcd_dev_t *dev);

void bsp_lcd_block_write(lcd_dev_t *dev,uint16_t x_start,uint16_t x_end,uint16_t y_start,uint16_t y_end);


#endif //BSP_LCD_H
