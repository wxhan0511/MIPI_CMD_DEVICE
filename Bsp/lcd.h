//
// Created by xuebin on 24-8-22.
//

#ifndef LCD_H
#define LCD_H

#include "stdlib.h"
#include "bsp.h"
//#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/
/* LCD RST/WR/RD/BL/CS/RS pin definitions
 * LCD_D0~D15 are not defined here because there are too many pins; modify them
 * directly in lcd_init. When porting, besides changing these 6 IOs, you must also
 * change the D0~D15 IOs in LCD_Init.
 */

/* RESET is shared with the system reset pin, so no RESET pin definition is needed here */
//#define LCD_RST_GPIO_PORT               GPIOx
//#define LCD_RST_GPIO_PIN                SYS_GPIO_PINx
//#define LCD_RST_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOx_CLK_ENABLE(); }while(0)   /* Enable the clock of the corresponding IO port */

#define LCD_WR_GPIO_PORT                GPIOD
#define LCD_WR_GPIO_PIN                 GPIO_PIN_5
#define LCD_WR_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)   /* Enable the clock of the corresponding IO port */

#define LCD_RD_GPIO_PORT                GPIOD
#define LCD_RD_GPIO_PIN                 GPIO_PIN_4
#define LCD_RD_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)   /* Enable the clock of the corresponding IO port */

#define LCD_BL_GPIO_PORT                GPIOH
#define LCD_BL_GPIO_PIN                 GPIO_PIN_12
#define LCD_BL_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOH_CLK_ENABLE(); }while(0)   /* Enable the clock of the backlight IO port */

/* LCD_CS (IO must be set correctly per LCD_FSMC_NEX) and LCD_RS (IO must be set correctly per LCD_FSMC_AX) pin definitions */
#define LCD_CS_GPIO_PORT                GPIOD
#define LCD_CS_GPIO_PIN                 GPIO_PIN_7
#define LCD_CS_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0)   /* Enable the clock of the corresponding IO port */

#define LCD_RS_GPIO_PORT                GPIOD
#define LCD_RS_GPIO_PIN                 GPIO_PIN_11
#define LCD_RS_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0)   /* Enable the clock of the corresponding IO port */

/* FSMC-related parameter definitions
 * Note: by default the LCD is connected to FSMC bank1, which has 4 chip selects: FSMC_NE1~4
 *
 * If LCD_FSMC_NEX is changed, the related LCD_CS_GPIO settings must be changed too
 * If LCD_FSMC_AX is changed, the related LCD_RS_GPIO settings must be changed too
 */
#define LCD_FSMC_NEX         1              /* LCD_CS on FSMC_NE4; valid range: 1~4 only */
#define LCD_FSMC_AX          16             /* LCD_RS on FSMC_A6; valid range: 0~25 */

#define LCD_FSMC_BCRX        FSMC_Bank1->BTCR[(LCD_FSMC_NEX - 1) * 2]       /* BCR register, computed automatically from LCD_FSMC_NEX */
#define LCD_FSMC_BTRX        FSMC_Bank1->BTCR[(LCD_FSMC_NEX - 1) * 2 + 1]   /* BTR register, computed automatically from LCD_FSMC_NEX */
#define LCD_FSMC_BWTRX       FSMC_Bank1E->BWTR[(LCD_FSMC_NEX - 1) * 2]      /* BWTR register, computed automatically from LCD_FSMC_NEX */

/******************************************************************************************/

/* Key LCD parameters */
typedef struct
{
    uint16_t width;     /* LCD width */
    uint16_t height;    /* LCD height */
    uint16_t id;        /* LCD ID */
    uint8_t dir;        /* Portrait or landscape: 0, portrait; 1, landscape. */
    uint16_t wramcmd;   /* GRAM write start command */
    uint16_t setxcmd;   /* Set X coordinate command */
    uint16_t setycmd;   /* Set Y coordinate command */
} _lcd_dev;

/* LCD parameters */
extern _lcd_dev lcddev; /* Manages key LCD parameters */

/* LCD pen color and background color */
extern uint32_t  g_point_color;     /* Default red */
extern uint32_t  g_back_color;      /* Background color, default white */

/* LCD backlight control */
#define LCD_BL(x)   do{ x ? \
                      HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_SET) : \
                      HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_RESET); \
                     }while(0)

