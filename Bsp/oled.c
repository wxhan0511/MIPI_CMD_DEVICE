/**
 * @file       oled.c
 * @brief      MIPI_CMD OLED Driver
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-08-14
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

/* Includes ------------------------------------------------------------------*/
#include "oled.h"
#include <stdio.h>
#include <string.h>
#include "i2c.h"
#include "stm32f4xx_hal_i2c.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint8_t CMD_Data[]={
0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,0xA1, 0xC8, 0xDA,

0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6,0x8D, 0x14,

0xAF};

extern I2C_HandleTypeDef  hi2c1;
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/**
 * @function: void OLED_Init(void)
 * @description: OLED initialization
 * @return {*}
 */
void OLED_Init(void)
{
	HAL_Delay(200);

	uint8_t i = 0;
	for(i=0; i<23; i++)
	{
		OLED_WR_CMD(CMD_Data[i]);
	}
	
}

/**
 * @function: void OLED_WR_CMD(uint8_t cmd)
 * @description: Write a control command to the device
 * @param {uint8_t} cmd: command as defined in the chip datasheet
 * @return {*}
 */
void OLED_WR_CMD(uint8_t cmd)
{
	HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x00,I2C_MEMADD_SIZE_8BIT,&cmd,1,0x100);
}

/**
 * @function: void OLED_WR_DATA(uint8_t data)
 * @description: Write control data to the device
 * @param {uint8_t} data: data
 * @return {*}
 */
void OLED_WR_DATA(uint8_t data)
{
	HAL_I2C_Mem_Write(&hi2c1 ,0x78,0x40,I2C_MEMADD_SIZE_8BIT,&data,1,0x100);
}

/**
 * @function: void OLED_On(void)
 * @description: Update the display

 * @return {*}
 */
void OLED_On(void)
{
	uint8_t i,n;
	for(i=0;i<8;i++)
	{
		OLED_WR_CMD(0xb0+i);    //set page address (0~7)
		OLED_WR_CMD(0x00);      //set display position - column low address
		OLED_WR_CMD(0x10);      //set display position - column high address
		for(n=0;n<128;n++)
			OLED_WR_DATA(1);
	}
}


/**
 * @function: OLED_Clear(void)
 * @description: Clear the screen; the entire screen is black! Same as unlit!!!
 * @return {*}
 */
void OLED_Clear(void)
{
	uint8_t i,n;
	for(i=0;i<8;i++)
	{
		OLED_WR_CMD(0xb0+i);    //set page address (0~7)
		OLED_WR_CMD(0x00);      //set display position - column low address
		OLED_WR_CMD(0x10);      //set display position - column high address
		for(n=0;n<128;n++)
			OLED_WR_DATA(0);
	}
}

/**
 * @function: void OLED_Display_On(void)
 * @description: Turn on the OLED display
 * @return {*}
 */
void OLED_Display_On(void)
{
	OLED_WR_CMD(0X8D);  //SET DCDC command
	OLED_WR_CMD(0X14);  //DCDC ON
	OLED_WR_CMD(0XAF);  //DISPLAY ON, turn on the display
}


/**
 * @function: void OLED_Display_Off(void)
 * @description: Turn off the OLED display
 * @return {*}
 */
void OLED_Display_Off(void)
{
	OLED_WR_CMD(0X8D);  //SET DCDC command
	OLED_WR_CMD(0X10);  //DCDC OFF
	OLED_WR_CMD(0XAE);  //DISPLAY OFF, turn off the display
}

/**
 * @function: void OLED_Set_Pos(uint8_t x, uint8_t y)
 * @description: Set the coordinates
 * @param {uint8_t} x,y
 * @return {*}
 */
void OLED_Set_Pos(uint8_t x, uint8_t y)
{
	OLED_WR_CMD(0xb0+y);	//set page address (0~7)
	OLED_WR_CMD(((x&0xf0)>>4)|0x10); //set display position - column high address
	OLED_WR_CMD(x&0x0f);	//set display position - column low address
}


/**
 * @function: static unsigned int oled_pow(uint8_t m,uint8_t n)
 * @description: m^n function
 * @param {uint8_t} m,n
 * @return {unsigned int} result
 */
static unsigned int oled_pow(uint8_t m,uint8_t n)
{
	unsigned int result=1;
	while(n--)result*=m;
	return result;
}

