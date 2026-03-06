//
// Created by 薛斌 on 24-8-22.
//

#include "bsp.h"
#include "lcd.h"

#include "bsp_dwt.h"
#include "fsmc.h"
#include "tim.h"

SRAM_HandleTypeDef g_sram_handle;   /* SRAM句柄(用于控制LCD) */

/* LCD的画笔颜色和背景色 */
uint32_t g_point_color = 0xF800;    /* 画笔颜色 */
uint32_t g_back_color  = 0xFFFF;    /* 背景色 */

/* 管理LCD重要参数 */
_lcd_dev lcddev;

/**
 * @brief       LCD写数据
 * @param       data: 要写入的数据
 * @retval      无
 */
void lcd_wr_data(volatile uint16_t data)
{
    data = data;            /* 使用-O2优化的时候,必须插入的延时 */
    LCD->LCD_RAM = data;
}

/**
 * @brief       LCD写寄存器编号/地址函数
 * @param       regno: 寄存器编号/地址
 * @retval      无
 */
void lcd_wr_regno(volatile uint16_t regno)
{
    regno = regno;          /* 使用-O2优化的时候,必须插入的延时 */
    LCD2->LCD_REG = regno;   /* 写入要写的寄存器序号 */
}

/**
 * @brief       LCD写寄存器
 * @param       regno:寄存器编号/地址
 * @param       data:要写入的数据
 * @retval      无
 */
void lcd_write_reg(uint16_t regno, uint16_t data)
{
    LCD2->LCD_REG = regno;   /* 写入要写的寄存器序号 */
    LCD->LCD_RAM = data;    /* 写入数据 */
}

void lcd_write_cmd_8bit(uint8_t cmd)
{
    cmd = cmd;
    LCD2->LCD_REG = cmd;    /* 写入数据 */
}

void lcd_write_data_8bit(uint8_t data)
{
    data = data;
    LCD->LCD_RAM = data;    /* 写入数据 */
}

/**
 * @brief       LCD延时函数,仅用于部分在mdk -O1时间优化时需要设置的地方
 * @param       t:延时的数值
 * @retval      无
 */
static void lcd_opt_delay(uint32_t i)
{
    while (i--); /* 使用AC6时空循环可能被优化,可使用while(1) __asm volatile(""); */
}

/**
 * @brief       LCD读数据
 * @param       无
 * @retval      读取到的数据
 */
uint16_t lcd_rd_data(void)
{
    volatile uint8_t ram;  /* 防止被优化 */
	lcd_opt_delay(2);
    ram = LCD->LCD_RAM;
    return ram;
}

/**
 * @brief       准备写GRAM
 * @param       无
 * @retval      无
 */
void lcd_write_ram_prepare(void)
{
    LCD->LCD_REG = lcddev.wramcmd;
}

/**
 * @brief       读取个某点的颜色值
 * @param       x,y:坐标
 * @retval      此点的颜色(32位颜色,方便兼容LTDC)
 */
uint32_t lcd_read_point(uint16_t x, uint16_t y)
{
    uint16_t r = 0, g = 0, b = 0;

    if (x >= lcddev.width || y >= lcddev.height)
    {
        return 0;   /* 超过了范围,直接返回 */
    }

    lcd_set_cursor(x, y);       /* 设置坐标 */

    if (lcddev.id == 0x5510)
    {
        lcd_wr_regno(0x2E00);   /* 5510 发送读GRAM指令 */
    }
    else
    {
        lcd_wr_regno(0x2E);     /* 9341/5310/1963/7789/7796/9806 等发送读GRAM指令 */
    }

    r = lcd_rd_data();          /* 假读(dummy read) */

    if (lcddev.id == 0x1963)
    {
        return r;   /* 1963直接读就可以 */
    }

    r = lcd_rd_data();          /* 实际坐标颜色 */

    if (lcddev.id == 0x7796)    /* 7796 一次读取一个像素值 */
    {
        return r;
    }

    /* 9341/5310/5510/7789/9806要分2次读出 */
    b = lcd_rd_data();
    g = r & 0xFF;               /* 对于9341/5310/5510/7789/9806,第一次读取的是RG的值,R在前,G在后,各占8位 */
    g <<= 8;

    return (((r >> 11) << 11) | ((g >> 10) << 5) | (b >> 11));  /* ILI9341/NT35310/NT35510/ST7789/ILI9806需要公式转换一下 */
}

/**
 * @brief       LCD开启显示
 * @param       无
 * @retval      无
 */
void lcd_display_on(void)
{
    if (lcddev.id == 0x5510)
    {
        lcd_wr_regno(0x2900);   /* 开启显示 */
    }
    else                        /* 9341/5310/1963/7789/7796/9806 等发送开启显示指令 */
    {
        lcd_wr_regno(0x29);     /* 开启显示 */
    }
}

/**
 * @brief       LCD关闭显示
 * @param       无
 * @retval      无
 */
