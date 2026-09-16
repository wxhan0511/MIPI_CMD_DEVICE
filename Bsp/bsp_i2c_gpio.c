/*
	Application notes:
	Before accessing an I2C device, call i2c_CheckDevice() first to check whether the device is present; that function configures the GPIOs
*/

#include "bsp.h"
#include "bsp_i2c_gpio.h"

/* GPIO ports connected to the I2C bus; modify only these 4 lines to change the SCL/SDA pins */
#define I2C_SCL_GPIO	GPIOB			/* GPIO connected to the SCL clock line */
#define I2C_SDA_GPIO	GPIOB			/* GPIO connected to the SDA data line */

#define I2C_SCL_PIN		GPIO_PIN_10			/* GPIO connected to the SCL clock line */
#define I2C_SDA_PIN		GPIO_PIN_11			/* GPIO connected to the SDA data line */

#define ALL_I2C_GPIO_CLK_ENABLE()	__HAL_RCC_GPIOB_CLK_ENABLE()

/* Macros that drive and read SCL/SDA */
#define I2C_SCL_1()  I2C_SCL_GPIO->BSRR = I2C_SCL_PIN				/* SCL = 1 */
#define I2C_SCL_0()  I2C_SCL_GPIO->BSRR = ((uint32_t)I2C_SCL_PIN << 16U)				/* SCL = 0 */

#define I2C_SDA_1()  I2C_SDA_GPIO->BSRR = I2C_SDA_PIN				/* SDA = 1 */
#define I2C_SDA_0()  I2C_SDA_GPIO->BSRR = ((uint32_t)I2C_SDA_PIN << 16U)				/* SDA = 0 */


#define I2C_SDA_READ()  ((I2C_SDA_GPIO->IDR & I2C_SDA_PIN) != 0)	/* Read the SDA pin state */
#define I2C_SCL_READ()  ((I2C_SCL_GPIO->IDR & I2C_SCL_PIN) != 0)	/* Read the SCL pin state */

#define I2C_2_SCL_GPIO	GPIOB			/* GPIO connected to the SCL clock line */
#define I2C_2_SDA_GPIO	GPIOB			/* GPIO connected to the SDA data line */

#define I2C_2_SCL_PIN		GPIO_PIN_6			/* GPIO connected to the SCL clock line */
#define I2C_2_SDA_PIN		GPIO_PIN_7		/* GPIO connected to the SDA data line */

#define ALL_I2C_2_GPIO_CLK_ENABLE()	__HAL_RCC_GPIOB_CLK_ENABLE()

/* Macros that drive and read SCL/SDA */
#define I2C_2_SCL_1()  I2C_SCL_GPIO->BSRR = I2C_2_SCL_PIN				/* SCL = 1 */
#define I2C_2_SCL_0()  I2C_SCL_GPIO->BSRR = ((uint32_t)I2C_2_SCL_PIN << 16U)				/* SCL = 0 */

#define I2C_2_SDA_1()  I2C_SDA_GPIO->BSRR = I2C_2_SDA_PIN				/* SDA = 1 */
#define I2C_2_SDA_0()  I2C_SDA_GPIO->BSRR = ((uint32_t)I2C_2_SDA_PIN << 16U)				/* SDA = 0 */


#define I2C_2_SDA_READ()  ((I2C_2_SCL_GPIO->IDR & I2C_2_SDA_PIN) != 0)	/* Read the SDA pin state */
#define I2C_2_SCL_READ()  ((I2C_2_SCL_GPIO->IDR & I2C_2_SCL_PIN) != 0)	/* Read the SCL pin state */

/*
*********************************************************************************************************
*	Function: bsp_InitI2C
*	Description: Configure the I2C bus GPIOs, implemented by bit-banging the IOs
*	Parameter:  None
*	Return: None
*********************************************************************************************************
*/
void bsp_InitI2C(void)
{
    GPIO_InitTypeDef gpio_init;

    /* Step 1: Initialize the GPIO clock */
    ALL_I2C_GPIO_CLK_ENABLE();

    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;	/* Open-drain output */
    gpio_init.Pull = GPIO_NOPULL;			/* No pull-up (external pull-up required) */
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;	// GPIO_SPEED_FREQ_HIGH;  /* GPIO speed level */

    gpio_init.Pin = I2C_SCL_PIN;
    HAL_GPIO_Init(I2C_SCL_GPIO, &gpio_init);

    gpio_init.Pin = I2C_SDA_PIN;
    HAL_GPIO_Init(I2C_SDA_GPIO, &gpio_init);

    /* Send a stop signal to reset all devices on the I2C bus to standby mode */
    i2c_Stop();

    /* Step 1: Initialize the GPIO clock */

}

void bsp_InitI2C_2(void){
}