/**
 * @function: void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size,uint8_t Color_Turn)
 * @description: Display a single character starting at a specific position on the OLED12864
 * @param {uint8_t} x: horizontal coordinate where the character display starts
 * @param {uint8_t} y: vertical coordinate where the character display starts
 * @param {uint8_t} chr: character to display
 * @param {uint8_t} Char_Size: font size of the character, font 16/12
 * @param {uint8_t} Color_Turn: inverted display or not (1 inverted, 0 normal)
 * @return {*}
 */
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t Char_Size,uint8_t Color_Turn)
{
	unsigned char c=0,i=0;
		c=chr-' ';//get the offset value
		if(x>128-1){x=0;y=y+2;}
		if(Char_Size ==16)
		{
			OLED_Set_Pos(x,y);
			for(i=0;i<8;i++)
				{
				  if(Color_Turn)
					  OLED_WR_DATA(~F8X16[c*16+i]);
				  else
					  OLED_WR_DATA(F8X16[c*16+i]);
				}
			OLED_Set_Pos(x,y+1);
			for(i=0;i<8;i++)
			    {
				  if(Color_Turn)
					  OLED_WR_DATA(~F8X16[c*16+i+8]);
				  else
					  OLED_WR_DATA(F8X16[c*16+i+8]);
			    }

			}
	     else
	     {
				OLED_Set_Pos(x,y);
				for(i=0;i<6;i++)
			    {
				  if(Color_Turn)
					  OLED_WR_DATA(~F6x8[c][i]);
				  else
					  OLED_WR_DATA(F6x8[c][i]);
			    }
		  }
}

/**
 * @function: void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_tChar_Size, uint8_t Color_Turn)
 * @description: Display a string starting at a specific position on the OLED12864
 * @param {uint8_t} x: start horizontal coordinate of the string, x:0~127
 * @param {uint8_t} y: start vertical coordinate of the string, y:0~7; with font size 16 leave a gap of 2 between rows, with font size 12 a gap of 1
 * @param {uint8_t} *chr: string to display
 * @param {uint8_t} Char_Size: font size of the string, 16/12; 16 = 8X16, 12 = 6x8
 * @param {uint8_t} Color_Turn: inverted display or not (1 inverted, 0 normal)
 * @return {*}
 */
void OLED_ShowString(uint8_t x,uint8_t y,char*chr,uint8_t Char_Size, uint8_t Color_Turn)
{
	uint8_t  j=0;
	while (chr[j]!='\0')
	{		OLED_ShowChar(x,y,chr[j],Char_Size, Color_Turn);
			if (Char_Size == 12) //6X8 font: add 6 to the column for the next character
				x += 6;
			else  //8X16 font: add 8 to the column for the next character
				x += 8;

			if (x > 122 && Char_Size==12) //TextSize6x8: if the line is full, continue on the next line
			{
				x = 0;
				y++;
			}
			if (x > 120 && Char_Size== 16) //TextSize8x16: if the line is full, continue on the next line
			{
				x = 0;
				y++;
			}
			j++;
	}
}

/**
 * @function: void OLED_ShowNum(uint8_t x,uint8_t y,unsigned int num,uint8_t len,uint8_t size2, Color_Turn)
 * @description: Display a number
 * @param {uint8_t} x: start horizontal coordinate of the number, x:0~126
 * @param {uint8_t} y: start vertical coordinate of the number, y:0~7; with font size 16 leave a gap of 2 between rows, with font size 12 a gap of 1
 * @param {unsigned int} num: input data
 * @param {uint8_t } len: number of digits to display
 * @param {uint8_t} size2: data size, 16/12; 16 = 8X16, 12 = 6x8
 * @param {uint8_t} Color_Turn: inverted display or not (1 inverted, 0 normal)
 * @return {*}
 */
void OLED_ShowNum(uint8_t x,uint8_t y,unsigned int num,uint8_t len,uint8_t size2, uint8_t Color_Turn)
{
	uint8_t t,temp;
	uint8_t enshow=0;
	for(t=0;t<len;t++)
	{
		temp=(num/oled_pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(size2/2)*t,y,' ',size2, Color_Turn);
				continue;
			}else enshow=1;

		}
	 	OLED_ShowChar(x+(size2/2)*t,y,temp+'0',size2, Color_Turn);
	}
}