void lcd_display_off(void)
{
    if (lcddev.id == 0x5510)
    {
        lcd_wr_regno(0x2800);   /* 关闭显示 */
    }
    else                        /* 9341/5310/1963/7789/7796/9806 等发送关闭显示指令 */
    {
        lcd_wr_regno(0x28);     /* 关闭显示 */
    }
}

/**
 * @brief       设置光标位置(对RGB屏无效)
 * @param       x,y: 坐标
 * @retval      无
 */
void lcd_set_cursor(uint16_t x, uint16_t y)
{
        lcd_write_cmd_8bit(0x2a);
        lcd_write_data_8bit(x >> 8);
        lcd_write_data_8bit(x & 0xFF);
    lcd_write_cmd_8bit(0x2b);
        lcd_write_data_8bit(y >> 8);
        lcd_write_data_8bit(y & 0xFF);
    lcd_write_cmd_8bit(0x2c);
}

void lcd_set_cursor_address(uint16_t x_start, uint16_t y_start,uint16_t x_end, uint16_t y_end)
{
    lcd_write_cmd_8bit(0x2a);
    lcd_write_data_8bit(x_start>>8);
    lcd_write_data_8bit(x_start & 0xFF);
    lcd_write_data_8bit(x_end>>8);
    lcd_write_data_8bit(x_end & 0xFF);
    bsp_delay_us(10);
    lcd_write_cmd_8bit(0x2b);

    lcd_write_data_8bit(y_start>>8);
    lcd_write_data_8bit(y_start & 0xFF);
    lcd_write_data_8bit(y_end>>8);
    lcd_write_data_8bit(y_end & 0xFF);
    bsp_delay_us(10);
    lcd_write_cmd_8bit(0x2c);
}

/**
 * @brief       设置LCD的自动扫描方向(对RGB屏无效)
 *   @note
 *              9341/5310/5510/1963/7789/7796/9806等IC已经实际测试
 *              注意:其他函数可能会受到此函数设置的影响(尤其是9341),
 *              所以,一般设置为L2R_U2D即可,如果设置为其他扫描方式,可能导致显示不正常.
 *
 * @param       dir:0~7,代表8个方向(具体定义见lcd.h)
 * @retval      无
 */
void lcd_scan_dir(uint8_t dir)
{
    uint16_t regval = 0;
    uint16_t dirreg = 0;
    uint16_t temp;

    /* 横屏时，对1963不改变扫描方向！竖屏时1963改变方向(这里仅用于1963的特殊处理,对其他驱动IC无效) */
    if ((lcddev.dir == 1 && lcddev.id != 0x1963) || (lcddev.dir == 0 && lcddev.id == 0x1963))
    {
        switch (dir)   /* 方向转换 */
        {
            case 0:
                dir = 6;
                break;

            case 1:
                dir = 7;
                break;

            case 2:
                dir = 4;
                break;

            case 3:
                dir = 5;
                break;

            case 4:
                dir = 1;
                break;

            case 5:
                dir = 0;
                break;

            case 6:
                dir = 3;
                break;

            case 7:
                dir = 2;
                break;
        }
    }


    /* 根据扫描方式 设置 0x36/0x3600 寄存器 bit 5,6,7 位的值 */
    switch (dir)
    {
        case L2R_U2D:   /* 从左到右,从上到下 */
            regval |= (0 << 7) | (0 << 6) | (0 << 5);
            break;

        case L2R_D2U:   /* 从左到右,从下到上 */
            regval |= (1 << 7) | (0 << 6) | (0 << 5);
            break;

        case R2L_U2D:   /* 从右到左,从上到下 */
            regval |= (0 << 7) | (1 << 6) | (0 << 5);
            break;

        case R2L_D2U:   /* 从右到左,从下到上 */
            regval |= (1 << 7) | (1 << 6) | (0 << 5);
            break;

        case U2D_L2R:   /* 从上到下,从左到右 */
            regval |= (0 << 7) | (0 << 6) | (1 << 5);
            break;

        case U2D_R2L:   /* 从上到下,从右到左 */
            regval |= (0 << 7) | (1 << 6) | (1 << 5);
            break;

        case D2U_L2R:   /* 从下到上,从左到右 */
            regval |= (1 << 7) | (0 << 6) | (1 << 5);
            break;

        case D2U_R2L:   /* 从下到上,从右到左 */
            regval |= (1 << 7) | (1 << 6) | (1 << 5);
            break;
    }

    dirreg = 0x36;  /* 对绝大部分驱动IC, 由0x36寄存器控制 */

    if (lcddev.id == 0x5510)
    {
        dirreg = 0x3600;    /* 对于5510, 和其他驱动ic的寄存器有差异 */
    }

     /* 9341 & 7789 & 7796 要设置BGR位 */
    if (lcddev.id == 0x9341 || lcddev.id == 0x7789 || lcddev.id == 0x7796)
    {
        regval |= 0x08;
    }

    lcd_write_reg(dirreg, regval);

    if (lcddev.id != 0x1963)                    /* 1963不做坐标处理 */
    {
        if (regval & 0x20)
        {
            if (lcddev.width < lcddev.height)   /* 交换X,Y */
            {
                temp = lcddev.width;
                lcddev.width = lcddev.height;
                lcddev.height = temp;
            }
        }
        else
        {
            if (lcddev.width > lcddev.height)   /* 交换X,Y */
            {
                temp = lcddev.width;
                lcddev.width = lcddev.height;
                lcddev.height = temp;
            }
        }
    }

    /* 设置显示区域(开窗)大小 */
    if (lcddev.id == 0x5510)
    {
        lcd_wr_regno(lcddev.setxcmd);
        lcd_wr_data(0);
        lcd_wr_regno(lcddev.setxcmd + 1);
        lcd_wr_data(0);
        lcd_wr_regno(lcddev.setxcmd + 2);
        lcd_wr_data((lcddev.width - 1) >> 8);
        lcd_wr_regno(lcddev.setxcmd + 3);
        lcd_wr_data((lcddev.width - 1) & 0xFF);
        lcd_wr_regno(lcddev.setycmd);
        lcd_wr_data(0);
        lcd_wr_regno(lcddev.setycmd + 1);
        lcd_wr_data(0);
        lcd_wr_regno(lcddev.setycmd + 2);
        lcd_wr_data((lcddev.height - 1) >> 8);
        lcd_wr_regno(lcddev.setycmd + 3);
        lcd_wr_data((lcddev.height - 1) & 0xFF);
    }
    else
    {
        lcd_wr_regno(lcddev.setxcmd);
        lcd_wr_data(0);
        lcd_wr_data(0);
        lcd_wr_data((lcddev.width - 1) >> 8);
        lcd_wr_data((lcddev.width - 1) & 0xFF);
        lcd_wr_regno(lcddev.setycmd);
        lcd_wr_data(0);
        lcd_wr_data(0);
        lcd_wr_data((lcddev.height - 1) >> 8);
        lcd_wr_data((lcddev.height - 1) & 0xFF);
    }
}

