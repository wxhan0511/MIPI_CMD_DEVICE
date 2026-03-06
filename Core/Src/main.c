/**
 * @file       main.c
 * @brief      Main program body
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-12-31
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "dac.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "bsp_led.h"
#include "bsp.h"
#include "stm32f4xx_hal.h"
#include "delay.h"
#include "gtb_task.h"
#include "crc.h"
#include "fsmc.h"
#include "bsp_mcp4728_ctl.h" 
//5.0前期测试
#include "widget_main.h"




/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);

static void test_fun(void);
/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  HAL_Init();
  SystemClock_Config(); // sysclk=168M,pclk1=42M,pclk2=84M
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init(); // PB6 PB7
  MX_I2C2_Init(); // PB10 PB11
  MX_CRC_Init();
  MX_FSMC_Init();
  MX_DAC_Init();
  MX_SPI1_Init(); //***ADS1256***
  MX_SPI2_Init();//***M SPI***
  MX_SPI3_Init();//***FLASH***
  MX_USART3_UART_Init();
  bsp_init();
  // while(1){
  //     HAL_Delay(100);
  //     enableTim1CaptureCompareInterrupt();
  //     HAL_Delay(5000);
  //     disableTim1CaptureCompareInterrupt();
  // };
  // test_gtb_task();
  osKernelInitialize(); /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();
  osKernelStart();
  while (1)
  {
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON; // 8M
  RCC_OscInitStruct.HSIState = RCC_HSI_ON; // 16M
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4; // 8*PLLN/PLLM/PLLP=168(8*N/M=VCO=336)
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7; // 336(VCO)/PLLQ=48
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2; // PCLK1:AHB1 PCLK2:AHB2  HFCLK:AHB
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;                                                            // 168
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;                                                                   // 168
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;                                                                    // 42M                                                                   // 42  TIM3
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;                                                                    // 84M                                                                    // 84

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler(__FILE__, __LINE__);
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSI, RCC_MCODIV_1);
}

void Error_Handler(const char *FileName, int LineNumber)
{

  __disable_irq();
  printf("Error Handler Entered , File: %s, Line: %d!\r\n", FileName, LineNumber);
  while (1)
    ;
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */


