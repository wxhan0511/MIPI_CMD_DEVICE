//
// Created by xuebin on 24-8-22.
//

#include "bsp.h"
#include "lcd.h"

#include "bsp_dwt.h"
#include "fsmc.h"
#include "tim.h"

/* LCD pen color and background color */
uint32_t g_point_color = 0xF800;    /* Pen color */
uint32_t g_back_color  = 0xFFFF;    /* Background color */

/* Key LCD parameters */
_lcd_dev lcddev;

/**
 * @brief       Write data to the LCD
 * @param       data: data to write
 * @retval      None
 */
void lcd_wr_data(volatile uint16_t data)
{
    data = data;            /* Dummy delay required when compiled with -O2 */
    LCD->LCD_RAM = data;
}

/**
 * @brief       Write LCD register number/address
 * @param       regno: register number/address
 * @retval      None
 */
void lcd_wr_regno(volatile uint16_t regno)
{
    regno = regno;          /* Dummy delay required when compiled with -O2 */
    LCD2->LCD_REG = regno;   /* Write the register index to be written */
}

/**
 * @brief       Write an LCD register
 * @param       regno: register number/address
 * @param       data: data to write
 * @retval      None
 */
void lcd_write_reg(uint16_t regno, uint16_t data)
{
    LCD2->LCD_REG = regno;   /* Write the register index to be written */
    LCD->LCD_RAM = data;    /* Write the data */
}

void lcd_write_cmd_8bit(uint8_t cmd)
{
    cmd = cmd;
    LCD2->LCD_REG = cmd;    /* Write the command */
}

void lcd_write_data_8bit(uint8_t data)
{
    data = data;
    LCD->LCD_RAM = data;    /* Write the data */
}

/**
 * @brief       LCD delay function, only needed in a few places under MDK -O1 timing optimization
 * @param       t: delay count
 * @retval      None
 */
static void lcd_opt_delay(uint32_t i)
{
    while (i--); /* The empty loop may be optimized away with AC6; use while(1) __asm volatile("") instead */
}

/**
 * @brief       Read data from the LCD
 * @param       None
 * @retval      The data read
 */
uint16_t lcd_rd_data(void)
{
    volatile uint8_t ram;  /* Prevent the compiler from optimizing it away */
	lcd_opt_delay(2);
    ram = LCD->LCD_RAM;
    return ram;
}

/**
 * @brief       Prepare to write GRAM
 * @param       None
 * @retval      None
 */
void lcd_write_ram_prepare(void)
{
    LCD->LCD_REG = lcddev.wramcmd;
}

/**
 * @brief       Read the color of a given point
 * @param       x,y: coordinates
 * @retval      The color of this point (32-bit color, for LTDC compatibility)
 */
uint32_t lcd_read_point(uint16_t x, uint16_t y)
{
    uint16_t r = 0, g = 0, b = 0;

    if (x >= lcddev.width || y >= lcddev.height)
    {
        return 0;   /* Out of range, return directly */
    }

    lcd_set_cursor(x, y);       /* Set coordinates */

    if (lcddev.id == 0x5510)
    {
        lcd_wr_regno(0x2E00);   /* 5510: send the read-GRAM command */
    }
    else
    {
        lcd_wr_regno(0x2E);     /* 9341/5310/1963/7789/7796/9806 etc.: send the read-GRAM command */
    }

    r = lcd_rd_data();          /* Dummy read */

    if (lcddev.id == 0x1963)
    {
        return r;   /* 1963 can be read directly */
    }

    r = lcd_rd_data();          /* Actual color at the coordinates */

    if (lcddev.id == 0x7796)    /* 7796 returns one pixel value per read */
    {
        return r;
    }

    /* 9341/5310/5510/7789/9806 require two separate reads */
    b = lcd_rd_data();
    g = r & 0xFF;               /* For 9341/5310/5510/7789/9806 the first read returns RG, R first then G, 8 bits each */
    g <<= 8;

    return (((r >> 11) << 11) | ((g >> 10) << 5) | (b >> 11));  /* ILI9341/NT35310/NT35510/ST7789/ILI9806 require this formula conversion */
}

/**
 * @brief       Turn on the LCD display
 * @param       None
 * @retval      None
 */