/**
 * @brief       画点
 * @param       x,y: 坐标
 * @param       color: 点的颜色(32位颜色,方便兼容LTDC)
 * @retval      无
 */
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
    lcd_set_cursor(x, y);       /* 设置光标位置 */
    lcd_write_ram_prepare();    /* 开始写入GRAM */
    LCD->LCD_RAM = color;
}

/**
 * @brief       SSD1963背光亮度设置函数
 * @param       pwm: 背光等级,0~100.越大越亮.
 * @retval      无
 */
void lcd_ssd_backlight_set(uint8_t pwm)
{
    lcd_wr_regno(0xBE);         /* 配置PWM输出 */
    lcd_wr_data(0x05);          /* 1设置PWM频率 */
    lcd_wr_data(pwm * 2.55);    /* 2设置PWM占空比 */
    lcd_wr_data(0x01);          /* 3设置C */
    lcd_wr_data(0xFF);          /* 4设置D */
    lcd_wr_data(0x00);          /* 5设置E */
    lcd_wr_data(0x00);          /* 6设置F */
}

/**
 * @brief       设置LCD显示方向
 * @param       dir:0,竖屏; 1,横屏
 * @retval      无
 */
void lcd_display_dir(uint8_t dir)
{
    lcddev.dir = dir;   /* 竖屏/横屏 */

    if (dir == 0)       /* 竖屏 */
    {
        lcddev.width = 240;
        lcddev.height = 320;


            lcddev.wramcmd = 0x2C;
            lcddev.setxcmd = 0x2A;
            lcddev.setycmd = 0x2B;

    }
    else        /* 横屏 */
    {
        lcddev.width = 320;         /* 默认宽度 */
        lcddev.height = 240;        /* 默认高度 */


            lcddev.wramcmd = 0x2C;
            lcddev.setxcmd = 0x2A;
            lcddev.setycmd = 0x2B;

    }

    //lcd_scan_dir(DFT_SCAN_DIR);     /* 默认扫描方向 */
}

/**
 * @brief       设置窗口(对RGB屏无效), 并自动设置画点坐标到窗口左上角(sx,sy).
 * @param       sx,sy:窗口起始坐标(左上角)
 * @param       width,height:窗口宽度和高度,必须大于0!!
 *   @note      窗体大小:width*height.
 *
 * @retval      无
 */
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    uint16_t twidth, theight;
    twidth = sx + width - 1;
    theight = sy + height - 1;

        lcd_write_cmd_8bit(0x2a);
        lcd_write_data_8bit(sx >> 8);
        lcd_write_data_8bit(sx & 0xFF);
        lcd_write_data_8bit(twidth >> 8);
        lcd_write_data_8bit(twidth & 0xFF);
        lcd_write_cmd_8bit(0x2b);
        lcd_write_data_8bit(sy >> 8);
        lcd_write_data_8bit(sy & 0xFF);
        lcd_write_data_8bit(theight >> 8);
        lcd_write_data_8bit(theight & 0xFF);
    lcd_write_cmd_8bit(0x2c);
}

