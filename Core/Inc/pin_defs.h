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


/*For Human Machine Interface ADS1256*/
#define ADC_DRDY2_Pin GPIO_PIN_1
#define ADC_DRDY2_GPIO_Port GPIOA
#define ADC_SPI_CS2_Pin GPIO_PIN_2
#define ADC_SPI_CS2_GPIO_Port GPIOA
/*For W25Q256JVEQ*/
#define FLASH_CS_Pin GPIO_PIN_8
#define FLASH_CS_GPIO_Port GPIOC
/*For IC power board ADS1256*/
#define ADC_DRDY1_Pin GPIO_PIN_11
#define ADC_DRDY1_GPIO_Port GPIOA
#define ADC_SPI_CS1_Pin GPIO_PIN_12
#define ADC_SPI_CS1_GPIO_Port GPIOA
/*For ADS1256, IC power board, W25Q256JVEQ*/
#define ADC_RESET_Pin GPIO_PIN_0
#define ADC_RESET_GPIO_Port GPIOA
#define ADC_SPI_MOSI_Pin GPIO_PIN_7
#define ADC_SPI_MOSI_GPIO_Port GPIOA
#define ADC_SPI_MISO_Pin GPIO_PIN_6
#define ADC_SPI_MISO_GPIO_Port GPIOA
#define ADC_SPI_CLK_Pin GPIO_PIN_5
#define ADC_SPI_CLK_GPIO_Port GPIOA

#define FLASH_MOSI_Pin ADC_SPI_MOSI_Pin
#define FLASH_MOSI_GPIO_Port ADC_SPI_MOSI_GPIO_Port
#define FLASH_MISO_Pin ADC_SPI_MISO_Pin
#define FLASH_MISO_GPIO_Port ADC_SPI_MISO_GPIO_Port
#define FLASH_CLK_Pin ADC_SPI_CLK_Pin
#define FLASH_CLK_GPIO_Port ADC_SPI_CLK_GPIO_Port
/* ============================================================================
 * ADS1256 and W25Q256JVEQ SPI Pin Definitions
 * ============================================================================
 */
#define LATCH_D1_Pin GPIO_PIN_0
#define LATCH_D1_GPIO_Port GPIOE
#define LATCH_D2_Pin GPIO_PIN_1
#define LATCH_D2_GPIO_Port GPIOE
#define LATCH_D3_Pin GPIO_PIN_2
#define LATCH_D3_GPIO_Port GPIOE
#define LATCH_D4_Pin GPIO_PIN_3
#define LATCH_D4_GPIO_Port GPIOE
#define LATCH_D5_Pin GPIO_PIN_4
#define LATCH_D5_GPIO_Port GPIOE
#define LATCH_D6_Pin GPIO_PIN_5
#define LATCH_D6_GPIO_Port GPIOE
#define LATCH_D7_Pin GPIO_PIN_6
#define LATCH_D7_GPIO_Port GPIOE
#define LATCH_D0_Pin GPIO_PIN_12
#define LATCH_D0_GPIO_Port GPIOE
/* ============================================================================
 * Latch Data Pin Definitions (PE0–PE6, PE12)
 * ============================================================================
 */
#define LATCH0_CP_Pin GPIO_PIN_8
#define LATCH0_CP_GPIO_Port GPIOA
#define LATCH1_CP_Pin GPIO_PIN_0
#define LATCH1_CP_GPIO_Port GPIOB
#define LATCH2_CP_Pin GPIO_PIN_1
#define LATCH2_CP_GPIO_Port GPIOB
/* ============================================================================
 * Latch Clock Pin Definitions
 * ============================================================================
 */
#define PWM_FUNC_Pin GPIO_PIN_13
#define PWM_FUNC_GPIO_Port GPIOE
#define Pulse_FUNC_Pin GPIO_PIN_14
#define Pulse_FUNC_GPIO_Port GPIOE
//I = 1000 * D%, PWM frequency range 100~100KHz, recommended 10KHz, duty cycle resolution 0.1%
#define LED_PWM_IN_Pin GPIO_PIN_11
#define LED_PWM_IN_GPIO_Port GPIOE
/* ============================================================================
 * PWM Related Pin Definitions
 * ============================================================================
 */

