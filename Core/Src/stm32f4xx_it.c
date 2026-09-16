/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32f4xx_it.c
 * @brief   Interrupt Service Routines.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_it.h"
#include "i2c.h"
#include "bsp_ads1256.h"
#include "usart.h"
#include "delay.h"
#include "bsp_power.h"
#include "i2c_utils.h"
#include "calibration_utils.h"
#include "bsp_gtb.h"
#include "i2c_task.h"
#include "tim.h"
#include "spi.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void fault_uart_write(const char *text)
{
  while (*text != '\0')
  {
    while ((USART3->SR & USART_SR_TXE) == 0U)
    {
    }
    USART3->DR = (uint8_t)*text++;
  }
}

static void fault_uart_write_hex(uint32_t value)
{
  static const char hex[] = "0123456789ABCDEF";

  for (int32_t shift = 28; shift >= 0; shift -= 4)
  {
    while ((USART3->SR & USART_SR_TXE) == 0U)
    {
    }
    USART3->DR = (uint8_t)hex[(value >> shift) & 0x0FU];
  }
}

static void fault_uart_write_register(const char *name, uint32_t value)
{
  fault_uart_write(name);
  fault_uart_write("0x");
  fault_uart_write_hex(value);
  fault_uart_write("\r\n");
}

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DAC_HandleTypeDef hdac;
extern DMA_HandleTypeDef hdma_i2c1_rx;
extern DMA_HandleTypeDef hdma_i2c1_tx;
extern DMA_HandleTypeDef hdma_i2c2_rx;
extern DMA_HandleTypeDef hdma_i2c2_tx;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern DMA_HandleTypeDef hdma_spi1_rx;
extern DMA_HandleTypeDef hdma_spi1_tx;
extern DMA_HandleTypeDef hdma_spi3_rx;
extern DMA_HandleTypeDef hdma_spi3_tx;
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern SPI_HandleTypeDef hspi3;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;
extern UART_HandleTypeDef huart3;
extern TIM_HandleTypeDef htim6;

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
extern tp_config_t tp_config_hid;

extern volatile uint8_t i2c1_rx_ready_flag;

extern TIM_HandleTypeDef htim8;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */
  __disable_irq();
  uint32_t fault_exc_return;
  __ASM volatile("mov %0, lr" : "=r"(fault_exc_return));
  uint32_t *fault_stack = (fault_exc_return & 4U) != 0U
                              ? (uint32_t *)__get_PSP()
                              : (uint32_t *)__get_MSP();

  fault_uart_write("\r\nHARDFAULT\r\n");
  fault_uart_write_register("HFSR=", SCB->HFSR);
  fault_uart_write_register("CFSR=", SCB->CFSR);
  fault_uart_write_register("BFAR=", SCB->BFAR);
  fault_uart_write_register("MMFAR=", SCB->MMFAR);
  fault_uart_write_register("EXC_RETURN=", fault_exc_return);

  uintptr_t fault_stack_address = (uintptr_t)fault_stack;
  if (((fault_stack_address >= 0x20000000U) &&
       (fault_stack_address <= 0x2001FFE0U)) ||
      ((fault_stack_address >= 0x10000000U) &&
       (fault_stack_address <= 0x1000FFE0U)))
  {
    fault_uart_write_register("LR=", fault_stack[5]);
    fault_uart_write_register("PC=", fault_stack[6]);
  }

  while (1)
  {
  }