void lcd_display_on(void)
{
    if (lcddev.id == 0x5510)
    {
        lcd_wr_regno(0x2900);   /* Turn on display */
    }
    else                        /* 9341/5310/1963/7789/7796/9806 etc.: send the display-on command */
    {
        lcd_wr_regno(0x29);     /* Turn on display */
    }
}

/**
 * @brief       Turn off the LCD display
 * @param       None
 * @retval      None
 */
void lcd_display_off(void)
{
    if (lcddev.id == 0x5510)
    {
        lcd_wr_regno(0x2800);   /* Turn off display */
    }
    else                        /* 9341/5310/1963/7789/7796/9806 etc.: send the display-off command */
    {
        lcd_wr_regno(0x28);     /* Turn off display */
    }
}

/**
 * @brief       Set the cursor position (not applicable to RGB panels)
 * @param       x,y: coordinates
 * @retval      None
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
 * @brief       Set the LCD auto scan direction (not applicable to RGB panels)
 *   @note
 *              Tested on 9341/5310/5510/1963/7789/7796/9806 ICs.
 *              Note: other functions may be affected by this setting (especially 9341),
 *              so L2R_U2D is recommended; other scan directions may cause an abnormal display.
 *
 * @param       dir: 0~7, representing 8 directions (see lcd.h for definitions)
 * @retval      None
 */