/**
 * @function: void OLED_Showdecimal(uint8_t x,uint8_t y,float num,uint8_t z_len,uint8_t f_len,uint8_t size2, uint8_t Color_Turn)
 * @description: Display a signed floating-point number
 * @param {uint8_t} x: start horizontal coordinate of the number, x:0~126
 * @param {uint8_t} y: start vertical coordinate of the number, y:0~7; with font size 16 leave a gap of 2 between rows, with font size 12 a gap of 1
 * @param {float} num: floating-point input
 * @param {uint8_t } z_ len: number of integer-part digits
 * @param {uint8_t } f_len: number of fractional-part digits
 * @param {uint8_t} size2: data size, 16/12; 16 = 8X16, 12 = 6x8
 * @param {uint8_t} Color_Turn: inverted display or not (1 inverted, 0 normal)
 * @return {*}
 */
void OLED_Showdecimal(uint8_t x,uint8_t y,float num,uint8_t z_len,uint8_t f_len,uint8_t size2, uint8_t Color_Turn)
{
	uint8_t t,temp,i=0;//i is the negative-number flag
	uint8_t enshow;
	int z_temp,f_temp;
	if(num<0)
	{
		z_len+=1;
		i=1;
		num=-num;
	}
	z_temp=(int)num;
	//integer part
	for(t=0;t<z_len;t++)
	{
		temp=(z_temp/oled_pow(10,z_len-t-1))%10;
		if(enshow==0 && t<(z_len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(size2/2)*t,y,' ',size2, Color_Turn);
				continue;
			}
			else
			enshow=1;
		}
		OLED_ShowChar(x+(size2/2)*t,y,temp+'0',size2, Color_Turn);
	}
	//decimal point
	OLED_ShowChar(x+(size2/2)*(z_len),y,'.',size2, Color_Turn);

	f_temp=(int)((num-z_temp)*(oled_pow(10,f_len)));
  //fractional part
	for(t=0;t<f_len;t++)
	{
		temp=(f_temp/oled_pow(10,f_len-t-1))%10;
		OLED_ShowChar(x+(size2/2)*(t+z_len)+5,y,temp+'0',size2, Color_Turn);
	}
	if(i==1)//if negative, write '-' at the first position
	{
		OLED_ShowChar(x,y,'-',size2, Color_Turn);
		i=0;
	}
}



/**
 * @function: void OLED_ShowCHinese(uint8_t x,uint8_t y,uint8_t no, uint8_t Color_Turn)
 * @description: Display a 16X16 Chinese character starting at a specific position on the OLED
 * @param {uint8_t} x: start horizontal coordinate of the Chinese character, x: 0~112; leave a gap of 16 between columns
 * @param {uint8_t} y: start vertical coordinate of the Chinese character, y: 0~6; leave a gap of 2 between rows
 * @param {uint8_t} no: index of the Chinese character to display
 * @param {uint8_t} Color_Turn: inverted display or not (1 inverted, 0 normal)
 * @return {*}
 */
void OLED_ShowCHinese(uint8_t x,uint8_t y,uint8_t no, uint8_t Color_Turn)
{
	uint8_t t=0;
	OLED_Set_Pos(x,y);
    for(t=0;t<16;t++)
		{
				if (Color_Turn)
					OLED_WR_DATA(~Hzk[2*no][t]); //display the upper half of the Chinese character
				else
					OLED_WR_DATA(Hzk[2*no][t]); //display the upper half of the Chinese character
        }

		OLED_Set_Pos(x,y+1);
    for(t=0;t<16;t++)
		{
				if (Color_Turn)
					OLED_WR_DATA(~Hzk[2*no+1][t]); //display the lower half of the Chinese character
				else
					OLED_WR_DATA(Hzk[2*no+1][t]);//display the lower half of the Chinese character

         }
}

/**
 * @function: void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t *  BMP,uint8_t Color_Turn)
 * @description: Display a BMP image in a specific area of the OLED
 * @param {uint8_t} x0: start horizontal coordinate of the image, x0:0~127
 * @param {uint8_t} y0: start vertical coordinate of the image, y0:0~7
 * @param {uint8_t} x1: end horizontal coordinate of the image, x1:1~128
 * @param {uint8_t} y1: end vertical coordinate of the image, y1:1~8
 * @param {uint8_t} *BMP: image data to display
 * @param {uint8_t} Color_Turn: inverted display or not (1 inverted, 0 normal)
 * @return {*}
 */
void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t *  BMP,uint8_t Color_Turn)
{
   uint32_t j = 0;
   uint8_t x = 0, y = 0;

  if(y1%8==0)
		y = y1/8;
  else
		y = y1/8 + 1;
	for(y=y0;y<y1;y++)
	{
		OLED_Set_Pos(x0,y);
    for(x=x0;x<x1;x++)
		{
			if (Color_Turn)
				OLED_WR_DATA(~BMP[j++]);//display the inverted image
			else
				OLED_WR_DATA(BMP[j++]);//display the image

		}
	}
}