/**
 * @brief       SRAM底层驱动，时钟使能，引脚分配
 * @note        此函数会被HAL_SRAM_Init()调用,初始化读写总线引脚
 * @param       hsram:SRAM句柄
 * @retval      无
 */
//void HAL_SRAM_MspInit(SRAM_HandleTypeDef *hsram)
//{
//    GPIO_InitTypeDef gpio_init_struct;
//
//    __HAL_RCC_FSMC_CLK_ENABLE();            /* 使能FSMC时钟 */
//    __HAL_RCC_GPIOD_CLK_ENABLE();           /* 使能GPIOD时钟 */
//    __HAL_RCC_GPIOE_CLK_ENABLE();           /* 使能GPIOE时钟 */
//
//    /* 初始化PD0,1, 8,9,10,14,15 */
//    gpio_init_struct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 \
//                           | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
//    gpio_init_struct.Mode = GPIO_MODE_AF_PP;            /* 推挽复用 */
//    gpio_init_struct.Pull = GPIO_PULLUP;                /* 上拉 */
//    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;      /* 高速 */
//    gpio_init_struct.Alternate = GPIO_AF12_FSMC;        /* 复用为FSMC */
//
//    HAL_GPIO_Init(GPIOD, &gpio_init_struct);            /* 初始化 */
//
//    /* 初始化PE7,8,9,10,11,12,13,14,15 */
//    gpio_init_struct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 \
//                           | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
//    HAL_GPIO_Init(GPIOE, &gpio_init_struct);
//}

/**
 * @brief       初始化LCD
 *   @note      该初始化函数可以初始化各种型号的LCD(详见本.c文件最前面的描述)
 *
 * @param       无
 * @retval      无
 */