#if 0
  // EXC_RETURN value of this exception (tells which stack was in use)
  uint32_t exc_return;
  __ASM volatile("mov %0, lr" : "=r"(exc_return));

  // Fault status registers
  uint32_t hfsr = SCB->HFSR;   // HardFault status register
  uint32_t cfsr = SCB->CFSR;   // Configurable fault status register (Mem/Bus/Usage)
  uint32_t mmfar = SCB->MMFAR; // MemManage fault address register
  uint32_t bfar = SCB->BFAR;   // BusFault address register

  // Print HardFault banner
  MIPI_CMD_DEBUG("\r\n\r\n==================== HardFault ====================\r\n");
  MIPI_CMD_ERROR("EXC_RETURN = 0x%08X\r\n", exc_return);
  MIPI_CMD_ERROR("HFSR = 0x%08X\r\n", hfsr);
  MIPI_CMD_ERROR("CFSR = 0x%08X\r\n", cfsr);

  // Split the three fault status groups
  uint8_t mmfsr = (cfsr >> 0) & 0xFF;    // MemManage fault status
  uint8_t bfsr = (cfsr >> 8) & 0xFF;     // BusFault status
  uint16_t ufsr = (cfsr >> 16) & 0xFFFF; // UsageFault status

  // ==============================================
  // MemManage fault decode
  // ==============================================
  if (mmfsr != 0)
  {
    MIPI_CMD_ERROR("[MemManage Fault]\r\n");
    if (mmfsr & (1 << 0))
      MIPI_CMD_ERROR("  IACCVIOL: Instruction access violation\r\n");
    if (mmfsr & (1 << 1))
      MIPI_CMD_ERROR("  DACCVIOL: Data access violation\r\n");
    if (mmfsr & (1 << 2))
      MIPI_CMD_ERROR("  MUNSTKERR: Unstacking error\r\n");
    if (mmfsr & (1 << 3))
      MIPI_CMD_ERROR("  MSTKERR: Stacking error\r\n");
    if (mmfsr & (1 << 4))
      MIPI_CMD_ERROR("  MLSPERR: Lazy state preservation error\r\n");
    if (mmfsr & (1 << 7))
      MIPI_CMD_ERROR("  MMARVALID: MMFAR address valid\r\n");

    // If the fault address is valid, print it
    if (mmfsr & (1 << 7))
    {
      MIPI_CMD_ERROR("  Fault Address MMFAR = 0x%08X\r\n", mmfar);
    }
  }

  // ==============================================
  // BusFault decode
  // ==============================================
  if (bfsr != 0)
  {
    MIPI_CMD_ERROR("[BusFault]\r\n");
    if (bfsr & (1 << 0))
      MIPI_CMD_ERROR("  IBUSERR: Instruction bus error(PC runaway)\r\n");
    if (bfsr & (1 << 1))
      MIPI_CMD_ERROR("  PRECISERR: Precise data bus error\r\n");
    if (bfsr & (1 << 2))
      MIPI_CMD_ERROR("  IMPRECISERR: Imprecise data bus error\r\n");
    if (bfsr & (1 << 3))
      MIPI_CMD_ERROR("  UNSTKERR: Unstacking error\r\n");
    if (bfsr & (1 << 4))
      MIPI_CMD_ERROR("  STKERR: Stacking error\r\n");
    if (bfsr & (1 << 5))
      MIPI_CMD_ERROR("  LSPERR: Lazy state preservation error\r\n");
    if (bfsr & (1 << 7))
      MIPI_CMD_ERROR("  BFARVALID: BFAR address valid\r\n");

    // If the fault address is valid, print it
    if (bfsr & (1 << 7))
    {
      MIPI_CMD_ERROR("  Fault Address BFAR = 0x%08X\r\n", bfar);
    }
  }

  // ==============================================
  // UsageFault decode
  // ==============================================
  if (ufsr != 0)
  {
    MIPI_CMD_ERROR("[UsageFault]\r\n");
    if (ufsr & (1 << 0))
      MIPI_CMD_ERROR("  UNDEFINSTR: Undefined instruction\r\n");
    if (ufsr & (1 << 1))
      MIPI_CMD_ERROR("  INVSTATE: Invalid state\r\n");
    if (ufsr & (1 << 2))
      MIPI_CMD_ERROR("  INVPC: Invalid EXC_RETURN\r\n");
    if (ufsr & (1 << 3))
      MIPI_CMD_ERROR("  NOCP: No coprocessor\r\n");
    if (ufsr & (1 << 8))
      MIPI_CMD_ERROR("  UNALIGNED: Unaligned access\r\n");
    if (ufsr & (1 << 9))
      MIPI_CMD_ERROR("  DIVBYZERO: Divide by zero\r\n");
  }

  // ==============================================
  // HardFault status flags
  // ==============================================
  if (hfsr & (1 << 1))
    MIPI_CMD_ERROR("  VECTBL: Vector table read fault\r\n");
  if (hfsr & (1 << 30))
    MIPI_CMD_ERROR("  FORCED: Forced HardFault\r\n");
  if (hfsr & (1 << 31))
    MIPI_CMD_ERROR("  DEBUGEVT: Debug event\r\n");

  // ==============================================
  // Get the current stack pointer (MSP/PSP auto-detected)
  // ==============================================
  uint32_t *sp;
  __ASM volatile(
      "TST lr, #4 \n" // Test LR bit4 to determine MSP or PSP
      "ITE EQ \n"
      "MRSEQ %0, MSP \n" // If equal, use MSP
      "MRSNE %0, PSP \n" // Otherwise use PSP
      : "=r"(sp));

  // Print the PC that triggered the HardFault (stack offset 6*4)
  MIPI_CMD_ERROR("\r\n>>>>>> Fault PC = 0x%08X <<<<<<\r\n", sp[6]);
  MIPI_CMD_ERROR(">>>>>> Fault LR = 0x%08X <<<<<<\r\n", sp[5]);
  MIPI_CMD_ERROR("=====================================================\r\n");
