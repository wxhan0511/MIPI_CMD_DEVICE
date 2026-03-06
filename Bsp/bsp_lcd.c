//
// Created by 薛斌 on 24-8-16.
//

#include "bsp_lcd.h"

#include "cmsis_os2.h"
#include "main.h"
#include "tim.h"
lcd_dev_t lcd;

static void bsp_lcd_opt_delay(uint32_t i)
{
    while (i--);
}

void bsp_lcd_init(lcd_dev_t *dev)
{

	//背光
	// HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
	//
	// //reset
	// bsp_lcd_reset(dev);

#ifdef DEMO_BOARD
    bsp_lcd_write_cmd(dev,0xfe);
    bsp_lcd_write_cmd(dev,0xfe);
    bsp_lcd_write_cmd(dev,0xef);
    bsp_lcd_write_cmd(dev,0x36);
    //	bsp_lcd_write_data(dev,0x48); 
    bsp_lcd_write_data(dev,0x28); 
	
    bsp_lcd_write_cmd(dev,0x3a);
    bsp_lcd_write_data(dev,0x05);
			
    bsp_lcd_write_cmd(dev,0x86);	
    bsp_lcd_write_data(dev,0x98);
    bsp_lcd_write_cmd(dev,0x89);	
    bsp_lcd_write_data(dev,0x03);
    bsp_lcd_write_cmd(dev,0x8b);	
    bsp_lcd_write_data(dev,0x80);	
    bsp_lcd_write_cmd(dev,0x8d);	
    bsp_lcd_write_data(dev,0x33);
    bsp_lcd_write_cmd(dev,0x8e);	 
    bsp_lcd_write_data(dev,0x0f);


    bsp_lcd_write_cmd(dev,0xe8);
    bsp_lcd_write_data(dev,0x12);
    bsp_lcd_write_data(dev,0x00);
		
    bsp_lcd_write_cmd(dev,0xc3);	
    bsp_lcd_write_data(dev,0x1d);
    bsp_lcd_write_cmd(dev,0xc4);	
    bsp_lcd_write_data(dev,0x1d);
    bsp_lcd_write_cmd(dev,0xc9);	
    bsp_lcd_write_data(dev,0x0f);
		
    bsp_lcd_write_cmd(dev,0xff);
    bsp_lcd_write_data(dev,0x62);

    bsp_lcd_write_cmd(dev,0x99);	
    bsp_lcd_write_data(dev,0x3e);
    bsp_lcd_write_cmd(dev,0x9d);	
    bsp_lcd_write_data(dev,0x4b);
    bsp_lcd_write_cmd(dev,0x98);	
    bsp_lcd_write_data(dev,0x3e);
    bsp_lcd_write_cmd(dev,0x9c);	
    bsp_lcd_write_data(dev,0x4b);

    bsp_lcd_write_cmd(dev,0xF0);
    bsp_lcd_write_data(dev,0x49);
    bsp_lcd_write_data(dev,0x0b);
    bsp_lcd_write_data(dev,0x09);
    bsp_lcd_write_data(dev,0x08);
    bsp_lcd_write_data(dev,0x06);
    bsp_lcd_write_data(dev,0x2e);

    bsp_lcd_write_cmd(dev,0xF2);
    bsp_lcd_write_data(dev,0x49);
    bsp_lcd_write_data(dev,0x0b);
    bsp_lcd_write_data(dev,0x09);
    bsp_lcd_write_data(dev,0x08);
    bsp_lcd_write_data(dev,0x06);
    bsp_lcd_write_data(dev,0x2e);

    bsp_lcd_write_cmd(dev,0xF1);
    bsp_lcd_write_data(dev,0x45);
    bsp_lcd_write_data(dev,0x92);
    bsp_lcd_write_data(dev,0x93);
    bsp_lcd_write_data(dev,0x2b);
    bsp_lcd_write_data(dev,0x31);
    bsp_lcd_write_data(dev,0x6F);

    bsp_lcd_write_cmd(dev,0xF3);
    bsp_lcd_write_data(dev,0x45);
    bsp_lcd_write_data(dev,0x92);
    bsp_lcd_write_data(dev,0x93);
    bsp_lcd_write_data(dev,0x2b);
    bsp_lcd_write_data(dev,0x31);
    bsp_lcd_write_data(dev,0x6F);

    bsp_lcd_write_cmd(dev,0x35);
    bsp_lcd_write_data(dev,0x00);

    bsp_lcd_write_cmd(dev,0x11);
    HAL_Delay(120);
    bsp_lcd_write_cmd(dev,0x29);
    bsp_lcd_write_cmd(dev,0x2c);
#else
	/*  Power control B (CFh)  */
	bsp_lcd_opt_delay(1);
	bsp_lcd_write_cmd(dev,0xCF  );
	bsp_lcd_write_data(dev,0x00  );
	bsp_lcd_write_data(dev,0x81  );
	bsp_lcd_write_data(dev,0x30  );	
	/*  Power on sequence control (EDh) */
	bsp_lcd_opt_delay(1);
	bsp_lcd_write_cmd(dev,0xED );
	bsp_lcd_write_data(dev,0x64 );
	bsp_lcd_write_data(dev,0x03 );
	bsp_lcd_write_data(dev,0x12 );
	bsp_lcd_write_data(dev,0x81 );
	/*  Driver timing control A (E8h) */
	bsp_lcd_opt_delay(1);
	bsp_lcd_write_cmd(dev,0xE8 );
	bsp_lcd_write_data(dev,0x85 );
	bsp_lcd_write_data(dev,0x10 );
	bsp_lcd_write_data(dev,0x78 );	
	/*  Power control A (CBh) */
	bsp_lcd_opt_delay(1);
	bsp_lcd_write_cmd(dev,0xCB );
	bsp_lcd_write_data(dev,0x39 );
	bsp_lcd_write_data(dev,0x2C );
	bsp_lcd_write_data(dev,0x00 );
	bsp_lcd_write_data(dev,0x34 );
	bsp_lcd_write_data(dev,0x02 );
	/* Pump ratio control (F7h) */
	bsp_lcd_opt_delay(1);
	bsp_lcd_write_cmd(dev,0xF7 );
	bsp_lcd_write_data(dev,0x20 );
	/* Driver timing control B */
	bsp_lcd_opt_delay(1);
	bsp_lcd_write_cmd(dev,0xEA );
	bsp_lcd_write_data(dev,0x00 );
	bsp_lcd_write_data(dev,0x00 );	
	/* Frame Rate Control (In Normal Mode/Full Colors) (B1h) */
	bsp_lcd_opt_delay(1);
	bsp_lcd_write_cmd(dev,0xB1 );
	bsp_lcd_write_data(dev,0x00 );
	bsp_lcd_write_data(dev,0x1B );
	/*  Display Function Control (B6h) */
	bsp_lcd_opt_delay(1);
	bsp_lcd_write_cmd(dev,0xB6 );
	bsp_lcd_write_data(dev,0x0A );
	bsp_lcd_write_data(dev,0xA2 );
	/* Power Control 1 (C0h) */
	bsp_lcd_opt_delay(1);
	bsp_lcd_write_cmd(dev,0xC0 );
	bsp_lcd_write_data(dev,0x35 );
	/* Power Control 2 (C1h) */
	bsp_lcd_opt_delay(1);
	bsp_lcd_write_cmd(dev,0xC1 );
	bsp_lcd_write_data(dev,0x11 );
	/* VCOM Control 1 (C5h) */
	bsp_lcd_write_cmd(dev,0xC5 );
	bsp_lcd_write_data(dev,0x45 );
	bsp_lcd_write_data(dev,0x45 );
	/*  VCOM Control 2 (C7h)  */
	bsp_lcd_write_cmd(dev,0xC7 );
	bsp_lcd_write_data(dev,0xA2 );
	/* Enable 3G (F2h) */
	bsp_lcd_write_cmd(dev,0xF2 );
	bsp_lcd_write_data(dev,0x00 );
	/* Gamma Set (26h) */
	bsp_lcd_write_cmd(dev,0x26 );
	bsp_lcd_write_data(dev,0x01 );
	bsp_lcd_opt_delay(1);
	/* Positive Gamma Correction */
	bsp_lcd_write_cmd(dev,0xE0 ); //Set Gamma
	bsp_lcd_write_data(dev,0x0F );
	bsp_lcd_write_data(dev,0x26 );
	bsp_lcd_write_data(dev,0x24 );
	bsp_lcd_write_data(dev,0x0B );
	bsp_lcd_write_data(dev,0x0E );
	bsp_lcd_write_data(dev,0x09 );
	bsp_lcd_write_data(dev,0x54 );
	bsp_lcd_write_data(dev,0xA8 );
	bsp_lcd_write_data(dev,0x46 );
	bsp_lcd_write_data(dev,0x0C );
	bsp_lcd_write_data(dev,0x17 );
	bsp_lcd_write_data(dev,0x09 );
	bsp_lcd_write_data(dev,0x0F );
	bsp_lcd_write_data(dev,0x07 );
	bsp_lcd_write_data(dev,0x00 );
	
	/* Negative Gamma Correction (E1h) */
	bsp_lcd_write_cmd(dev,0XE1 ); //Set Gamma
	bsp_lcd_write_data(dev,0x00 );
	bsp_lcd_write_data(dev,0x19 );
	bsp_lcd_write_data(dev,0x1B );
	bsp_lcd_write_data(dev,0x04 );
	bsp_lcd_write_data(dev,0x10 );
	bsp_lcd_write_data(dev,0x07 );
	bsp_lcd_write_data(dev,0x2A );
	bsp_lcd_write_data(dev,0x47 );
	bsp_lcd_write_data(dev,0x39 );
	bsp_lcd_write_data(dev,0x03 );
	bsp_lcd_write_data(dev,0x06 );
	bsp_lcd_write_data(dev,0x06 );
	bsp_lcd_write_data(dev,0x30 );
	bsp_lcd_write_data(dev,0x38 );
	bsp_lcd_write_data(dev,0x0F );
	
	/* memory access control set */
	bsp_lcd_opt_delay(1);
	bsp_lcd_write_cmd(dev,0x36 ); 	
	bsp_lcd_write_data(dev,0xC8 );    /*����  ���Ͻǵ� (���)�����½� (�յ�)ɨ�跽ʽ*/
	bsp_lcd_opt_delay(1);
	
	/* column address control set */
	bsp_lcd_write_cmd(dev,0x2a );
	bsp_lcd_write_data(dev,0x00 );
	bsp_lcd_write_data(dev,0x00 );
	bsp_lcd_write_data(dev,0x00 );
	bsp_lcd_write_data(dev,0xEF );
	
	/* page address control set */
	bsp_lcd_opt_delay(1);
	bsp_lcd_write_cmd(dev,0x2b );
	bsp_lcd_write_data(dev,0x00 );
	bsp_lcd_write_data(dev,0x00 );
	bsp_lcd_write_data(dev,0x01 );
	bsp_lcd_write_data(dev,0x3F );
	
	/*  Pixel Format Set (3Ah)  */
	bsp_lcd_opt_delay(1);
	bsp_lcd_write_cmd(dev,0x3a ); 
	bsp_lcd_write_data(dev,0x55 );
	
	/* Sleep Out (11h)  */
	bsp_lcd_write_cmd(dev,0x11 );	
	bsp_lcd_opt_delay ( 0xAFFf<<2 );
	bsp_lcd_opt_delay(1);
	/* Display ON (29h) */
	bsp_lcd_write_cmd(dev,0x29 ); 
#endif
    
}