/* LCD address structure */
typedef struct
{
    volatile uint8_t LCD_REG;
    volatile uint8_t LCD_RAM;
} LCD_TypeDef;


/* Detailed derivation of LCD_BASE:
 * We generally use FSMC bank1 (BANK1) to drive TFTLCD panels (MCU panels). Bank1 has a
 * total address range of 256MB, evenly divided into 4 blocks:
 * Bank1 (FSMC_NE1) address range: 0x6000 0000 ~ 0x63FF FFFF
 * Bank2 (FSMC_NE2) address range: 0x6400 0000 ~ 0x67FF FFFF
 * Bank3 (FSMC_NE3) address range: 0x6800 0000 ~ 0x6BFF FFFF
 * Bank4 (FSMC_NE4) address range: 0x6C00 0000 ~ 0x6FFF FFFF
 *
 * We must choose the proper chip select (wired to LCD_CS) and address line (wired to
 * LCD_RS) according to the hardware connection.
 * The F407 motor dev board uses FSMC_NE4 for LCD_CS and FSMC_A10 for LCD_RS, with a
 * 16-bit data bus. The calculation is as follows:
 * The base address of FSMC_NE4 is: 0x6C00 0000; the base of NEx (x=1/2/3/4) is:
 * 0x6000 0000 + (0x400 0000 * (x - 1))
 * FSMC_A10 corresponds to address 2^10 * 2 = 0x800; FSMC_Ay corresponds to (y = 0~25): 2^y * 2
 *
 * LCD->LCD_REG corresponds to LCD_RS = 0 (LCD register); LCD->LCD_RAM corresponds to
 * LCD_RS = 1 (LCD data)
 * Then the address of LCD->LCD_RAM is:  0x6C00 0000 + 2^10 * 2 = 0x6C00 0800
 * The address of LCD->LCD_REG can be any address other than LCD->LCD_RAM.
 * Since we use a struct to manage LCD_REG and LCD_RAM (REG first, RAM after, both
 * 16-bit data width)
 * the struct base address (LCD_BASE) = LCD_RAM - 2 = 0x6C00 0800 -2
 *
 * A more general formula ((chip select FSMC_NEx, x=1/2/3/4; RS on address line FSMC_Ay, y=0~25)):
 *          LCD_BASE = (0x6000 0000 + (0x400 0000 * (x - 1))) | (2^y * 2 -2)
 *          Equivalent (using shift operations):
 *          LCD_BASE = (0x6000 0000 + (0x400 0000 * (x - 1))) | ((1 << y) * 2 -2)
 */
#define LCD_BASE        (uint32_t)((0x60000000 + (0x4000000 * (LCD_FSMC_NEX - 1))) | (((1 << LCD_FSMC_AX) * 2) -2))
#define LCD_BASE2        (uint32_t)((0x60000000 + (0x4000000 * (LCD_FSMC_NEX - 1))))
#define LCD_BASE3        (uint32_t)((0x60000000 + (0x4000000 * (2 - 1))) | (((1 << LCD_FSMC_AX) * 2) -2))
#define LCD_BASE4        (uint32_t)((0x60000000 + (0x4000000 * (2 - 1))))
#define LCD             ((LCD_TypeDef *) LCD_BASE)
#define LCD2            ((LCD_TypeDef *) LCD_BASE2)
#define LCD3            ((LCD_TypeDef *) LCD_BASE3)
#define LCD4            ((LCD_TypeDef *) LCD_BASE4)
/******************************************************************************************/
/* LCD scan direction and color definitions */

/* Scan direction definitions */
#define L2R_U2D         0           /* Left to right, top to bottom */
#define L2R_D2U         1           /* Left to right, bottom to top */
#define R2L_U2D         2           /* Right to left, top to bottom */
#define R2L_D2U         3           /* Right to left, bottom to top */

#define U2D_L2R         4           /* Top to bottom, left to right */
#define U2D_R2L         5           /* Top to bottom, right to left */
#define D2U_L2R         6           /* Bottom to top, left to right */
#define D2U_R2L         7           /* Bottom to top, right to left */

#define DFT_SCAN_DIR    L2R_U2D     /* Default scan direction */