void lcd_init(void)
{
    // GPIO_InitTypeDef gpio_init_struct;
    // FSMC_NORSRAM_TimingTypeDef fsmc_read_handle;
    // FSMC_NORSRAM_TimingTypeDef fsmc_write_handle;
    //
    // LCD_CS_GPIO_CLK_ENABLE();   /* LCD_CS脚时钟使能 */
    // LCD_WR_GPIO_CLK_ENABLE();   /* LCD_WR脚时钟使能 */
    // LCD_RD_GPIO_CLK_ENABLE();   /* LCD_RD脚时钟使能 */
    // LCD_RS_GPIO_CLK_ENABLE();   /* LCD_RS脚时钟使能 */
    // LCD_BL_GPIO_CLK_ENABLE();   /* LCD_BL脚时钟使能 */
    //
    // gpio_init_struct.Pin = LCD_CS_GPIO_PIN;
    // gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* 推挽复用 */
    // gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
    // gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* 高速 */
    // gpio_init_struct.Alternate = GPIO_AF12_FSMC;            /* 复用为FSMC */
    // HAL_GPIO_Init(LCD_CS_GPIO_PORT, &gpio_init_struct);     /* 初始化LCD_CS引脚 */
    //
    // gpio_init_struct.Pin = LCD_WR_GPIO_PIN;
    // HAL_GPIO_Init(LCD_WR_GPIO_PORT, &gpio_init_struct);     /* 初始化LCD_WR引脚 */
    //
    // gpio_init_struct.Pin = LCD_RD_GPIO_PIN;
    // HAL_GPIO_Init(LCD_RD_GPIO_PORT, &gpio_init_struct);     /* 初始化LCD_RD引脚 */
    //
    // gpio_init_struct.Pin = LCD_RS_GPIO_PIN;
    // HAL_GPIO_Init(LCD_RS_GPIO_PORT, &gpio_init_struct);     /* 初始化LCD_RS引脚 */
    //
    // gpio_init_struct.Pin = LCD_BL_GPIO_PIN;
    // gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;            /* 推挽输出 */
    // HAL_GPIO_Init(LCD_BL_GPIO_PORT, &gpio_init_struct);     /* LCD_BL引脚模式设置(推挽输出) */
    //
    // g_sram_handle.Instance = FSMC_NORSRAM_DEVICE;
    // g_sram_handle.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
    //
    // g_sram_handle.Init.NSBank = FSMC_NORSRAM_BANK1;                        /* 使用NE4 */
    // g_sram_handle.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;     /* 地址/数据线不复用 */
    // g_sram_handle.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_8;    /* 16位数据宽度 */
    // g_sram_handle.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;   /* 是否使能突发访问,仅对同步突发存储器有效,此处未用到 */
    // g_sram_handle.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW; /* 等待信号的极性,仅在突发模式访问下有用 */
    // g_sram_handle.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;      /* 存储器是在等待周期之前的一个时钟周期还是等待周期期间使能NWAIT */
    // g_sram_handle.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;       /* 存储器写使能 */
    // g_sram_handle.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;              /* 等待使能位,此处未用到 */
    // g_sram_handle.Init.ExtendedMode = FSMC_EXTENDED_MODE_ENABLE;           /* 读写使用不同的时序 */
    // g_sram_handle.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;  /* 是否使能同步传输模式下的等待信号,此处未用到 */
    // g_sram_handle.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;              /* 禁止突发写 */
    //
    // /* FSMC读时序控制寄存器 */
    // fsmc_read_handle.AddressSetupTime = 0x0F;           /* 地址建立时间(ADDSET)为15个fsmc_ker_ck(1/168=6)即6*15=90ns */
    // fsmc_read_handle.AddressHoldTime = 0x00;            /* 地址保持时间(ADDHLD) 模式A是没有用到 */
    // fsmc_read_handle.DataSetupTime = 60;                /* 数据保存时间(DATAST)为60个fsmc_ker_ck=6*60=360ns */
    //                                                     /* 因为液晶驱动IC的读数据的时候,速度不能太快,尤其是个别奇葩芯片 */
    // fsmc_read_handle.AccessMode = FSMC_ACCESS_MODE_A;   /* 模式A */
    // /* FSMC写时序控制寄存器 */
    // fsmc_write_handle.AddressSetupTime = 9;             /* 地址建立时间(ADDSET)为9个fsmc_ker_ck=6*9=54ns */
    // fsmc_write_handle.AddressHoldTime = 0x00;           /* 地址保持时间(ADDHLD) 模式A是没有用到 */
    // fsmc_write_handle.DataSetupTime = 9;                /* 数据保存时间(DATAST)为9个fsmc_ker_ck=6*9=54ns */
    //                                                     /* 注意：某些液晶驱动IC的写信号脉宽，最少也得50ns */
    // fsmc_write_handle.AccessMode = FSMC_ACCESS_MODE_A;  /* 模式A */
    //
    // HAL_SRAM_Init(&g_sram_handle, &fsmc_read_handle, &fsmc_write_handle);
    // osDelay(50);

    /* 尝试9341 ID的读取 */
    // uint8_t id[5];
    // lcd_wr_regno(0x04);
    // id[0] = lcd_rd_data();  /* dummy read */
    // id[1] = lcd_rd_data();  /* 读到0x00 */
    // id[2] = lcd_rd_data();  /* 读取93 */
    // id[3] = lcd_rd_data(); /* 读取41 */
    // id[4] = lcd_rd_data(); /* 读取41 */
    // printf("%x %x %x %x %x\r\n",id[0],id[1],id[2],id[3],id[4]);
    // lcd_write_cmd_8bit(0xfe);
    // lcd_write_cmd_8bit(0xef);
    // lcd_write_cmd_8bit(0x36);
    // lcd_write_data_8bit(0x48);
    // lcd_write_cmd_8bit(0x3a);
    // lcd_write_data_8bit(0x05);
    //
    // lcd_write_cmd_8bit(0x21);
		  //
    // lcd_write_cmd_8bit(0x86);
    // lcd_write_data_8bit(0x98);
	   //
    // lcd_write_cmd_8bit(0x89);
    // lcd_write_data_8bit(0x03);
    //
    // lcd_write_cmd_8bit(0xe8);
    // lcd_write_data_8bit(0x12);
    // lcd_write_data_8bit(0x00);
	   //
    // lcd_write_cmd_8bit(0x8b);
    // lcd_write_data_8bit(0x80);
	   //
    // lcd_write_cmd_8bit(0x8d);
    // lcd_write_data_8bit(0x22);
	   //
    // lcd_write_cmd_8bit(0xc9);
    // lcd_write_data_8bit(0x0a);
    // lcd_write_cmd_8bit(0xc3);
    // lcd_write_data_8bit(0x30);
    // lcd_write_cmd_8bit(0xc5);
    // lcd_write_data_8bit(0x15);
    // lcd_write_cmd_8bit(0xc6);
    // lcd_write_data_8bit(0x0a);
    // lcd_write_cmd_8bit(0xc7);
    // lcd_write_data_8bit(0x0a);
    // lcd_write_cmd_8bit(0xc8);
    // lcd_write_data_8bit(0x0e);
    // lcd_write_cmd_8bit(0xff);
    // lcd_write_data_8bit(0x62);
    //
    // lcd_write_cmd_8bit(0x99);
    // lcd_write_data_8bit(0x3e);
    // lcd_write_cmd_8bit(0x9d);
    // lcd_write_data_8bit(0x4b);
    // lcd_write_cmd_8bit(0x8e);
    // lcd_write_data_8bit(0x0f);
    //
    // lcd_write_cmd_8bit(0xF0);
    // lcd_write_data_8bit(0x82);
    // lcd_write_data_8bit(0x00);
    // lcd_write_data_8bit(0x0A);
    // lcd_write_data_8bit(0x09);
    // lcd_write_data_8bit(0x07);
    // lcd_write_data_8bit(0x2F);
    //
    // lcd_write_cmd_8bit(0xF1);
    // lcd_write_data_8bit(0x47);
    // lcd_write_data_8bit(0x98);
    // lcd_write_data_8bit(0xB7);
    // lcd_write_data_8bit(0x20);
    // lcd_write_data_8bit(0x25);
    // lcd_write_data_8bit(0xCF);
    //
    // lcd_write_cmd_8bit(0xF2);
    // lcd_write_data_8bit(0x45);
    // lcd_write_data_8bit(0x00);
    // lcd_write_data_8bit(0x0E);
    // lcd_write_data_8bit(0x0C);
    // lcd_write_data_8bit(0x08);
    // lcd_write_data_8bit(0x30);
    //
    // lcd_write_cmd_8bit(0xF3);
    // lcd_write_data_8bit(0x40);
    // lcd_write_data_8bit(0xB4);
    // lcd_write_data_8bit(0x92);
    // lcd_write_data_8bit(0x0F);
    // lcd_write_data_8bit(0x12);
    // lcd_write_data_8bit(0xBF);
    //
    // lcd_write_cmd_8bit(0x11);
    // osDelay(120);
    // lcd_write_cmd_8bit(0x29);
    // lcd_write_cmd_8bit(0x2c);

    //code 2


    lcd_write_cmd_8bit(0xfe);
    lcd_write_cmd_8bit(0xfe);
    lcd_write_cmd_8bit(0xef);
    lcd_write_cmd_8bit(0x36);
    //	lcd_wr_data(0x48);
    lcd_write_data_8bit(0x28);

    lcd_write_cmd_8bit(0x3a);
    lcd_write_data_8bit(0x05);

    lcd_write_cmd_8bit(0x86);
    lcd_write_data_8bit(0x98);
    lcd_write_cmd_8bit(0x89);
    lcd_write_data_8bit(0x03);
    lcd_write_cmd_8bit(0x8b);
    lcd_write_data_8bit(0x80);
    lcd_write_cmd_8bit(0x8d);
    lcd_write_data_8bit(0x33);
    lcd_write_cmd_8bit(0x8e);
    lcd_write_data_8bit(0x0f);


    lcd_write_cmd_8bit(0xe8);
    lcd_write_data_8bit(0x12);
    lcd_write_data_8bit(0x00);

    lcd_write_cmd_8bit(0xc3);
    lcd_write_data_8bit(0x1d);
    lcd_write_cmd_8bit(0xc4);
    lcd_write_data_8bit(0x1d);
    lcd_write_cmd_8bit(0xc9);
    lcd_write_data_8bit(0x0f);

    lcd_write_cmd_8bit(0xff);
    lcd_write_data_8bit(0x62);

    lcd_write_cmd_8bit(0x99);
    lcd_write_data_8bit(0x3e);
    lcd_write_cmd_8bit(0x9d);
    lcd_write_data_8bit(0x4b);
    lcd_write_cmd_8bit(0x98);
    lcd_write_data_8bit(0x3e);
    lcd_write_cmd_8bit(0x9c);
    lcd_write_data_8bit(0x4b);

    lcd_write_cmd_8bit(0xF0);
    lcd_write_data_8bit(0x49);
    lcd_write_data_8bit(0x0b);
    lcd_write_data_8bit(0x09);
    lcd_write_data_8bit(0x08);
    lcd_write_data_8bit(0x06);
    lcd_write_data_8bit(0x2e);

    lcd_write_cmd_8bit(0xF2);
    lcd_write_data_8bit(0x49);
    lcd_write_data_8bit(0x0b);
    lcd_write_data_8bit(0x09);
    lcd_write_data_8bit(0x08);
    lcd_write_data_8bit(0x06);
    lcd_write_data_8bit(0x2e);

    lcd_write_cmd_8bit(0xF1);
    lcd_write_data_8bit(0x45);
    lcd_write_data_8bit(0x92);
    lcd_write_data_8bit(0x93);
    lcd_write_data_8bit(0x2b);
    lcd_write_data_8bit(0x31);
    lcd_write_data_8bit(0x6F);

    lcd_write_cmd_8bit(0xF3);
    lcd_write_data_8bit(0x45);
    lcd_write_data_8bit(0x92);
    lcd_write_data_8bit(0x93);
    lcd_write_data_8bit(0x2b);
    lcd_write_data_8bit(0x31);
    lcd_write_data_8bit(0x6F);

    lcd_write_cmd_8bit(0x35);
    lcd_write_data_8bit(0x00);

    lcd_write_cmd_8bit(0x11);
    app_delay(120);
    lcd_write_cmd_8bit(0x29);
    lcd_write_cmd_8bit(0x2c);

    app_delay(100);
    // uint8_t id[5];
    // memset(&id,0,sizeof(id));
    // lcd_wr_regno(0x0a);
    // id[0] = lcd_rd_data();  /* dummy read */
    // id[1] = lcd_rd_data();  /* 读到0x00 */
    // id[2] = lcd_rd_data();  /* 读取93 */
    // id[3] = lcd_rd_data(); /* 读取41 */
    // printf("%x %x %x %x\r\n",id[0],id[1],id[2],id[3]);
    // app_delay(100);
    //
    // lcd_wr_regno(0x04);
    // id[0] = lcd_rd_data();  /* dummy read */
    // id[1] = lcd_rd_data();  /* 读到0x00 */
    // id[2] = lcd_rd_data();  /* 读取93 */
    // id[3] = lcd_rd_data(); /* 读取41 */
    // printf("%x %x %x %x\r\n",id[0],id[1],id[2],id[3]);
    // osDelay(100);

     lcd_set_cursor_address(0,0,320-1,240-1);
    //lcd_set_cursor_address(150,100,200,150);
     for(uint32_t i = 0; i < 240*320; i++)
     {
         LCD->LCD_RAM = 0xaa;
         LCD->LCD_RAM = 0xaa;
     }
    // osDelay(1000);
}

