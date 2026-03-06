/*
 * ============================================================================
 * pin_defs.h
 * Consolidated GPIO pin/port definitions generated from existing project
 * and schematic. Keep names consistent with existing code.
 * ============================================================================
 */
#ifndef __PIN_DEFS_H
#define __PIN_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f4xx_hal.h"

/* ============================================================================
 * ADS1256 and W25Q256JVEQ SPI Pin Definitions
 * ============================================================================
/*For Human Machine Interface ADS1256*/
#define ADC_DRDY2_Pin GPIO_PIN_1
#define ADC_DRDY2_GPIO_Port GPIOA
#define ADC_SPI_CS2_Pin GPIO_PIN_2
#define ADC_SPI_CS2_GPIO_Port GPIOA

/*For ADS1256*/
#define ADC_DRDY1_Pin GPIO_PIN_2
#define ADC_DRDY1_GPIO_Port GPIOA
#define ADC_SPI_CS1_Pin GPIO_PIN_1
#define ADC_SPI_CS1_GPIO_Port GPIOA

#define ADC_RESET_Pin GPIO_PIN_0
#define ADC_RESET_GPIO_Port GPIOA
#define ADC_SPI_MOSI_Pin GPIO_PIN_7
#define ADC_SPI_MOSI_GPIO_Port GPIOA
#define ADC_SPI_MISO_Pin GPIO_PIN_6
#define ADC_SPI_MISO_GPIO_Port GPIOA
#define ADC_SPI_CLK_Pin GPIO_PIN_5
#define ADC_SPI_CLK_GPIO_Port GPIOA
/* ============================================================================
 * Flash SPI Pin Definitions
 * ============================================================================ */
#define FLASH_MOSI_Pin GPIO_PIN_11
#define FLASH_MOSI_GPIO_Port GPIOC
#define FLASH_MISO_Pin GPIO_PIN_10
#define FLASH_MISO_GPIO_Port GPIOC
#define FLASH_CLK_Pin GPIO_PIN_13
#define FLASH_CLK_GPIO_Port GPIOC
#define FLASH_CS_Pin GPIO_PIN_12
#define FLASH_CS_GPIO_Port GPIOC
/* ============================================================================
 * Latch Data Pin Definitions
 * ============================================================================ */
#define LATCH_0_Pin GPIO_PIN_11
#define LATCH_0_GPIO_Port GPIOA
#define LATCH_1_Pin GPIO_PIN_10
#define LATCH_1_GPIO_Port GPIOA
#define LATCH_2_Pin GPIO_PIN_9
#define LATCH_2_GPIO_Port GPIOA
#define LATCH_3_Pin GPIO_PIN_8
#define LATCH_3_GPIO_Port GPIOA
#define LATCH_4_Pin GPIO_PIN_9
#define LATCH_4_GPIO_Port GPIOC
#define LATCH_5_Pin GPIO_PIN_8
#define LATCH_5_GPIO_Port GPIOC
#define LATCH_6_Pin GPIO_PIN_7
#define LATCH_6_GPIO_Port GPIOC
#define LATCH_7_Pin GPIO_PIN_6
#define LATCH_7_GPIO_Port GPIOC
/* ============================================================================
 * D Trigger Clock Pin Definitions
 * ============================================================================ */
#define D_CLK_1_Pin GPIO_PIN_13
#define D_CLK_1_GPIO_Port GPIOD
#define D_CLK_2_Pin GPIO_PIN_4
#define D_CLK_2_GPIO_Port GPIOA
#define D_CLK_3_Pin GPIO_PIN_12
#define D_CLK_3_GPIO_Port GPIOD
#define D_CLK_4_Pin GPIO_PIN_6
#define D_CLK_4_GPIO_Port GPIOB
#define D_CLK_5_Pin GPIO_PIN_2
#define D_CLK_5_GPIO_Port GPIOD
#define D_CLK_6_Pin GPIO_PIN_8
#define D_CLK_6_GPIO_Port GPIOB
#define D_CLK_7_Pin GPIO_PIN_9
#define D_CLK_7_GPIO_Port GPIOB
#define D_CLK_8_Pin GPIO_PIN_4
#define D_CLK_8_GPIO_Port GPIOB
/* ============================================================================
 * PWM Pin Definitions
 * ============================================================================ */
#define PWM_FUNC_Pin GPIO_PIN_13
#define PWM_FUNC_GPIO_Port GPIOE
#define Pulse_FUNC_Pin GPIO_PIN_14
#define Pulse_FUNC_GPIO_Port GPIOE

#define LED_PWM_IN GPIO_PIN_12
#define LED_PWM_IN_GPIO_Port GPIOE
/* ============================================================================
 * I2C Pin Definitions
 * ============================================================================ */
#define I2C_SCL_Pin GPIO_PIN_6
#define I2C_SCL_GPIO_Port GPIOB
#define I2C_SDA_Pin GPIO_PIN_7
#define I2C_SDA_GPIO_Port GPIOB
#define TP_I2C_SCL_Pin GPIO_PIN_10
#define TP_I2C_SCL_GPIO_Port GPIOB
#define TP_I2C_SDA_Pin GPIO_PIN_11
#define TP_I2C_SDA_GPIO_Port GPIOB
/* ============================================================================
 * DAC Pin Definitions
 * ============================================================================ */