#endif

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
    ;
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */
  __disable_irq();
  fault_uart_write("\r\nMEMMANAGE FAULT\r\n");
  fault_uart_write_register("CFSR=", SCB->CFSR);
  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault.
 */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */
  __disable_irq();
  fault_uart_write("\r\nBUSFAULT\r\n");
  fault_uart_write_register("CFSR=", SCB->CFSR);
  fault_uart_write_register("BFAR=", SCB->BFAR);
  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */
  __disable_irq();
  fault_uart_write("\r\nUSAGEFAULT\r\n");
  fault_uart_write_register("CFSR=", SCB->CFSR);
  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/
void TIM1_UP_TIM10_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_UP_TIM10_IRQn 0 */

  /* USER CODE END TIM1_UP_TIM10_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_UP_TIM10_IRQn 1 */

  /* USER CODE END TIM1_UP_TIM10_IRQn 1 */
}
/**
 * @brief  This function handles TIM interrupt request.
 * @param  None
 * @retval None
 */
void TIM1_CC_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim1);
}

/**
 * @brief This function handles EXTI line1 interrupt.
 */
void EXTI1_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI1_IRQn 0 */

  /* USER CODE END EXTI1_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(ADC_DRDY2_Pin);
  /* USER CODE BEGIN EXTI1_IRQn 1 */

  /* USER CODE END EXTI1_IRQn 1 */
}

void EXTI15_10_IRQHandler(void)
{

  HAL_GPIO_EXTI_IRQHandler(ADC_DRDY1_Pin);
}

void EXTI9_5_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(TSPI_INT_IN_Pin);
}
/**
 * @brief This function handles DMA1 stream0 global interrupt.
 */
void DMA1_Stream0_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream0_IRQn 0 */

  /* USER CODE END DMA1_Stream0_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_i2c1_rx);
  /* USER CODE BEGIN DMA1_Stream0_IRQn 1 */

  /* USER CODE END DMA1_Stream0_IRQn 1 */
}
/* ============================================================================
 * DMA1 Stream1 global interrupt handler. Handles USART3 RX DMA interrupts.
 * ============================================================================ */
/**
 * @brief This function handles DMA1 stream1 global interrupt.
 */
void DMA1_Stream1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream1_IRQn 0 */

  /* USER CODE END DMA1_Stream1_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart3_rx);
  /* USER CODE BEGIN DMA1_Stream1_IRQn 1 */

  /* USER CODE END DMA1_Stream1_IRQn 1 */
}
/**
 * @brief This function handles DMA1 stream4 global interrupt.
 */
void DMA1_Stream4_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream4_IRQn 0 */

  /* USER CODE END DMA1_Stream4_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart3_tx);
  /* USER CODE BEGIN DMA1_Stream4_IRQn 1 */

  /* USER CODE END DMA1_Stream4_IRQn 1 */
}
/**
 * @brief This function handles DMA1 stream2 global interrupt.
 */
void DMA1_Stream2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream2_IRQn 0 */

  /* USER CODE END DMA1_Stream2_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi3_rx);
  /* USER CODE BEGIN DMA1_Stream2_IRQn 1 */

  /* USER CODE END DMA1_Stream2_IRQn 1 */
}

/**
 * @brief This function handles DMA1 stream3 global interrupt.
 */
void DMA1_Stream3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream3_IRQn 0 */

  /* USER CODE END DMA1_Stream3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_i2c2_rx);
  /* USER CODE BEGIN DMA1_Stream3_IRQn 1 */

  /* USER CODE END DMA1_Stream3_IRQn 1 */
}