void lcd_scan_dir(uint8_t dir)
{
    uint16_t regval = 0;
    uint16_t dirreg = 0;
    uint16_t temp;

    /* In landscape, do not change the scan direction for 1963! In portrait, 1963 changes direction (1963-specific handling only, no effect on other driver ICs) */
    if ((lcddev.dir == 1 && lcddev.id != 0x1963) || (lcddev.dir == 0 && lcddev.id == 0x1963))
    {
        switch (dir)   /* Direction conversion */
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


    /* Set bits 5, 6, 7 of register 0x36/0x3600 according to the scan direction */
    switch (dir)
    {
        case L2R_U2D:   /* Left to right, top to bottom */
            regval |= (0 << 7) | (0 << 6) | (0 << 5);
            break;

        case L2R_D2U:   /* Left to right, bottom to top */
            regval |= (1 << 7) | (0 << 6) | (0 << 5);
            break;

        case R2L_U2D:   /* Right to left, top to bottom */
            regval |= (0 << 7) | (1 << 6) | (0 << 5);
            break;

        case R2L_D2U:   /* Right to left, bottom to top */
            regval |= (1 << 7) | (1 << 6) | (0 << 5);
            break;

        case U2D_L2R:   /* Top to bottom, left to right */
            regval |= (0 << 7) | (0 << 6) | (1 << 5);
            break;

        case U2D_R2L:   /* Top to bottom, right to left */
            regval |= (0 << 7) | (1 << 6) | (1 << 5);
            break;

        case D2U_L2R:   /* Bottom to top, left to right */
            regval |= (1 << 7) | (0 << 6) | (1 << 5);
            break;

        case D2U_R2L:   /* Bottom to top, right to left */
            regval |= (1 << 7) | (1 << 6) | (1 << 5);
            break;
    }

    dirreg = 0x36;  /* For most driver ICs, controlled by register 0x36 */

    if (lcddev.id == 0x5510)
    {
        dirreg = 0x3600;    /* For 5510, the register differs from other driver ICs */
    }

     /* 9341, 7789 and 7796 need the BGR bit set */
    if (lcddev.id == 0x9341 || lcddev.id == 0x7789 || lcddev.id == 0x7796)
    {
        regval |= 0x08;
    }

    lcd_write_reg(dirreg, regval);

    if (lcddev.id != 0x1963)                    /* 1963 does no coordinate processing */
    {
        if (regval & 0x20)
        {
            if (lcddev.width < lcddev.height)   /* Swap X and Y */
            {
                temp = lcddev.width;
                lcddev.width = lcddev.height;
                lcddev.height = temp;
            }
        }
        else
        {
            if (lcddev.width > lcddev.height)   /* Swap X and Y */
            {
                temp = lcddev.width;
                lcddev.width = lcddev.height;
                lcddev.height = temp;
            }
        }
    }

    /* Set the display area (window) size */
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
 * @brief       Draw a point
 * @param       x,y: coordinates
 * @param       color: point color (32-bit color, for LTDC compatibility)
 * @retval      None
 */
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
    lcd_set_cursor(x, y);       /* Set cursor position */
    lcd_write_ram_prepare();    /* Start writing GRAM */
    LCD->LCD_RAM = color;
}

/**
 * @brief       Set the SSD1963 backlight brightness
 * @param       pwm: backlight level, 0~100; larger is brighter
 * @retval      None
 */
void lcd_ssd_backlight_set(uint8_t pwm)
{
    lcd_wr_regno(0xBE);         /* Configure PWM output */
    lcd_wr_data(0x05);          /* 1: set PWM frequency */
    lcd_wr_data(pwm * 2.55);    /* 2: set PWM duty cycle */
    lcd_wr_data(0x01);          /* 3: set C */
    lcd_wr_data(0xFF);          /* 4: set D */
    lcd_wr_data(0x00);          /* 5: set E */
    lcd_wr_data(0x00);          /* 6: set F */
}

/**
 * @brief       Set the LCD display direction
 * @param       dir: 0, portrait; 1, landscape
 * @retval      None
 */
void lcd_display_dir(uint8_t dir)
{
    lcddev.dir = dir;   /* Portrait/landscape */

    if (dir == 0)       /* Portrait */
    {
        lcddev.width = 240;
        lcddev.height = 320;


            lcddev.wramcmd = 0x2C;
            lcddev.setxcmd = 0x2A;
            lcddev.setycmd = 0x2B;

    }
    else        /* Landscape */
    {
        lcddev.width = 320;         /* Default width */
        lcddev.height = 240;        /* Default height */


            lcddev.wramcmd = 0x2C;
            lcddev.setxcmd = 0x2A;
            lcddev.setycmd = 0x2B;

    }

    //lcd_scan_dir(DFT_SCAN_DIR);     /* Default scan direction */
}

/**
 * @brief       Set the window (not applicable to RGB panels) and automatically set the
 *              drawing cursor to the window's top-left corner (sx,sy).
 * @param       sx,sy: window start coordinates (top-left corner)
 * @param       width,height: window width and height, must be greater than 0!!
 *   @note      Window size: width*height.
 *
 * @retval      None
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
 * @brief       SRAM low-level driver: clock enable, pin assignment
 * @note        Called by HAL_SRAM_Init() to initialize the read/write bus pins
 * @param       hsram: SRAM handle
 * @retval      None
 */
//void HAL_SRAM_MspInit(SRAM_HandleTypeDef *hsram)
//{
//    GPIO_InitTypeDef gpio_init_struct;
//
//    __HAL_RCC_FSMC_CLK_ENABLE();            /* Enable the FSMC clock */
//    __HAL_RCC_GPIOD_CLK_ENABLE();           /* Enable the GPIOD clock */
//    __HAL_RCC_GPIOE_CLK_ENABLE();           /* Enable the GPIOE clock */
//
//    /* Initialize PD0, 1, 8, 9, 10, 14, 15 */
//    gpio_init_struct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 \
//                           | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
//    gpio_init_struct.Mode = GPIO_MODE_AF_PP;            /* Alternate function push-pull */
//    gpio_init_struct.Pull = GPIO_PULLUP;                /* Pull-up */
//    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;      /* High speed */
//    gpio_init_struct.Alternate = GPIO_AF12_FSMC;        /* Alternate function: FSMC */
//
//    HAL_GPIO_Init(GPIOD, &gpio_init_struct);            /* Initialize */
//
//    /* Initialize PE7, 8, 9, 10, 11, 12, 13, 14, 15 */
//    gpio_init_struct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 \
//                           | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
//    HAL_GPIO_Init(GPIOE, &gpio_init_struct);
//}

/**
 * @brief       Initialize the LCD
 *   @note      This function can initialize LCDs of various models (see the description
 *              at the top of this .c file)
 *
 * @param       None
 * @retval      None
 */
void lcd_init(void)
{
    // GPIO_InitTypeDef gpio_init_struct;
    // FSMC_NORSRAM_TimingTypeDef fsmc_read_handle;
    // FSMC_NORSRAM_TimingTypeDef fsmc_write_handle;
    //
    // LCD_CS_GPIO_CLK_ENABLE();   /* Enable the LCD_CS pin clock */
    // LCD_WR_GPIO_CLK_ENABLE();   /* Enable the LCD_WR pin clock */
    // LCD_RD_GPIO_CLK_ENABLE();   /* Enable the LCD_RD pin clock */
    // LCD_RS_GPIO_CLK_ENABLE();   /* Enable the LCD_RS pin clock */
    // LCD_BL_GPIO_CLK_ENABLE();   /* Enable the LCD_BL pin clock */
    //
    // gpio_init_struct.Pin = LCD_CS_GPIO_PIN;
    // gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* Alternate function push-pull */
    // gpio_init_struct.Pull = GPIO_PULLUP;                    /* Pull-up */
    // gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* High speed */
    // gpio_init_struct.Alternate = GPIO_AF12_FSMC;            /* Alternate function: FSMC */
    // HAL_GPIO_Init(LCD_CS_GPIO_PORT, &gpio_init_struct);     /* Initialize the LCD_CS pin */
    //
    // gpio_init_struct.Pin = LCD_WR_GPIO_PIN;
    // HAL_GPIO_Init(LCD_WR_GPIO_PORT, &gpio_init_struct);     /* Initialize the LCD_WR pin */
    //
    // gpio_init_struct.Pin = LCD_RD_GPIO_PIN;
    // HAL_GPIO_Init(LCD_RD_GPIO_PORT, &gpio_init_struct);     /* Initialize the LCD_RD pin */
    //
    // gpio_init_struct.Pin = LCD_RS_GPIO_PIN;
    // HAL_GPIO_Init(LCD_RS_GPIO_PORT, &gpio_init_struct);     /* Initialize the LCD_RS pin */
    //
    // gpio_init_struct.Pin = LCD_BL_GPIO_PIN;
    // gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;            /* Push-pull output */
    // HAL_GPIO_Init(LCD_BL_GPIO_PORT, &gpio_init_struct);     /* Set LCD_BL pin mode (push-pull output) */
    //
    // g_sram_handle.Instance = FSMC_NORSRAM_DEVICE;
    // g_sram_handle.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
    //
    // g_sram_handle.Init.NSBank = FSMC_NORSRAM_BANK1;                        /* Use NE4 */
    // g_sram_handle.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;     /* Address/data lines not multiplexed */
    // g_sram_handle.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_8;    /* 16-bit data width */
    // g_sram_handle.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;   /* Burst access enable, only valid for synchronous burst memories, unused here */
    // g_sram_handle.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW; /* Wait signal polarity, only useful in burst mode access */
    // g_sram_handle.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;      /* Whether NWAIT is asserted one clock cycle before the wait period or during it */
    // g_sram_handle.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;       /* Memory write enable */
    // g_sram_handle.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;              /* Wait enable bit, unused here */
    // g_sram_handle.Init.ExtendedMode = FSMC_EXTENDED_MODE_ENABLE;           /* Different timings for read and write */
    // g_sram_handle.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;  /* Wait signal enable in synchronous transfer mode, unused here */
    // g_sram_handle.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;              /* Disable write burst */
    //
    // /* FSMC read timing control register */
    // fsmc_read_handle.AddressSetupTime = 0x0F;           /* Address setup time (ADDSET) = 15 fsmc_ker_ck cycles (1/168=6), i.e. 6*15=90ns */
    // fsmc_read_handle.AddressHoldTime = 0x00;            /* Address hold time (ADDHLD), not used in mode A */
    // fsmc_read_handle.DataSetupTime = 60;                /* Data setup time (DATAST) = 60 fsmc_ker_ck cycles = 6*60=360ns */
    //                                                     /* The LCD driver IC must not be read too fast, especially some finicky chips */
    // fsmc_read_handle.AccessMode = FSMC_ACCESS_MODE_A;   /* Mode A */
    // /* FSMC write timing control register */
    // fsmc_write_handle.AddressSetupTime = 9;             /* Address setup time (ADDSET) = 9 fsmc_ker_ck cycles = 6*9=54ns */
    // fsmc_write_handle.AddressHoldTime = 0x00;           /* Address hold time (ADDHLD), not used in mode A */
    // fsmc_write_handle.DataSetupTime = 9;                /* Data setup time (DATAST) = 9 fsmc_ker_ck cycles = 6*9=54ns */
    //                                                     /* Note: some LCD driver ICs need a write pulse width of at least 50ns */
    // fsmc_write_handle.AccessMode = FSMC_ACCESS_MODE_A;  /* Mode A */
    //
    // HAL_SRAM_Init(&g_sram_handle, &fsmc_read_handle, &fsmc_write_handle);
    // osDelay(50);

    /* Try reading the 9341 ID */
    // uint8_t id[5];
    // lcd_wr_regno(0x04);
    // id[0] = lcd_rd_data();  /* dummy read */
    // id[1] = lcd_rd_data();  /* Read 0x00 */
    // id[2] = lcd_rd_data();  /* Read 0x93 */
    // id[3] = lcd_rd_data(); /* Read 0x41 */
    // id[4] = lcd_rd_data(); /* Read 0x41 */
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
    // id[1] = lcd_rd_data();  /* Read 0x00 */
    // id[2] = lcd_rd_data();  /* Read 0x93 */
    // id[3] = lcd_rd_data(); /* Read 0x41 */
    // printf("%x %x %x %x\r\n",id[0],id[1],id[2],id[3]);
    // app_delay(100);
    //
    // lcd_wr_regno(0x04);
    // id[0] = lcd_rd_data();  /* dummy read */
    // id[1] = lcd_rd_data();  /* Read 0x00 */
    // id[2] = lcd_rd_data();  /* Read 0x93 */
    // id[3] = lcd_rd_data(); /* Read 0x41 */
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
 * @brief       Clear the screen
 * @param       color: color to clear with
 * @retval      None
 */
void lcd_clear(uint16_t color)
{
    uint32_t index = 0;
    uint32_t totalpoint = lcddev.width;

    totalpoint *= lcddev.height;    /* Get the total number of points */
    lcd_set_cursor(0x00, 0x0000);   /* Set cursor position */
    lcd_write_ram_prepare();        /* Start writing GRAM */

    for (index = 0; index < totalpoint; index++)
    {
        LCD->LCD_RAM = color;
    }
}

/**
 * @brief       Fill a single color in the specified area
 * @param       (sx,sy),(ex,ey): diagonal coordinates of the fill rectangle, area size: (ex - sx + 1) * (ey - sy + 1)
 * @param       color:  color to fill (32-bit color, for LTDC compatibility)
 * @retval      None
 */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    uint16_t i, j;
    uint16_t xlen = 0;
    xlen = ex - sx + 1;

    for (i = sy; i <= ey; i++)
    {
        lcd_set_cursor(sx, i);      /* Set cursor position */
        lcd_write_ram_prepare();    /* Start writing GRAM */

        for (j = 0; j < xlen; j++)
        {
            LCD->LCD_RAM = color;   /* Display the color */
        }
    }
}

/**
 * @brief       Fill the specified area with a block of colors
 * @param       (sx,sy),(ex,ey): diagonal coordinates of the fill rectangle, area size: (ex - sx + 1) * (ey - sy + 1)
 * @param       color: start address of the color array to fill
 * @retval      None
 */
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    uint16_t height, width;
    uint16_t i, j;

    width = ex - sx + 1;            /* Get the fill width */
    height = ey - sy + 1;           /* Height */

    for (i = 0; i < height; i++)
    {
        lcd_set_cursor(sx, sy + i); /* Set cursor position */
        lcd_write_ram_prepare();    /* Start writing GRAM */

        for (j = 0; j < width; j++)
        {
            LCD->LCD_RAM = color[i * width + j]; /* Write the data */
        }
    }
}