/**
 * @brief       清屏函数
 * @param       color: 要清屏的颜色
 * @retval      无
 */
void lcd_clear(uint16_t color)
{
    uint32_t index = 0;
    uint32_t totalpoint = lcddev.width;

    totalpoint *= lcddev.height;    /* 得到总点数 */
    lcd_set_cursor(0x00, 0x0000);   /* 设置光标位置 */
    lcd_write_ram_prepare();        /* 开始写入GRAM */

    for (index = 0; index < totalpoint; index++)
    {
        LCD->LCD_RAM = color;
    }
}

/**
 * @brief       在指定区域内填充单个颜色
 * @param       (sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex - sx + 1) * (ey - sy + 1)
 * @param       color:  要填充的颜色(32位颜色,方便兼容LTDC)
 * @retval      无
 */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    uint16_t i, j;
    uint16_t xlen = 0;
    xlen = ex - sx + 1;

    for (i = sy; i <= ey; i++)
    {
        lcd_set_cursor(sx, i);      /* 设置光标位置 */
        lcd_write_ram_prepare();    /* 开始写入GRAM */

        for (j = 0; j < xlen; j++)
        {
            LCD->LCD_RAM = color;   /* 显示颜色 */
        }
    }
}

/**
 * @brief       在指定区域内填充指定颜色块
 * @param       (sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex - sx + 1) * (ey - sy + 1)
 * @param       color: 要填充的颜色数组首地址
 * @retval      无
 */
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    uint16_t height, width;
    uint16_t i, j;

    width = ex - sx + 1;            /* 得到填充的宽度 */
    height = ey - sy + 1;           /* 高度 */

    for (i = 0; i < height; i++)
    {
        lcd_set_cursor(sx, sy + i); /* 设置光标位置 */
        lcd_write_ram_prepare();    /* 开始写入GRAM */

        for (j = 0; j < width; j++)
        {
            LCD->LCD_RAM = color[i * width + j]; /* 写入数据 */
        }
    }
}