/*
*********************************************************************************************************
*	Function: i2c_Delay
*	Description: I2C bus bit delay, about 400 KHz
*	Parameter:  None
*	Return: None
*********************************************************************************************************
*/
static void i2c_Delay(void)
{
    /*
        At a CPU clock of 168 MHz executing from internal Flash, MDK project without optimization, waveform observed with an oscilloscope.
        With a loop count of 5, SCL frequency = 1.78 MHz (measured 92 ms per operation; read/write failed as soon as the probe was attached. Timing critical.)
        With a loop count of 10, SCL frequency = 1.1 MHz (measured 138 ms, read speed: 118724 B/s)
        With a loop count of 30, SCL frequency = 440 KHz; SCL high time 1.0 us, SCL low time 1.2 us

        With a 2.2K ohm pull-up the SCL rise time is about 0.5 us; with a 4.7K ohm pull-up it is about 1 us

        In practice, about 400 KHz is a suitable choice.
    */
    //for (i = 0; i < 30; i++);
    //for (i = 0; i < 60; i++);
    //bsp_DelayUS(2); 229.57KHz timing
    bsp_delay_us(2);
}

/*
*********************************************************************************************************
*	Function: i2c_Start
*	Description: CPU generates the I2C bus start signal
*	Parameter:  None
*	Return: None
*********************************************************************************************************
*/
void i2c_Start(void)
{
    /* When SCL is high, a falling edge on SDA indicates the I2C bus start signal */
    I2C_SDA_1();
    I2C_SCL_1();
    i2c_Delay();
    I2C_SDA_0();
    i2c_Delay();

    I2C_SCL_0();
    i2c_Delay();
}

void i2c_2_Start(void)
{
    /* When SCL is high, a falling edge on SDA indicates the I2C bus start signal */
    I2C_2_SDA_1();
    I2C_2_SCL_1();
    i2c_Delay();
    I2C_2_SDA_0();
    i2c_Delay();

    I2C_2_SCL_0();
    i2c_Delay();
}

/*
*********************************************************************************************************
*	Function: i2c_Stop
*	Description: CPU generates the I2C bus stop signal
*	Parameter:  None
*	Return: None
*********************************************************************************************************
*/
void i2c_Stop(void)
{
    /* When SCL is high, a rising edge on SDA indicates the I2C bus stop signal */
    I2C_SDA_0();
    i2c_Delay();
    I2C_SCL_1();
    i2c_Delay();
    I2C_SDA_1();
    i2c_Delay();
}

void i2c_2_Stop(void)
{
    /* When SCL is high, a rising edge on SDA indicates the I2C bus stop signal */
    I2C_2_SDA_0();
    i2c_Delay();
    I2C_2_SCL_1();
    i2c_Delay();
    I2C_2_SDA_1();
    i2c_Delay();
}

/*
*********************************************************************************************************
*	Function: i2c_SendByte
*	Description: CPU sends 8-bit data to the I2C bus device
*	Parameter:  _ucByte Byte to be transmitted
*	Return: None
*********************************************************************************************************
*/
void i2c_SendByte(uint8_t _ucByte)
{
    uint8_t i;

    /* Send the MSB (bit7) of the byte first */
    for (i = 0; i < 8; i++)
    {
        if (_ucByte & 0x80)
        {
            I2C_SDA_1();
        }
        else
        {
            I2C_SDA_0();
        }
        i2c_Delay();
        I2C_SCL_1();
        i2c_Delay();
        I2C_SCL_0();
        I2C_SCL_0();	/* 2019-03-14 Extra line added for GT811 data transfer issues, equivalent to a few tens of ns extra delay */
        if (i == 7)
        {
            I2C_SDA_1(); // Release the bus
        }
        _ucByte <<= 1;	/* Shift one bit to the left */
    }
}
void i2c_2_SendByte(uint8_t _ucByte)
{
    uint8_t i;

    /* Send the MSB (bit7) of the byte first */
    for (i = 0; i < 8; i++)
    {
        if (_ucByte & 0x80)
        {
            I2C_2_SDA_1();
        }
        else
        {
            I2C_2_SDA_0();
        }
        i2c_Delay();
        I2C_2_SCL_1();
        i2c_Delay();
        I2C_2_SCL_0();
        I2C_2_SCL_0();	/* 2019-03-14 Extra line added for GT811 data transfer issues, equivalent to a few tens of ns extra delay */
        if (i == 7)
        {
            I2C_2_SDA_1(); // Release the bus
        }
        _ucByte <<= 1;	/* Shift one bit to the left */
    }
}
/*
*********************************************************************************************************
*	Function: i2c_ReadByte
*	Description: CPU reads 8-bit data from the I2C bus device
*	Parameter:  None
*	Return: The data read
*********************************************************************************************************
*/
uint8_t i2c_ReadByte(void)
{
    uint8_t i;
    uint8_t value;

    /* Read out bit7 of the data as the first bit */
    value = 0;
    for (i = 0; i < 8; i++)
    {
        value <<= 1;
        I2C_SCL_1();
        i2c_Delay();
        if (I2C_SDA_READ())
        {
            value++;
        }
        I2C_SCL_0();
        i2c_Delay();
    }
    return value;
}