/**
 * @brief       Draw a line
 * @param       x1,y1: start coordinates
 * @param       x2,y2: end coordinates
 * @param       color: line color
 * @retval      None
 */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, row, col;
    delta_x = x2 - x1;  /* Calculate coordinate increments */
    delta_y = y2 - y1;
    row = x1;
    col = y1;

    if (delta_x > 0)
    {
        incx = 1;   /* Set single-step direction */
    }
    else if (delta_x == 0)
    {
        incx = 0;   /* Vertical line */
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
        incy = 0;       /* Horizontal line */
    }
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }

    if ( delta_x > delta_y)
    {
        distance = delta_x;  /* Select the primary increment axis */
    }
    else
    {
        distance = delta_y;
    }

    for (t = 0; t <= distance + 1; t++ )        /* Line drawing output */
    {
        lcd_draw_point(row, col, color);        /* Draw point */
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
 * @brief       Draw a horizontal line
 * @param       x,y: start coordinates
 * @param       len  : line length
 * @param       color: line color
 * @retval      None
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
 * @brief       Draw a rectangle
 * @param       x1,y1: start coordinates
 * @param       x2,y2: end coordinates
 * @param       color: rectangle color
 * @retval      None
 */
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    lcd_draw_line(x1, y1, x2, y1, color);
    lcd_draw_line(x1, y1, x1, y2, color);
    lcd_draw_line(x1, y2, x2, y2, color);
    lcd_draw_line(x2, y1, x2, y2, color);
}