/**
 * @brief       画线
 * @param       x1,y1: 起点坐标
 * @param       x2,y2: 终点坐标
 * @param       color: 线的颜色
 * @retval      无
 */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, row, col;
    delta_x = x2 - x1;  /* 计算坐标增量 */
    delta_y = y2 - y1;
    row = x1;
    col = y1;

    if (delta_x > 0)
    {
        incx = 1;   /* 设置单步方向 */
    }
    else if (delta_x == 0)
    {
        incx = 0;   /* 垂直线 */
    }
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }

    if (delta_y > 0)
    {
        incy = 1;
    }
    else if (delta_y == 0)
    {
        incy = 0;       /* 水平线 */
    }
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }

    if ( delta_x > delta_y)
    {
        distance = delta_x;  /* 选取基本增量坐标轴 */
    }
    else
    {
        distance = delta_y;
    }

    for (t = 0; t <= distance + 1; t++ )        /* 画线输出 */
    {
        lcd_draw_point(row, col, color);        /* 画点 */
        xerr += delta_x;
        yerr += delta_y;

        if (xerr > distance)
        {
            xerr -= distance;
            row += incx;
        }

        if (yerr > distance)
        {
            yerr -= distance;
            col += incy;
        }
    }
}

/**
 * @brief       画水平线
 * @param       x,y: 起点坐标
 * @param       len  : 线长度
 * @param       color: 矩形的颜色
 * @retval      无
 */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
    if ((len == 0) || (x > lcddev.width) || (y > lcddev.height))
    {
        return;
    }

    lcd_fill(x, y, x + len - 1, y, color);
}

/**
 * @brief       画矩形
 * @param       x1,y1: 起点坐标
 * @param       x2,y2: 终点坐标
 * @param       color: 矩形的颜色
 * @retval      无
 */
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    lcd_draw_line(x1, y1, x2, y1, color);
    lcd_draw_line(x1, y1, x1, y2, color);
    lcd_draw_line(x1, y2, x2, y2, color);
    lcd_draw_line(x2, y1, x2, y2, color);
}

