#ifndef __BSP_SPI_BUS_H
#define __BSP_SPI_BUS_H


#define SPIx_CLK_ENABLE()			__HAL_RCC_SPI4_CLK_ENABLE()

#define SPIx_SCK_CLK_ENABLE()		__HAL_RCC_GPIOE_CLK_ENABLE()
#define SPIx_SCK_GPIO				GPIOE
#define SPIx_SCK_PIN				GPIO_PIN_2
#define SPIx_SCK_AF					GPIO_AF5_SPI4

#define SPIx_MISO_CLK_ENABLE()		__HAL_RCC_GPIOE_CLK_ENABLE()
#define SPIx_MISO_GPIO				GPIOE
#define SPIx_MISO_PIN 				GPIO_PIN_5
#define SPIx_MISO_AF				GPIO_AF5_SPI4

#define SPIx_MOSI_CLK_ENABLE()		__HAL_RCC_GPIOE_CLK_ENABLE()
#define SPIx_MOSI_GPIO				GPIOE
#define SPIx_MOSI_PIN 				GPIO_PIN_6
#define SPIx_MOSI_AF				GPIO_AF5_SPI4

#define SPIx_CS_CLK_ENABLE()		__HAL_RCC_GPIOE_CLK_ENABLE()
#define SPIx_CS_GPIO				GPIOE
#define SPIx_CS_PIN 				GPIO_PIN_4


#define	SPI_BUFFER_SIZE		(4 * 1024)				/*  */
#define Flash_TotalSize   (32 * 1024 * 1024)
#define Flash_SectorSize  (4 * 1024)

#define SF_CS_H()					GPIOC->BSRR = GPIO_PIN_9
#define SF_CS_L()					GPIOC->BSRR = ((uint32_t)GPIO_PIN_9 << 16U)


#define CMD_AAI       0xAD
#define CMD_DISWR	  0x04
#define CMD_EWRSR	  0x50
#define CMD_WRSR      0x01
#define CMD_WREN      0x06
#define CMD_READ      0x13
#define CMD_RDSR      0x05
#define CMD_RDID      0x9F
#define CMD_SE        0x21
#define CMD_BE        0xC7
#define DUMMY_BYTE    0xA5
#define WIP_FLAG      0x01


void bsp_flash_erase_sector(uint32_t addr);
void bsp_flash_erase_chip(void);
uint32_t bsp_flash_read_id(void);
void bsp_flash_read(uint8_t * p_buf, uint32_t read_addr, uint32_t read_size);
uint8_t bsp_flash_write(uint8_t* p_buf, uint32_t write_addr, uint16_t write_size);
uint8_t bsp_flash_read_status(void);
void QSPI_FLASH_Wait_Busy(void);



#endif