/**
 * @brief This function handles DMA1 stream5 global interrupt.
 */
void DMA1_Stream5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream5_IRQn 0 */

  /* USER CODE END DMA1_Stream5_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi3_tx);
  /* USER CODE BEGIN DMA1_Stream5_IRQn 1 */

  /* USER CODE END DMA1_Stream5_IRQn 1 */
}

/**
 * @brief This function handles DMA1 stream6 global interrupt.
 */
void DMA1_Stream6_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream6_IRQn 0 */

  /* USER CODE END DMA1_Stream6_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_i2c1_tx);
  /* USER CODE BEGIN DMA1_Stream6_IRQn 1 */

  /* USER CODE END DMA1_Stream6_IRQn 1 */
}

/**
 * @brief This function handles I2C1 event interrupt.
 * I2C1 event interrupts service function (must have) actual data receiving flow
 */
void I2C1_EV_IRQHandler(void)
{
  /* USER CODE BEGIN I2C1_EV_IRQn 0 */

  /* USER CODE END I2C1_EV_IRQn 0 */
  HAL_I2C_EV_IRQHandler(&hi2c1);
  /* USER CODE BEGIN I2C1_EV_IRQn 1 */
  /* USER CODE END I2C1_EV_IRQn 1 */
}

/**
 * @brief This function handles I2C2 event interrupt.
 * I2C2 event interrupts service function (must have) actual data receiving flow
 */
void I2C2_EV_IRQHandler(void)
{
  /* USER CODE BEGIN I2C2_EV_IRQn 0 */

  /* USER CODE END I2C2_EV_IRQn 0 */
  // I2C_DEBUG("I2C2_EV_IRQHandler\r\n");
  HAL_I2C_EV_IRQHandler(&hi2c2);
  /* USER CODE BEGIN I2C2_EV_IRQn 1 */

  /* USER CODE END I2C2_EV_IRQn 1 */
}

/**
 * @brief This function handles SPI1 global interrupt.
 */
void SPI1_IRQHandler(void)
{
  /* USER CODE BEGIN SPI1_IRQn 0 */

  /* USER CODE END SPI1_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi1);
  /* USER CODE BEGIN SPI1_IRQn 1 */

  /* USER CODE END SPI1_IRQn 1 */
}

void SPI2_IRQHandler(void)
{
  /* USER CODE BEGIN SPI2_IRQn 0 */

  /* USER CODE END SPI2_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi2);
  /* USER CODE BEGIN SPI2_IRQn 1 */

  /* USER CODE END SPI2_IRQn 1 */
}
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI2)
  {
    M_INT_LOW();
    meter_com_flag = 1;
    SPI2_Slave_OnRxCplt_IT(hspi);
  }
}
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI2)
  {
    M_INT_LOW();
    SPI2_Slave_OnTxCplt_IT(hspi);
  }
}
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi == &hspi_tp)
  {

    // printf("SPI2 TxRx Complete\r\n");
  }
  if (hspi->Instance == SPI3)
  {
  }
}
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI2)
  {
    extern volatile uint32_t g_spi2_err_code;
    g_spi2_err_code = hspi->ErrorCode; // Captured for task-level diagnostics
    // Handle the error. HAL_SPI error codes for reference:
    // #define HAL_SPI_ERROR_NONE              (0x00000000U)   /*!< No error                               */
    // #define HAL_SPI_ERROR_MODF              (0x00000001U)   /*!< MODF error                             */
    // #define HAL_SPI_ERROR_CRC               (0x00000002U)   /*!< CRC error                              */
    // #define HAL_SPI_ERROR_OVR               (0x00000004U)   /*!< OVR error                              */
    // #define HAL_SPI_ERROR_FRE               (0x00000008U)   /*!< FRE error                              */
    // #define HAL_SPI_ERROR_DMA               (0x00000010U)   /*!< DMA transfer error                     */
    // #define HAL_SPI_ERROR_FLAG              (0x00000020U)   /*!< Error on RXNE/TXE/BSY Flag             */
    // #define HAL_SPI_ERROR_ABORT             (0x00000040U)   /*!< Error during SPI Abort procedure       */
    SPI2_Slave_OnError_IT(hspi);
  }
}
/**
 * @brief This function handles USART3 global interrupt.
 */
