//
// Created by xuebin on 24-8-16.
//

#include "bsp_dwt.h"

#define  DEM_CR_TRCENA               (1 << 24)
#define  DWT_CR_CYCCNTENA            (1 <<  0)

void bsp_init_dwt(void)
{
    DEM_CR         |= (unsigned int)DEM_CR_TRCENA;
    DWT_CYCCNT      = (unsigned int)0u;
    DWT_CR         |= (unsigned int)DWT_CR_CYCCNTENA;
}


/*
*********************************************************************************************************
*	Function: bsp_delay_ms
*	Description: To make the low-level drivers compatible with both RTOS and bare-metal environments,
*             a blocking delay function is provided. In low-level drivers, the ms delay is mainly
*             used during initialization and does not affect real-time behavior.
*	Parameter: n Delay length, in 1 ms units
*	Return: None
*********************************************************************************************************
*/
void bsp_delay_ms(uint32_t time)
{
	bsp_delay_us(1000*time);
}

/*
*********************************************************************************************************
*	Function: bsp_delay_us
*	Description: The delay is implemented with the CPU's internal counter, a 32-bit counter.
*             	OSSchedLock(&err);
*				bsp_delay_us(5);
*				OSSchedUnlock(&err); Depending on the situation, decide whether a scheduler lock or disabled interrupts are needed
*	Parameter: time  Delay length, in 1 us units
*	Return: None
*   Note: 1. At a 168 MHz core clock, the 32-bit counter overflows after 2^32/168000000 = 25.565 s.
*                It is recommended to keep delays below 1 second when using this function.
*             2. Measured with an oscilloscope, the us delay function runs about 0.25 us longer than the requested time.
*             Test conditions for the data below:
*             (1) MDK5.15, optimization level 0 (the MDK optimization level has no effect on it).
*             (2) STM32F407IGT6
*             (3) Test method:
*				 GPIOI->BSRRL = GPIO_Pin_8;
*				 bsp_delay_us(10);
*				 GPIOI->BSRRH = GPIO_Pin_8;
*             -------------------------------------------
*                Requested               Actual
*             bsp_delay_us(1)          1.2360us
*             bsp_delay_us(2)          2.256us
*             bsp_delay_us(3)          3.256us
*             bsp_delay_us(4)          4.256us
*             bsp_delay_us(5)          5.276us
*             bsp_delay_us(6)          6.276us
*             bsp_delay_us(7)          7.276us
*             bsp_delay_us(8)          8.276us
*             bsp_delay_us(9)          9.276us
*             bsp_delay_us(10)         10.28us
*            3. Subtracting two unsigned 32-bit numbers and assigning the result to an unsigned 32-bit
*              number still yields the correct difference.
*              Suppose A, B, C are all unsigned 32-bit numbers.
*              If A > B, then A - B = C, which is straightforward and has no problems.
*              If A < B, then A - B = C, where C = 0xFFFFFFFF - B + A + 1. Pay special attention to this; it is exactly what this function relies on.
*********************************************************************************************************
*/
void bsp_delay_us(uint32_t time)
{
    uint32_t tCnt, tDelayCnt;
	uint32_t tStart;

	tStart = DWT_CYCCNT;                                     /* Counter value on entry */
	tCnt = 0;
	tDelayCnt = time * (SystemCoreClock / 1000000);	 /* Number of ticks required */

	while(tCnt < tDelayCnt)
	{
		tCnt = DWT_CYCCNT - tStart; /* Subtraction is still correct even if the 32-bit counter wraps around */
	}
}
uint32_t dwt_get_ms(void)
{
	return (DWT_CYCCNT / (SystemCoreClock / 1000000))/ 1000;
}
void bsp_delay_dwt(uint32_t time)
{
    uint32_t tCnt, tDelayCnt;
	uint32_t tStart;

	tCnt = 0;
	tDelayCnt = time;	 /* Number of ticks required */
	tStart = DWT_CYCCNT;         /* Counter value on entry */

	while(tCnt < tDelayCnt)
	{
		tCnt = DWT_CYCCNT - tStart; /* Subtraction is still correct even if the 32-bit counter wraps around */
	}
}