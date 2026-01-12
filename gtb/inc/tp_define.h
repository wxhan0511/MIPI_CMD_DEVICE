//
// Created by Bobby on 2023/7/4.
//

#ifndef AE_TOOL_MOTHER_BOARD_PROJECT_TP_DEFINE_H
#define AE_TOOL_MOTHER_BOARD_PROJECT_TP_DEFINE_H

//CMD for USB communication with GTB
//GTB operation
#define CMD_I_AM_GTB										0X40
#define CMD_RESET_FLOW									0XF0
#define CMD_READ_GTB_VERSION						0XF1
#define CMD_READ_GTB_ID									0XF2
#define CMD_WRITE_GTB_GPIO_BIT					0XF3
#define CMD_WRITE_GTB_GPIO_PORT					0XF4
#define CMD_READ_GTB_GPIO_BIT						0XF5
#define CMD_CONFIG_GTB_GPIO_PORT				0XFB

//com debug
#define CMD_COM_OPERATION         	 		0XF6
#define CMD_COM_OPEN              			0XD6
#define CMD_COM_CLOSE             			0XC6
#define CMD_COM_SEND              			0XB6

//LDO control
#define CMD_LDO_CONTROL        	 				0XF7
#define CMD_LDO_ON		        	 				0XD7
#define CMD_LDO_OFF		        	 				0XC7

//i2c and spi transfer
#define CMD_I2CSPI_MODE_I2C    					0
#define CMD_I2CSPI_MODE_SPI    					1
#define CMD_I2CSPIUSART_CHANGE_INFO    	0XF8
#define CMD_I2CSPI_CHANGE_SPEED       	0XD8
#define CMD_I2CSPI_CHANGE_SLAVE_ADDRESS 0XC8
#define CMD_I2CSPI_CHANGE_SPI_CONFIG	 	0XB8
#define CMD_I2CSPI_DEINIT_SPI					 	0XA8
#define CMD_I2CSPIUSART_CHANGE_INFO_I2C1		1
#define CMD_I2CSPIUSART_CHANGE_INFO_SPI2		2
#define CMD_I2CSPIUSART_CHANGE_INFO_USART2	3

//misc functions
#define CMD_MISC_FUNCTION								0XFC
#define CMD_I2C_HW_SW_SELECT						0X00
#define CMD_I2C_HW_ENABLE								0
#define CMD_I2C_SW_ENABLE								1

#define CMD_FW_MODE_SET									0X01

#define CMD_TRANSFER_FEEDBACK_SET				0X02
#define CMD_TRANSFER_FEEDBACK_DISABLE		0
#define CMD_TRANSFER_FEEDBACK_ENABLE		1

#define CMD_DEBUG_MODE_SET 							0X03
#define CMD_DEBUG_DISABLE								0
#define CMD_DEBUG_ENABLE								1

#define CMD_INT_TRANSFER_SET 						0X04
#define CMD_INT_TRANSFER_DISABLE				1
#define CMD_INT_TRANSFER_ENABLE					0

#define CMD_I2C_TRANSFER		  	 				0XF9
#define CMD_SPI_TRANSFER		  	 				0XFA
#define CMD_I2CSPI_TRANSFER_WRITE 	 		1
#define CMD_I2CSPI_TRANSFER_READ				2
#define CMD_I2CSPI_TRANSFER_REG					1
#define CMD_I2CSPI_TRANSFER_DATA				2
#define CMD_I2CSPI_TRANSFER_HOSTDOWNLOAD 	0x55
#define CMD_I2CSPI_TRANSFER_IDM						0x66
#define CMD_I2CSPI_TRANSFER_LONG_PACKET		0x65
#define CMD_SPI_TRANSFER_OFFLINE_ENABLE	3
#define CMD_I2C_TRANSFER_OFFLINE_ENABLE	4
#define CMD_FLASH_OPERATION_LONG_PACKET	5