/**
 * @brief       画圆
 * @param       x0,y0 : 圆中心坐标
 * @param       r     : 半径
 * @param       color : 圆的颜色
 * @retval      无
 */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a, b;
    int di;

    a = 0;
    b = r;
    di = 3 - (r << 1);       /* 判断下个点位置的标志 */

    while (a <= b)
    {
        lcd_draw_point(x0 + a, y0 - b, color);  /* 5 */
        lcd_draw_point(x0 + b, y0 - a, color);  /* 0 */
        lcd_draw_point(x0 + b, y0 + a, color);  /* 4 */
        lcd_draw_point(x0 + a, y0 + b, color);  /* 6 */
        lcd_draw_point(x0 - a, y0 + b, color);  /* 1 */
        lcd_draw_point(x0 - b, y0 + a, color);
        lcd_draw_point(x0 - a, y0 - b, color);  /* 2 */
        lcd_draw_point(x0 - b, y0 - a, color);  /* 7 */
        a++;

        /* 使用Bresenham算法画圆 */
        if (di < 0)
        {
            di += 4 * a + 6;
        }
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }
    }
}

/**
 * @brief       填充实心圆
 * @param       x,y  : 圆中心坐标
 * @param       r    : 半径
 * @param       color: 圆的颜色
 * @retval      无
 */
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{
    uint32_t i;
    uint32_t imax = ((uint32_t)r * 707) / 1000 + 1;
    uint32_t sqmax = (uint32_t)r * (uint32_t)r + (uint32_t)r / 2;
    uint32_t xr = r;

    lcd_draw_hline(x - r, y, 2 * r, color);

    for (i = 1; i <= imax; i++)
    {
        if ((i * i + xr * xr) > sqmax)
        {
            /* draw lines from outside */
            if (xr > imax)
            {
                lcd_draw_hline (x - i + 1, y + xr, 2 * (i - 1), color);
                lcd_draw_hline (x - i + 1, y - xr, 2 * (i - 1), color);
            }

            xr--;
        }

        /* draw lines from inside (center) */
        lcd_draw_hline(x - xr, y + i, 2 * xr, color);
        lcd_draw_hline(x - xr, y - i, 2 * xr, color);
    }
}

/**
 * @brief       平方函数, m^n
 * @param       m: 底数
 * @param       n: 指数
 * @retval      m的n次方
 */
static uint32_t lcd_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;

    while (n--)
    {
        result *= m;
    }

    return result;
}

/**
 * @brief       显示len个数字
 * @param       x,y : 起始坐标
 * @param       num : 数值(0 ~ 2^32)
 * @param       len : 显示数字的位数
 * @param       size: 选择字体 12/16/24/32
 * @param       color : 数字的颜色;
 * @retval      无
 */
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)   /* 按总显示位数循环 */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;   /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))               /* 没有使能显示,且还有位要显示 */
        {
            if (temp == 0)
            {
                lcd_show_char(x + (size / 2)*t, y, ' ', size, 0, color);    /* 显示空格,占位 */
                continue;       /* 继续下个一位 */
            }
            else
            {
                enshow = 1;     /* 使能显示 */
            }
        }

        lcd_show_char(x + (size / 2)*t, y, temp + '0', size, 0, color); /* 显示字符 */
    }
}

/**
 * @brief       扩展显示len个数字(高位是0也显示)
 * @param       x,y : 起始坐标
 * @param       num : 数值(0 ~ 2^32)
 * @param       len : 显示数字的位数
 * @param       size: 选择字体 12/16/24/32
 * @param       mode: 显示模式
 *              [7]:0,不填充;1,填充0.
 *              [6:1]:保留
 *              [0]:0,非叠加显示;1,叠加显示.
 * @param       color : 数字的颜色;
 * @retval      无
 */
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)       /* 按总显示位数循环 */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;    /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))   /* 没有使能显示,且还有位要显示 */
        {
            if (temp == 0)
            {
                if (mode & 0x80)    /* 高位需要填充0 */
                {
                    lcd_show_char(x + (size / 2)*t, y, '0', size, mode & 0x01, color);  /* 用0占位 */
                }
                else
                {
                    lcd_show_char(x + (size / 2)*t, y, ' ', size, mode & 0x01, color);  /* 用空格占位 */
                }

                continue;
            }
            else
            {
                enshow = 1;     /* 使能显示 */
            }
        }

        lcd_show_char(x + (size / 2)*t, y, temp + '0', size, mode & 0x01, color);
    }
}

/**
 * @brief       显示字符串
 * @param       x,y         : 起始坐标
 * @param       width,height: 区域大小
 * @param       size        : 选择字体 12/16/24/32
 * @param       p           : 字符串首地址
 * @param       color       : 字符串的颜色;
 * @retval      无
 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint8_t x0 = x;

    width += x;
    height += y;

    while ((*p <= '~') && (*p >= ' '))   /* 判断是不是非法字符! */
    {
        if (x >= width)
        {
            x = x0;
            y += size;
        }

        if (y >= height)
        {
            break;      /* 退出 */
        }

        lcd_show_char(x, y, *p, size, 0, color);
        x += size / 2;
        p++;
    }
}