void USART3_IRQHandler(void)
{
  /* USER CODE BEGIN USART3_IRQn 0 */
  /* USER CODE END USART3_IRQn 0 */
  HAL_UART_IRQHandler(&huart3);
  /* USER CODE BEGIN USART3_IRQn 1 */

  /* USER CODE END USART3_IRQn 1 */
}

/**
 * @brief This function handles DMA1 stream7 global interrupt.
 */
void DMA1_Stream7_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream7_IRQn 0 */

  /* USER CODE END DMA1_Stream7_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_i2c2_tx);
  /* USER CODE BEGIN DMA1_Stream7_IRQn 1 */

  /* USER CODE END DMA1_Stream7_IRQn 1 */
}

/**
 * @brief This function handles SPI3 global interrupt.
 */
void SPI3_IRQHandler(void)
{
  /* USER CODE BEGIN SPI3_IRQn 0 */

  /* USER CODE END SPI3_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi3);
  /* USER CODE BEGIN SPI3_IRQn 1 */

  /* USER CODE END SPI3_IRQn 1 */
}

/**
 * @brief This function handles TIM6 global interrupt, DAC1 and DAC2 underrun error interrupts.
 */
void TIM6_DAC_IRQHandler(void)
{
  /* USER CODE BEGIN TIM6_DAC_IRQn 0 */

  /* USER CODE END TIM6_DAC_IRQn 0 */
  //  if (hdac.State != HAL_DAC_STATE_RESET) {
  //    HAL_DAC_IRQHandler(&hdac);
  //  }
  HAL_TIM_IRQHandler(&htim6);
  /* USER CODE BEGIN TIM6_DAC_IRQn 1 */

  /* USER CODE END TIM6_DAC_IRQn 1 */
}

/**
 * @brief This function handles DMA2 stream0 global interrupt.
 */
void DMA2_Stream0_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream0_IRQn 0 */

  /* USER CODE END DMA2_Stream0_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi1_rx);
  /* USER CODE BEGIN DMA2_Stream0_IRQn 1 */

  /* USER CODE END DMA2_Stream0_IRQn 1 */
}

/**
 * @brief This function handles DMA2 stream2 global interrupt.
 */
void DMA2_Stream2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream2_IRQn 0 */

  /* USER CODE END DMA2_Stream2_IRQn 0 */

  /* USER CODE BEGIN DMA2_Stream2_IRQn 1 */

  /* USER CODE END DMA2_Stream2_IRQn 1 */
}

/**
 * @brief This function handles DMA2 stream3 global interrupt.
 */
void DMA2_Stream3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream3_IRQn 0 */

  /* USER CODE END DMA2_Stream3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi1_tx);
  /* USER CODE BEGIN DMA2_Stream3_IRQn 1 */

  /* USER CODE END DMA2_Stream3_IRQn 1 */
}

/**
 * @brief This function handles DMA2 stream7 global interrupt.
 */
void DMA2_Stream7_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream7_IRQn 0 */

  /* USER CODE END DMA2_Stream7_IRQn 0 */

  /* USER CODE BEGIN DMA2_Stream7_IRQn 1 */

  /* USER CODE END DMA2_Stream7_IRQn 1 */
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
  i2c1_rx_ready_flag = 1;
  I2C_DEBUG("I2C Listen Complete Callback\r\n");
  HAL_I2C_EnableListen_IT(hi2c);
}