uint8_t i2c_2_ReadByte(void)
{
    uint8_t i;
    uint8_t value;

    /* Read out bit7 of the data as the first bit */
    value = 0;
    for (i = 0; i < 8; i++)
    {
        value <<= 1;
        I2C_2_SCL_1();
        i2c_Delay();
        if (I2C_2_SDA_READ())
        {
            value++;
        }
        I2C_2_SCL_0();
        i2c_Delay();
    }
    return value;
}
/*
*********************************************************************************************************
*	Function: i2c_WaitAck
*	Description: CPU generates one clock and reads the device ACK response
*	Parameter:  None
*	Return: Returns 0 for a valid ACK, 1 for no device response
*********************************************************************************************************
*/
uint8_t i2c_WaitAck(void)
{
    uint8_t re;

    I2C_SDA_1();	/* CPU releases the SDA bus */
    i2c_Delay();
    I2C_SCL_1();	/* CPU drives SCL = 1; the device will return the ACK response */
    i2c_Delay();
    if (I2C_SDA_READ())	/* CPU reads the SDA pin state */
    {
        re = 1;
    }
    else
    {
        re = 0;
    }
    I2C_SCL_0();
    i2c_Delay();
    return re;

}

uint8_t i2c_2_WaitAck(void)
{
    uint8_t re;

    I2C_2_SDA_1();	/* CPU releases the SDA bus */
    i2c_Delay();
    I2C_2_SCL_1();	/* CPU drives SCL = 1; the device will return the ACK response */
    i2c_Delay();
    if (I2C_2_SDA_READ())	/* CPU reads the SDA pin state */
    {
        re = 1;
    }
    else
    {
        re = 0;
    }
    I2C_2_SCL_0();
    i2c_Delay();
    return re;

}

/*
*********************************************************************************************************
*	Function: i2c_Ack
*	Description: CPU generates an ACK signal
*	Parameter:  None
*	Return: None
*********************************************************************************************************
*/
void i2c_Ack(void)
{
    I2C_SDA_0();	/* CPU drives SDA = 0 */
    i2c_Delay();
    I2C_SCL_1();	/* CPU generates 1 clock */
    i2c_Delay();
    I2C_SCL_0();
    i2c_Delay();
    I2C_SDA_1();	/* CPU releases the SDA bus */

    i2c_Delay();
}
void i2c_2_Ack(void)
{
    I2C_2_SDA_0();	/* CPU drives SDA = 0 */
    i2c_Delay();
    I2C_2_SCL_1();	/* CPU generates 1 clock */
    i2c_Delay();
    I2C_2_SCL_0();
    i2c_Delay();
    I2C_2_SDA_1();	/* CPU releases the SDA bus */

    i2c_Delay();
}
/*
*********************************************************************************************************
*	Function: i2c_NAck
*	Description: CPU generates 1 NACK signal
*	Parameter:  None
*	Return: None
*********************************************************************************************************
*/
void i2c_NAck(void)
{
    I2C_SDA_1();	/* CPU drives SDA = 1 */
    i2c_Delay();
    I2C_SCL_1();	/* CPU generates 1 clock */
    i2c_Delay();
    I2C_SCL_0();
    i2c_Delay();
}
void i2c_2_NAck(void)
{
    I2C_2_SDA_1();	/* CPU drives SDA = 1 */
    i2c_Delay();
    I2C_2_SCL_1();	/* CPU generates 1 clock */
    i2c_Delay();
    I2C_2_SCL_0();
    i2c_Delay();
}
/*
*********************************************************************************************************
*	Function: i2c_CheckDevice
*	Description: Detect the device on the I2C bus: the CPU sends the device address and reads the device response to check whether it is present
*	Parameter:  _Address The I2C bus address of the device
*	Return: Returns 0 for success, 1 for not detected
*********************************************************************************************************
*/
uint8_t i2c_CheckDevice(uint8_t _Address)
{
    uint8_t ucAck;

    if (I2C_SDA_READ() && I2C_SCL_READ())
    {
        i2c_Start();		/* Send the start signal */

        /* The 7-bit device address + read/write bit (0 = w, 1 = r); bit7 is sent first */
        i2c_SendByte(_Address | I2C_WR);
        ucAck = i2c_WaitAck();	/* Wait for the device ACK response */

        i2c_Stop();			/* Send the stop signal */

        return ucAck;
    }
    return 1;	/* I2C bus error */
}