#define CMD_HOSTDOWNLOAD_WAIT_DATA			0xAA

#define SPI2_CPOL0_CPHA0								0
#define SPI2_CPOL0_CPHA1								1
#define SPI2_CPOL1_CPHA0								2
#define SPI2_CPOL1_CPHA1								3

//fw mode
#define FWMODE_DISABLE									0XFF
#define FWMODE_DEMO											0
#define FWMODE_RAWDATA									1
#define FWMODE_DEBUG										2

//off line mode
#define OFFLINE_SPI											3
#define OFFLINE_I2C											4

//IC type
#define IC_TYPE_GC											0
#define IC_TYPE_NT											1
#define IC_TYPE_MSTAR										2

//IC index
#define IC_INDEX_GC_7271								0
#define IC_INDEX_GC_7371								1
#define IC_INDEX_GC_7202								2
#define IC_INDEX_GC_7302								3

//dynamic cmd
#define CMD_SPI_DYNAMIC_CONTROL         0XFD
#define CMD_I2C_DYNAMIC_CONTROL         0XFE
#define DYNAMIC_WRITE                   0
#define DYNAMIC_READ                    1
#define DYNAMIC_DELAY                   2


//test cmd
#define CMD_TEST												0XFF


//flash operation
#define CMD_FLASH_OPERATION_START				0XE1
#define CMD_FLASH_ID_START							0XD0
#define CMD_FLASH_ERASE_START						0XD1
#define CMD_FLASH_PROGRAM_START					0XD2
#define CMD_FLASH_READ_START						0XD3

#define CMD_FLASH_OPERATION_IDM           0XE2
#define CMD_FLASH_PROGRAM_NON_OFFLINE_IDM 0X01
#define CMD_FLASH_PROGRAM_OFFLINE_IDM     0X02

#define CMD_FLASH_ID_END								0XC0
#define CMD_FLASH_ERASE_END							0XC1
#define CMD_FLASH_PROGRAM_END						0XC2
#define CMD_FLASH_READ_END							0XC3
#define CMD_FLASH_READ_ERROR						0XB3
#define CMD_FLASH_EX										0X00
#define CMD_FLASH_OB										0X01
#define CMD_FLASH_OF_SPI								0X02
#define CMD_FLASH_OF_I2C								0X03

#define CMD_USB_TRANSFER_STATUS					0XED
#define CMD_USB_TRANSFER_FAIL						0
#define CMD_USB_TRANSFER_NEXT						1
#define CMD_USB_TRANSFER_OK							2
#define CMD_FLASH_READ_FAIL							0
#define CMD_FLASH_READ_OK								1
#define CMD_FLASH_ERASE_TIMEOUT					30000
#define CMD_FLASH_PROGRAM_READ_TIMEOUT	10000


//INA226 CMD
#define CMD_INA226_GET_DATA							0XA0


//error code
#define CMD_SEND_ERROR_CODE							0XEE
#define CMD_DRIVER_ERROR								1
#define CMD_DRIVER_ERROR_BUSY						2
#define CMD_DRIVER_ERROR_TIMEOUT				3
#define CMD_DRIVER_ERROR_UNSUPPORTED		4
#define CMD_DRIVER_ERROR_PARAMETER			5


//LDO
#define LDO_3V3  												0
#define LDO_1V8  												1
#define LDO_2V8  												2


//LED
#define LED_I2C  												0
#define LED_SPI  												1
#define LED_USART  											2
#define LED_PD2													3


//I2C
#define I2C_SPEED_100K 									0
#define I2C_SPEED_400K 									1
#define I2C_SPEED_USER_DEFINED 					2
#define DEFAULT_SLAVE_ADDRESS						0x26
#define DEFAULT_I2C_SPEED								I2C_SPEED_400K//I2C_SPEED_100K