void bsp_lcd_reset(lcd_dev_t *dev)
{

	HAL_GPIO_WritePin(LCD_RESET_GPIO_Port,LCD_RESET_Pin,GPIO_PIN_SET);
	app_delay(100);
	HAL_GPIO_WritePin(LCD_RESET_GPIO_Port,LCD_RESET_Pin,GPIO_PIN_RESET);
	app_delay(100);
	HAL_GPIO_WritePin(LCD_RESET_GPIO_Port,LCD_RESET_Pin,GPIO_PIN_SET);
	app_delay(100);
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_SET);
}

void bsl_lcd_backlight(lcd_dev_t *dev,uint8_t en)
{

}

///
/// @param dev lcd
/// @param cmd 寄存器地址
void bsp_lcd_write_cmd(lcd_dev_t *dev,uint16_t cmd)
{
    if(dev == NULL)
        return;
    cmd = cmd;
    LCD->reg = cmd;
}

///
/// @param dev
/// @param data 寄存器值
void bsp_lcd_write_data(lcd_dev_t *dev,uint16_t data)
{
    if(dev == NULL)
        return;
    data = data;
    dev->reg = data;
}

///
/// @param dev
/// @return
uint16_t bsp_lcd_read_data(lcd_dev_t *dev)
{
    __IO uint16_t data;
    bsp_lcd_opt_delay(2);
    data = LCD->ram;
    return data;
}

/// 小窗口模式
/// @param dev
/// @param x_start
/// @param x_end
/// @param y_start
/// @param y_end
void bsp_lcd_block_write(lcd_dev_t *dev,uint16_t x_start,uint16_t x_end,uint16_t y_start,uint16_t y_end)
{
    bsp_lcd_write_cmd(dev,0x2a);
    bsp_lcd_write_data(dev,x_start >> 8);
    bsp_lcd_write_data(dev,x_start & 0xff);
    bsp_lcd_write_data(dev,x_end >> 8);
    bsp_lcd_write_data(dev,x_end & 0xff);

    bsp_lcd_write_cmd(dev,0x2b);
    bsp_lcd_write_data(dev,y_start >> 8);
    bsp_lcd_write_data(dev,y_start & 0xff);
    bsp_lcd_write_data(dev,y_end >> 8);
    bsp_lcd_write_data(dev,y_end & 0xff);

    bsp_lcd_write_cmd(dev,0x2c);
}

