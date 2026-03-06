
#ifndef _BSP_I2C_GPIO_H
#define _BSP_I2C_GPIO_H

#define I2C_WR	0		/* Ð´¿ØÖÆbit */
#define I2C_RD	1		/* ¶Á¿ØÖÆbit */

void bsp_InitI2C(void);
void i2c_Start(void);
void i2c_Stop(void);
void i2c_SendByte(uint8_t _ucByte);
uint8_t i2c_ReadByte(void);
uint8_t i2c_WaitAck(void);
void i2c_Ack(void);
void i2c_NAck(void);
uint8_t i2c_CheckDevice(uint8_t _Address);

void bsp_InitI2C_2(void);
void i2c_2_Start(void);
void i2c_2_Stop(void);
void i2c_2_SendByte(uint8_t _ucByte);
uint8_t i2c_2_ReadByte(void);
uint8_t i2c_2_WaitAck(void);
void i2c_2_Ack(void);
void i2c_2_NAck(void);



#endif