// I2C device address callback function (The slave will only enter the function in response to the address sent by the host))
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
  if (TransferDirection == I2C_DIRECTION_TRANSMIT)
  {
    HAL_I2C_Slave_Seq_Receive_IT(hi2c, i2c1_rx_buf, 2, I2C_LAST_FRAME);
  }
  else
  {
    HAL_I2C_Slave_Seq_Transmit_IT(hi2c, i2c1_tx_buf, 1, I2C_LAST_FRAME);
  }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{

  if (hi2c->Instance == hi2c1.Instance)
  {
#if 0
    // Print the error code
    I2C_DEBUG("I2C Error: Instance=0x%p, ErrorCode=0x%08lX\r\n", hi2c->Instance, hi2c->ErrorCode);

    // Fine-grained handling per error type
    if (hi2c->ErrorCode & HAL_I2C_ERROR_AF) {
        I2C_DEBUG("I2C Error: NACK received (AF)\r\n");
        // Retry or notify the host
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_ARLO) {
        I2C_DEBUG("I2C Error: Arbitration lost (ARLO)\r\n");
        // Arbitration recovery in multi-master scenarios
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_BERR) {
        I2C_DEBUG("I2C Error: Bus error (BERR)\r\n");
        // Bus reset
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_OVR) {
        I2C_DEBUG("I2C Error: Overrun/Underrun (OVR)\r\n");
        // Flush buffers
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_TIMEOUT) {
        I2C_DEBUG("I2C Error: Timeout\r\n");
        // Timeout retry
    }
    // Reset custom state variables
    first_byte_state = 1;
    offset = 0;
#endif
    if (I2C_IsSDALow(hi2c, GPIOB, GPIO_PIN_6, GPIO_PIN_7))
    {
      // SDA held low: run the recovery routine
      I2C_RecoverSDA(hi2c, GPIOB, GPIO_PIN_6, GPIO_PIN_7);
    }
    if (I2C_IsSCLLow(hi2c, GPIOB, GPIO_PIN_6, GPIO_PIN_7))
    {
      // SCL held low: run the recovery routine
      I2C_RecoverSCL(hi2c, GPIOB, GPIO_PIN_6, GPIO_PIN_7);
    }
    MX_I2C1_Init();

    // Re-enable listen mode so the slave keeps responding to the host
    HAL_I2C_EnableListen_IT(&hi2c1);
  }
  // Handle errors on other I2C instances
  else if (hi2c->Instance == hi2c2.Instance)
  {
#if 0
        // Print the error code
    I2C_DEBUG("I2C Error: Instance=0x%p, ErrorCode=0x%08lX\r\n", hi2c->Instance, hi2c->ErrorCode);

    // Fine-grained handling per error type
    if (hi2c->ErrorCode & HAL_I2C_ERROR_AF) {
        I2C_DEBUG("I2C Error: NACK received (AF)\r\n");
        // Retry or notify the host
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_ARLO) {
        I2C_DEBUG("I2C Error: Arbitration lost (ARLO)\r\n");
        // Arbitration recovery in multi-master scenarios
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_BERR) {
        I2C_DEBUG("I2C Error: Bus error (BERR)\r\n");
        // Bus reset
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_OVR) {
        I2C_DEBUG("I2C Error: Overrun/Underrun (OVR)\r\n");
        // Flush buffers
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_TIMEOUT) {
        I2C_DEBUG("I2C Error: Timeout\r\n");
        // Timeout retry
    }

    // Reset custom state variables
    first_byte_state = 1;
    offset = 0;
#endif
    if (I2C_IsSDALow(&hi2c2, GPIOB, GPIO_PIN_10, GPIO_PIN_11))
    {
      // SDA held low: run the recovery routine
      I2C_RecoverSDA(&hi2c2, GPIOB, GPIO_PIN_10, GPIO_PIN_11);
    }
    if (I2C_IsSCLLow(&hi2c2, GPIOB, GPIO_PIN_10, GPIO_PIN_11))
    {
      // SCL held low: run the recovery routine
      I2C_RecoverSCL(&hi2c2, GPIOB, GPIO_PIN_10, GPIO_PIN_11);
    }
    MX_I2C2_Init();
    // Re-enable listen mode so the slave keeps responding to the host
    HAL_I2C_EnableListen_IT(&hi2c2);
  }
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
}

// I2C interrupt callback
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  (void)hi2c;
}

void I2C1_ER_IRQHandler(void)
{
  HAL_I2C_ErrorCallback(&hi2c1);
}

void I2C2_ER_IRQHandler(void)
{
  HAL_I2C_ErrorCallback(&hi2c2);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  (void)xTask;
  __disable_irq();
  fault_uart_write("\r\nSTACK OVERFLOW: ");
  if (pcTaskName != NULL)
  {
    for (uint32_t i = 0; i < configMAX_TASK_NAME_LEN && pcTaskName[i] != '\0'; i++)
    {
      char character[2] = {pcTaskName[i], '\0'};
      fault_uart_write(character);
    }
  }
  fault_uart_write("\r\n");
  while (1)
  {
  }
}

void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef *hspi)
{
  (void)hspi;
}
/**
 * @brief This function handles USB On The Go HS End Point 1 Out global interrupt.
 */