/* Common pen colors */
#define WHITE           0xFFFF      /* White */
#define BLACK           0x0000      /* Black */
#define RED             0xF800      /* Red */
#define GREEN           0x07E0      /* Green */
#define BLUE            0x001F      /* Blue */
#define MAGENTA         0xF81F      /* Magenta/purple = BLUE + RED */
#define YELLOW          0xFFE0      /* Yellow = GREEN + RED */
#define CYAN            0x07FF      /* Cyan = GREEN + BLUE */

/* Less common colors */
#define BROWN           0xBC40      /* Brown */
#define BRRED           0xFC07      /* Brownish red */
#define GRAY            0x8430      /* Gray */
#define DARKBLUE        0x01CF      /* Dark blue */
#define LIGHTBLUE       0x7D7C      /* Light blue */
#define GRAYBLUE        0x5458      /* Grayish blue */
#define LIGHTGREEN      0x841F      /* Light green */
#define LGRAY           0xC618      /* Light gray (panel), window background color */
#define LGRAYBLUE       0xA651      /* Light gray-blue (middle-layer color) */
#define LBBLUE          0x2B12      /* Light brown-blue (inverse of the selected item) */

/******************************************************************************************/
/* SSD1963-related configuration parameters (normally no need to change) */

/* LCD resolution settings */
#define SSD_HOR_RESOLUTION      320     /* LCD horizontal resolution */
#define SSD_VER_RESOLUTION      240     /* LCD vertical resolution */

/* LCD driver parameter settings */
#define SSD_HOR_PULSE_WIDTH     1       /* Horizontal pulse width */
#define SSD_HOR_BACK_PORCH      46      /* Horizontal back porch */
#define SSD_HOR_FRONT_PORCH     210     /* Horizontal front porch */

#define SSD_VER_PULSE_WIDTH     1       /* Vertical pulse width */
#define SSD_VER_BACK_PORCH      23      /* Vertical back porch */
#define SSD_VER_FRONT_PORCH     22      /* Vertical front porch */

/* The following parameters are computed automatically */
#define SSD_HT          (SSD_HOR_RESOLUTION + SSD_HOR_BACK_PORCH + SSD_HOR_FRONT_PORCH)
#define SSD_HPS         (SSD_HOR_BACK_PORCH)
#define SSD_VT          (SSD_VER_RESOLUTION + SSD_VER_BACK_PORCH + SSD_VER_FRONT_PORCH)
#define SSD_VPS         (SSD_VER_BACK_PORCH)

/******************************************************************************************/
/* Function declarations */

void lcd_wr_data(volatile uint16_t data);            /* LCD write data */
void lcd_wr_regno(volatile uint16_t regno);          /* LCD write register number/address */
void lcd_write_reg(uint16_t regno, uint16_t data);   /* LCD write register value */
void lcd_write_cmd_8bit(uint8_t cmd);                /* LCD write command (8-bit bus) */
void lcd_write_data_8bit(uint8_t data);              /* LCD write data (8-bit bus) */

void lcd_init(void);                        /* Initialize the LCD */
void lcd_display_on(void);                  /* Turn display on */
void lcd_display_off(void);                 /* Turn display off */
void lcd_scan_dir(uint8_t dir);             /* Set screen scan direction */
void lcd_display_dir(uint8_t dir);          /* Set screen display direction */
void lcd_ssd_backlight_set(uint8_t pwm);    /* SSD1963 backlight control */

void lcd_write_ram_prepare(void);               /* Prepare to write GRAM */
void lcd_set_cursor(uint16_t x, uint16_t y);    /* Set cursor */
uint32_t lcd_read_point(uint16_t x, uint16_t y);/* Read point (32-bit color, LTDC compatible)  */
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);/* Draw point (32-bit color, LTDC compatible) */

void lcd_clear(uint16_t color);     /* LCD clear */
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);                   /* Fill a solid circle */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);                  /* Draw a circle */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color);                  /* Draw a horizontal line */
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);             /* Set window */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);          /* Fill rectangle with a solid color (32-bit color, LTDC compatible) */
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);   /* Fill rectangle with colors */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);     /* Draw a line */
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);/* Draw a rectangle */


void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color);
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color);
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color);

uint16_t lcd_rd_data(void);
void lcd_set_cursor_address(uint16_t x_start, uint16_t y_start,uint16_t x_end, uint16_t y_end);
#endif //LCD_H