/**
 * @function: void OLED_HorizontalShift(uint8_t direction)
 * @description: Scroll the entire screen content horizontally
 * @param {uint8_t} direction			LEFT	   0x27     	RIGHT  0x26
 * @return {*}
 */
void OLED_HorizontalShift(uint8_t direction)

{
	OLED_WR_CMD(0x2e);//stop scrolling
	OLED_WR_CMD(direction);//set scroll direction
	OLED_WR_CMD(0x00);//dummy byte, default 0x00
	OLED_WR_CMD(0x00);//set start page address
	OLED_WR_CMD(0x07);//set the frame interval between scroll steps
    //  0x00-5 frames, 0x01-64 frames, 0x02-128 frames, 0x03-256 frames, 0x04-3 frames, 0x05-4 frames, 0x06-25 frames, 0x07-2 frames,
	OLED_WR_CMD(0x07);//set end page address
	OLED_WR_CMD(0x00);//dummy byte, default 0x00
	OLED_WR_CMD(0xff);//dummy byte, default 0xff
	OLED_WR_CMD(0x2f);//0x2f start scrolling, 0x2e stop scrolling; data must be rewritten after disabling
}

/**
 * @function: void OLED_Some_HorizontalShift(uint8_t direction,uint8_t start,uint8_t end)
 * @description: Scroll part of the screen content horizontally
 * @param {uint8_t} direction			LEFT	   0x27     	RIGHT  0x26
 * @param {uint8_t} start: start page address 0x00~0x07
 * @param {uint8_t} end: end page address 0x01~0x07
 * @return {*}
 */
void OLED_Some_HorizontalShift(uint8_t direction,uint8_t start,uint8_t end)
{
	OLED_WR_CMD(0x2e);//stop scrolling
	OLED_WR_CMD(direction);//set scroll direction
	OLED_WR_CMD(0x00);//dummy byte, default 0x00
	OLED_WR_CMD(start);//set start page address
	OLED_WR_CMD(0x07);//set the frame interval between scroll steps; 0x07 = 2-frame scroll speed
	OLED_WR_CMD(end);//set end page address
	OLED_WR_CMD(0x00);//dummy byte, default 0x00
	OLED_WR_CMD(0xff);//dummy byte, default 0xff
	OLED_WR_CMD(0x2f);//0x2f start scrolling, 0x2e stop scrolling; data must be rewritten after disabling

}

/**
 * @function: void OLED_VerticalAndHorizontalShift(uint8_t direction)
 * @description: Scroll the entire screen content vertically and horizontally
 * @param {uint8_t} direction: scroll up-right	 0x29
 *                                                            scroll up-left   0x2A
 * @return {*}
 */
void OLED_VerticalAndHorizontalShift(uint8_t direction)
{
	OLED_WR_CMD(0x2e);//stop scrolling
	OLED_WR_CMD(direction);//set scroll direction
	OLED_WR_CMD(0x01);//dummy byte
	OLED_WR_CMD(0x00);//set start page address
	OLED_WR_CMD(0x07);//set the frame interval between scroll steps, i.e. the scroll speed
	OLED_WR_CMD(0x07);//set end page address
	OLED_WR_CMD(0x01);//vertical scroll offset
	OLED_WR_CMD(0x00);//dummy byte, default 0x00
	OLED_WR_CMD(0xff);//dummy byte, default 0xff
	OLED_WR_CMD(0x2f);//0x2f start scrolling, 0x2e stop scrolling; data must be rewritten after disabling
}

/**
 * @function: void OLED_DisplayMode(uint8_t mode)
 * @description: Display the screen content inverted
 * @param {uint8_t} direction			ON	0xA7
 *                                                          OFF	0xA6	default mode, pixels lit
 * @return {*}
 */
void OLED_DisplayMode(uint8_t mode)
{
	OLED_WR_CMD(mode);
}

/**
 * @function: void OLED_IntensityControl(uint8_t intensity)
 * @description: Adjust the screen brightness
 * @param  {uint8_t} intensity	0x00~0xFF,RESET=0x7F
 * @return {*}
 */
void OLED_IntensityControl(uint8_t intensity)
{
	OLED_WR_CMD(0x81);
	OLED_WR_CMD(intensity);
}
/* Exported functions --------------------------------------------------------*/