/* -------------------- I2C Pin Definitions -------------------- */
#define I2C_SCL_Pin GPIO_PIN_6
#define I2C_SCL_GPIO_Port GPIOB
#define I2C_SDA_Pin GPIO_PIN_7
#define I2C_SDA_GPIO_Port GPIOB
#define TP_I2C_SCL_Pin GPIO_PIN_10
#define TP_I2C_SCL_GPIO_Port GPIOB
#define TP_I2C_SDA_Pin GPIO_PIN_11
#define TP_I2C_SDA_GPIO_Port GPIOB
/* ============================================================================
 * I2C Pin Definitions
 * ============================================================================
 */
#define DAC_LDAC1_Pin GPIO_PIN_0
#define DAC_LDAC1_GPIO_Port GPIOC
#define DAC_BSY1_Pin GPIO_PIN_1
#define DAC_BSY1_GPIO_Port GPIOC
#define DAC_LDAC3_Pin GPIO_PIN_4
#define DAC_LDAC3_GPIO_Port GPIOC
#define DAC_BSY3_Pin GPIO_PIN_5
#define DAC_BSY3_GPIO_Port GPIOC
#define DAC_LDAC2_Pin GPIO_PIN_2
#define DAC_LDAC2_GPIO_Port GPIOC
#define DAC_BSY2_Pin GPIO_PIN_3
#define DAC_BSY2_GPIO_Port GPIOC
/* ============================================================================
 * DAC Pin Definitions
 * ============================================================================
 */
#define RLY1_Pin GPIO_PIN_5
#define RLY1_GPIO_Port GPIOCB
#define RLY2_Pin GPIO_PIN_12
#define RLY2_GPIO_Port GPIOCA
#define RLY3_Pin GPIO_PIN_6
#define RLY3_GPIO_Port GPIOC
#define RLY4_Pin GPIO_PIN_7
#define RLY4_GPIO_Port GPIOC
/* ============================================================================
 * Relay Pin Definitions
 * ============================================================================
 */
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
 * TSPI Pin Definitions
 * ============================================================================
 */
#define M_CS_Pin GPIO_PIN_12
#define M_CS_GPIO_Port GPIOB
#define M_SCK_Pin GPIO_PIN_13
#define M_SCK_GPIO_Port GPIOB
#define M_MISO_Pin GPIO_PIN_14
#define M_MISO_GPIO_Port GPIOB
#define M_MOSI_Pin GPIO_PIN_15
#define M_MOSI_GPIO_Port GPIOB
#define M_INT_Pin GPIO_PIN_12
#define M_INT_GPIO_Port GPIOD
/* ============================================================================
 * M SPI Pin Definitions
 * ============================================================================
 */
#define LCD_RESET1_Pin GPIO_PIN_6
#define LCD_RESET1_GPIO_Port GPIOD
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
#define LCD_BL_EN_Pin GPIO_PIN_15
#define LCD_BL_EN_GPIO_Port GPIOA
/* ============================================================================
 * LCD Related Pin Definitions
 * ============================================================================
 */

#define STM32_TX_Pin GPIO_PIN_9
#define STM32_TX_GPIO_Port GPIOA
#define STM32_RX_Pin GPIO_PIN_10
#define STM32_RX_GPIO_Port GPIOA
/* ============================================================================
 * UART Pin Definitions
 * ============================================================================
 */
//Output Enable Pin for Level Shifter and Latchs
//Attention:The latch outputs Q0–Q7 are driven only when OE# is pulled high
#define OE_GPIO_Port GPIOD
#define OE_Pin GPIO_PIN_13  
/* ============================================================================
 * OE Pin Definitions
 * ============================================================================
 */

#ifdef __cplusplus
}
#endif

#endif /* __PIN_DEFS_H */