#define DAC_LDAC1_Pin GPIO_PIN_0
#define DAC_LDAC1_GPIO_Port GPIOB
#define DAC_BSY1_Pin GPIO_PIN_1
#define DAC_BSY1_GPIO_Port GPIOB

#define DAC_LDAC2_Pin GPIO_PIN_5   
#define DAC_LDAC2_GPIO_Port GPIOB
#define DAC_BSY2_Pin GPIO_PIN_7
#define DAC_BSY2_GPIO_Port GPIOB

#define DAC_LDAC3_Pin GPIO_PIN_14
#define DAC_LDAC3_GPIO_Port GPIOB
#define DAC_BSY3_Pin GPIO_PIN_15
#define DAC_BSY3_GPIO_Port GPIOB

#define DAC_LDAC4_Pin GPIO_PIN_3
#define DAC_LDAC4_GPIO_Port GPIOB
#define DAC_BSY4_Pin GPIO_PIN_12
#define DAC_BSY4_GPIO_Port GPIOA

#define DAC_LDAC5_Pin GPIO_PIN_15
#define DAC_LDAC5_GPIO_Port GPIOA
#define DAC_BSY5_Pin GPIO_PIN_5
#define DAC_BSY5_GPIO_Port GPIOC
/* ============================================================================
 * TSPI Pin Definitions
 * ============================================================================ */
#define TSPI_INT_IN_Pin GPIO_PIN_8
#define TSPI_INT_IN_GPIO_Port GPIOC
#define TSPI_CLK_Pin GPIO_PIN_9
#define TSPI_CLK_GPIO_Port GPIOC
#define TSPI_MOSI_Pin GPIO_PIN_11
#define TSPI_MOSI_GPIO_Port GPIOC
#define TSPI_MISO_Pin GPIO_PIN_10
#define TSPI_MISO_GPIO_Port GPIOC
#define TSPI_CS_Pin GPIO_PIN_12
#define TSPI_CS_GPIO_Port GPIOC
#define TP_INT_Pin GPIO_PIN_13
#define TP_INT_GPIO_Port GPIOC
/* ============================================================================
 * M SPI Pin Definitions
 * ============================================================================ */
#define M_CS_Pin GPIO_PIN_0
#define M_CS_GPIO_Port GPIOC
#define M_SCK_Pin GPIO_PIN_1
#define M_SCK_GPIO_Port GPIOC
#define M_MISO_Pin GPIO_PIN_2
#define M_MISO_GPIO_Port GPIOC
#define M_MOSI_Pin GPIO_PIN_3
#define M_MOSI_GPIO_Port GPIOC
#define M_INT_Pin GPIO_PIN_4
#define M_INT_GPIO_Port GPIOC
/* ============================================================================
 * LCD Related Pin Definitions
 * ============================================================================ */
#define LCD_RESET_Pin GPIO_PIN_6
#define LCD_RESET_GPIO_Port GPIOD
#define LCD_CS_Pin GPIO_PIN_7
#define LCD_CS_GPIO_Port GPIOD
#define LCD_RS_Pin GPIO_PIN_11
#define LCD_RS_GPIO_Port GPIOD
#define LCD_WR_Pin GPIO_PIN_5
#define LCD_WR_GPIO_Port GPIOD
#define LCD_RD_Pin GPIO_PIN_4
#define LCD_RD_GPIO_Port GPIOD
#define LCD_D0_Pin GPIO_PIN_14
#define LCD_D0_GPIO_Port GPIOD
#define LCD_D1_Pin GPIO_PIN_15
#define LCD_D1_GPIO_Port GPIOD
#define LCD_D2_Pin GPIO_PIN_0
#define LCD_D2_GPIO_Port GPIOD
#define LCD_D3_Pin GPIO_PIN_1
#define LCD_D3_GPIO_Port GPIOD
#define LCD_D4_Pin GPIO_PIN_7
#define LCD_D4_GPIO_Port GPIOE
#define LCD_D5_Pin GPIO_PIN_8
#define LCD_D5_GPIO_Port GPIOE
#define LCD_D6_Pin GPIO_PIN_9
#define LCD_D6_GPIO_Port GPIOE
#define LCD_D7_Pin GPIO_PIN_10
#define LCD_D7_GPIO_Port GPIOE
#define LCD_BL_EN_Pin GPIO_PIN_5
#define LCD_BL_EN_GPIO_Port GPIOE
/* ============================================================================
 * OE Pin Definitions
 * ============================================================================ */
#define OE_GPIO_Port GPIOA
#define OE_Pin GPIO_PIN_12  
#define DAC_LS_GPIO_Port GPIOA
#define DAC_LS_Pin GPIO_PIN_4

/* ============================================================================
 * Key Pin Definitions
 * ============================================================================ */
#define KEY1_Pin GPIO_PIN_12
#define KEY1_GPIO_Port GPIOB
#define KEY2_Pin GPIO_PIN_13
#define KEY2_GPIO_Port GPIOB
/*  ============================================================================
 * Pulse Pin Definitions
 * ============================================================================ */
#define PULSE_A_Pin GPIO_PIN_14
#define PULSE_A_GPIO_Port GPIOE
#define PULSE_B_Pin GPIO_PIN_15
#define PULSE_B_GPIO_Port GPIOE
#ifdef __cplusplus
}
#endif

#endif /* __PIN_DEFS_H */