void OTG_HS_EP1_OUT_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_HS_EP1_OUT_IRQn 0 */

  /* USER CODE END OTG_HS_EP1_OUT_IRQn 0 */
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
  /* USER CODE BEGIN OTG_HS_EP1_OUT_IRQn 1 */

  /* USER CODE END OTG_HS_EP1_OUT_IRQn 1 */
}

/**
 * @brief This function handles USB On The Go HS End Point 1 In global interrupt.
 */
void OTG_HS_EP1_IN_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_HS_EP1_IN_IRQn 0 */

  /* USER CODE END OTG_HS_EP1_IN_IRQn 0 */
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
  /* USER CODE BEGIN OTG_HS_EP1_IN_IRQn 1 */

  /* USER CODE END OTG_HS_EP1_IN_IRQn 1 */
}

/* USER CODE BEGIN 1 */
/**
 * @brief This function handles USB On The Go HS global interrupt.
 */
void OTG_HS_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_HS_IRQn 0 */
  // TIME_DEBUG("INT: %lu ms\r\n", dwt_get_ms());
  /* USER CODE END OTG_HS_IRQn 0 */
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);

  /* USER CODE BEGIN OTG_HS_IRQn 1 */

  /* USER CODE END OTG_HS_IRQn 1 */
}
/* USER CODE END 1 */
void EXTI2_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}
// Voltage/current sampling callback
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == ADC_DRDY1_Pin)
  {
#if BSP_VOL_DEBUG
    if (cnt == 0)
    {
      bsp_ads1256_set_single_channel(&dev_vol, channel);
      bsp_delay_us(5);
    }
    else if (cnt == 1)
    {
      bsp_ads1256_sync_wakeup(&dev_vol);
    }
    else if (cnt == 2)
    {
      bsp_ads1256_read_data(&dev_vol, &buf[channel]);
      if (channel == 5)
      {
        channel = 1;
      }
      else
      {
        channel = 5;
      }
      cnt = 0;
      return;
    }

    cnt += 1;
#elif BSP_VOL_WORK
    bsp_ads1256_irq_handle(&dev_vol);
#endif

#if BSP_CUR_DEBUG
    if (cnt == 0)
    {
      bsp_ads1256_set_single_channel(&dev_cur, channel);
      bsp_delay_us(5);
    }
    else if (cnt == 1)
    {
      bsp_ads1256_sync_wakeup(&dev_cur);
    }
    else if (cnt == 2)
    {
      bsp_ads1256_read_data(&dev_cur, &buf[channel]);
      if (channel == 2)
      {
        channel = 1;
      }
      else
      {
        channel = 2;
      }
    }
    else if (cnt == 3)
    {
      cnt = 0;
      return;
    }
    cnt += 1;
#endif
  }
  if (GPIO_Pin == TSPI_INT_IN_Pin)
  {

    if ((tp_config_hid.transfer_flag == false) && (tp_config_hid.int_trans == true))
    {
      tp_config_hid.int_flag = true;
    }

#if DEBUG_LIYI == 1
    if (test_flag == 1)
    {
      bsp_DelayMS(1);
      uint8_t data[4] = {0xFF, 0xA3, 0x00, 0x00};
      uint8_t ret_data[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      SSD_SEND_array(0X01, 2, (uint8_t *)&data);
      uint8_t ret = 0;
      ret = SSD_READ_ACK_Report_HS(0x01, 0x14, 1, &ret_data[0]);
      SSD_READ_ACK_Report_HS(0x01, 0x15, 1, &ret_data[0]);
      SSD_READ_ACK_Report_HS(0x01, 0x16, 1, &ret_data[2]);
      SSD_READ_ACK_Report_HS(0x01, 0x17, 1, &ret_data[4]);
      SSD_READ_ACK_Report_HS(0x01, 0x18, 1, &ret_data[6]);
      SSD_READ_ACK_Report_HS(0x01, 0x19, 1, &ret_data[8]);
      SSD_READ_ACK_Report_HS(0x01, 0x1a, 1, &ret_data[10]);
      SSD_READ_ACK_Report_HS(0x01, 0x1b, 1, &ret_data[12]);
      set_lcd_clock_freq(89 * 2, 0);
      test_flag = 0;
    }
#endif

    // printf("te signal \r\n");
    // master_state.int_change_img_flag = 1;
  }
}

/* USER CODE END 1 */