/**
 * @brief       Draw a circle
 * @param       x0,y0 : circle center coordinates
 * @param       r     : radius
 * @param       color : circle color
 * @retval      None
 */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a, b;
    int di;

    a = 0;
    b = r;
    di = 3 - (r << 1);       /* Flag for deciding the next point position */

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

        /* Draw the circle using the Bresenham algorithm */
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
 * @brief       Draw a filled circle
 * @param       x,y  : circle center coordinates
 * @param       r    : radius
 * @param       color: circle color
 * @retval      None
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
 * @brief       Power function, m^n
 * @param       m: base
 * @param       n: exponent
 * @retval      m raised to the power of n
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
 * @brief       Display len digits
 * @param       x,y : start coordinates
 * @param       num : value (0 ~ 2^32)
 * @param       len : number of digits to display
 * @param       size: font selection 12/16/24/32
 * @param       color : digit color
 * @retval      None
 */
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)   /* Loop over the total number of digits */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;   /* Get the digit of the corresponding position */

        if (enshow == 0 && t < (len - 1))               /* Leading zeros not enabled and more digits remain */
        {
            if (temp == 0)
            {
                lcd_show_char(x + (size / 2)*t, y, ' ', size, 0, color);    /* Display a space as a placeholder */
                continue;       /* Continue with the next digit */
            }
            else
            {
                enshow = 1;     /* Enable display */
            }
        }

        lcd_show_char(x + (size / 2)*t, y, temp + '0', size, 0, color); /* Display the character */
    }
}