//INA226
#define INA226_SLAVE_ADDRESS_1					0x40
#define INA226_SLAVE_ADDRESS_2					0x41
#define INA226_SLAVE_ADDRESS_3					0x44

//interface mode
#define I2C_MODE												false
#define SPI_MODE												true


//for touch
#define TOUCH_DOWN											0
#define TOUCH_MOVE											1
#define TOUCH_UP1												2
#define TOUCH_UP2												3

//for SPI cs end
#define CS_END_DELAY										100 //us
//const uint16_t GTBVersion = 0x1027;					//GTB FW version is V1.x.x.x
#define MAXCMDCOUNT					10
#define MAXCMDSIZE					15
//size define
#define MAXCOMDATASIZE 									1024		//com max reveive data one time
#define MAXUSBPACKETSIZE 								64		//USB transfer packet size
#define MAXTOUCHDATASIZE								2048	//touch data packet size,include demo + rawdata,18x36x2 + 65 + 9
#define MAXI2CSPIBUFFERSIZE							2048

//other define
#define RST_ON												GPIO_PinWrite(GPIOB,8,0)
#define RST_OFF												GPIO_PinWrite(GPIOB,8,1)
#define PA0_ON												GPIO_PinWrite(GPIOA,0,0)
#define PA0_OFF												GPIO_PinWrite(GPIOA,0,1)
#define PA1_ON												GPIO_PinWrite(GPIOA,1,0)
#define PA1_OFF												GPIO_PinWrite(GPIOA,1,1)
#define PA8_ON												GPIO_PinWrite(GPIOA,8,0)
#define PA8_OFF												GPIO_PinWrite(GPIOA,8,1)
#define PA9_ON												GPIO_PinWrite(GPIOA,9,0)
#define PA9_OFF												GPIO_PinWrite(GPIOA,9,1)
#define PA10_ON												GPIO_PinWrite(GPIOA,10,0)
#define PA10_OFF											GPIO_PinWrite(GPIOA,10,1)
#define PA15_ON												GPIO_PinWrite(GPIOA,15,0)
#define PA15_OFF											GPIO_PinWrite(GPIOA,15,1)
#define PC0_ON												GPIO_PinWrite(GPIOC,0,0)
#define PC0_OFF												GPIO_PinWrite(GPIOC,0,1)
#define PC1_ON												GPIO_PinWrite(GPIOC,1,0)
#define PC1_OFF												GPIO_PinWrite(GPIOC,1,1)
#define PC2_ON												GPIO_PinWrite(GPIOC,2,0)
#define PC2_OFF												GPIO_PinWrite(GPIOC,2,1)
#define PC3_ON												GPIO_PinWrite(GPIOC,3,0)
#define PC3_OFF												GPIO_PinWrite(GPIOC,3,1)
#define PC9_ON												GPIO_PinWrite(GPIOC,9,0)
#define PC9_OFF												GPIO_PinWrite(GPIOC,9,1)
#define PC10_ON												GPIO_PinWrite(GPIOC,10,0)
#define PC10_OFF											GPIO_PinWrite(GPIOC,10,1)
#define PC11_ON												GPIO_PinWrite(GPIOC,11,0)
#define PC11_OFF											GPIO_PinWrite(GPIOC,11,1)
#define PC13_ON												GPIO_PinWrite(GPIOC,13,0)
#define PC13_OFF											GPIO_PinWrite(GPIOC,13,1)
#define PC14_ON												GPIO_PinWrite(GPIOC,14,0)
#define PC14_OFF											GPIO_PinWrite(GPIOC,14,1)
#define PC15_ON												GPIO_PinWrite(GPIOC,15,0)
#define PC15_OFF											GPIO_PinWrite(GPIOC,15,1)
#define PB5_ON												GPIO_PinWrite(GPIOB,5,0)
#define PB5_OFF												GPIO_PinWrite(GPIOB,5,1)

#endif //AE_TOOL_MOTHER_BOARD_PROJECT_TP_DEFINE_H