/**
 * @brief       Extended display of len digits (leading zeros are displayed too)
 * @param       x,y : start coordinates
 * @param       num : value (0 ~ 2^32)
 * @param       len : number of digits to display
 * @param       size: font selection 12/16/24/32
 * @param       mode: display mode
 *              [7]: 0, no padding; 1, pad with 0.
 *              [6:1]: reserved
 *              [0]: 0, non-overlapping display; 1, overlapping display.
 * @param       color : digit color
 * @retval      None
 */
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)       /* Loop over the total number of digits */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;    /* Get the digit of the corresponding position */

        if (enshow == 0 && t < (len - 1))   /* Leading zeros not enabled and more digits remain */
        {
            if (temp == 0)
            {
                if (mode & 0x80)    /* High-order digits need zero padding */
                {
                    lcd_show_char(x + (size / 2)*t, y, '0', size, mode & 0x01, color);  /* Pad with 0 */
                }
                else
                {
                    lcd_show_char(x + (size / 2)*t, y, ' ', size, mode & 0x01, color);  /* Pad with a space */
                }

                continue;
            }
            else
            {
                enshow = 1;     /* Enable display */
            }
        }

        lcd_show_char(x + (size / 2)*t, y, temp + '0', size, mode & 0x01, color);
    }
}

/**
 * @brief       Display a string
 * @param       x,y         : start coordinates
 * @param       width,height: area size
 * @param       size        : font selection 12/16/24/32
 * @param       p           : start address of the string
 * @param       color       : string color
 * @retval      None
 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint8_t x0 = x;

    width += x;
    height += y;

    while ((*p <= '~') && (*p >= ' '))   /* Check whether the character is invalid! */
    {
        if (x >= width)
        {
            x = x0;
            y += size;
        }

        if (y >= height)
        {
            break;      /* Exit */
        }

        lcd_show_char(x, y, *p, size, 0, color);
        x += size / 2;
        p++;
    }
}