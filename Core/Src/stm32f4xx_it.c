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
volatile uint16_t offset = 0; // Declare offset as a global variable
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

extern TIM_HandleTypeDef    htim8;
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
  while(1);
  /* USER CODE END HardFault_IRQn 0 */
  MIPI_CMD_DEBUG("HardFault_Handler\r\n");
  //先判断是MSP还是PSP，再打印 SP偏移6*4 字节处的PC值
  uint32_t *sp;
  __ASM volatile(
      "TST lr, #4 \n"
      "ITE EQ \n"
      "MRSEQ %0, MSP \n"
      "MRSNE %0, PSP \n"
      : "=r"(sp));
  MIPI_CMD_ERROR("PC = 0x%08X\r\n", sp[6]);
  if (SCB->SHCSR & SCB_SHCSR_MEMFAULTENA_Msk) {
    MIPI_CMD_ERROR("MemManage\r\n");
  }
  if (SCB->SHCSR & SCB_SHCSR_USGFAULTENA_Msk) {
    MIPI_CMD_ERROR("UsageFault\r\n");
  }
  if (SCB->SHCSR & SCB_SHCSR_BUSFAULTENA_Msk) {
    MIPI_CMD_ERROR("BusFault\r\n");
  }


  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

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
  //I2C_DEBUG("I2C2_EV_IRQHandler\r\n");
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
    if(hspi->Instance == SPI2) {
      M_INT_LOW();
      spi_rx_flag = 1;
      SPI2_Slave_OnRxCplt_IT(hspi);
      
    }
}
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2) {
        M_INT_LOW();
        SPI2_Slave_OnTxCplt_IT(hspi);
        
    }
}
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
     if(hspi == &hspi_tp){
        
         //printf("SPI2 TxRx Complete\r\n");
     }
    if(hspi->Instance == SPI3){
     }

}
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2) {
        // 处理错误
        // #define HAL_SPI_ERROR_NONE              (0x00000000U)   /*!< No error                               */
        // #define HAL_SPI_ERROR_MODF              (0x00000001U)   /*!< MODF error                             */
        // #define HAL_SPI_ERROR_CRC               (0x00000002U)   /*!< CRC error                              */
        // #define HAL_SPI_ERROR_OVR               (0x00000004U)   /*!< OVR error                              */
        // #define HAL_SPI_ERROR_FRE               (0x00000008U)   /*!< FRE error                              */
        // #define HAL_SPI_ERROR_DMA               (0x00000010U)   /*!< DMA transfer error                     */
        // #define HAL_SPI_ERROR_FLAG              (0x00000020U)   /*!< Error on RXNE/TXE/BSY Flag             */
        // #define HAL_SPI_ERROR_ABORT             (0x00000040U)   /*!< Error during SPI Abort procedure       */
        SPI2_Slave_OnError_IT(hspi);    }
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
  offset = 0;
  i2c1_rx_ready_flag = 1;
  I2C_DEBUG("I2C Listen Complete Callback\r\n");
  HAL_I2C_EnableListen_IT(hi2c);
}

//I2C device address callback function (The slave will only enter the function in response to the address sent by the host))
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
  if(TransferDirection == I2C_DIRECTION_TRANSMIT) 
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
    // 打印错误码
    I2C_DEBUG("I2C Error: Instance=0x%p, ErrorCode=0x%08lX\r\n", hi2c->Instance, hi2c->ErrorCode);

    // 根据错误类型做细致处理
    if (hi2c->ErrorCode & HAL_I2C_ERROR_AF) {
        I2C_DEBUG("I2C Error: NACK received (AF)\r\n");
        // 可做重试或通知主机
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_ARLO) {
        I2C_DEBUG("I2C Error: Arbitration lost (ARLO)\r\n");
        // 多主机场景可做仲裁恢复
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_BERR) {
        I2C_DEBUG("I2C Error: Bus error (BERR)\r\n");
        // 可做总线复位
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_OVR) {
        I2C_DEBUG("I2C Error: Overrun/Underrun (OVR)\r\n");
        // 可清理缓冲区
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_TIMEOUT) {
        I2C_DEBUG("I2C Error: Timeout\r\n");
        // 可做超时重试
    }
    // 清理自定义状态变量
    first_byte_state = 1;
    offset = 0;
#endif
      if (I2C_IsSDALow(hi2c,GPIOB,GPIO_PIN_6,GPIO_PIN_7)) {
        // 如果 SDA 被拉低，调用恢复函数
        I2C_RecoverSDA(hi2c,GPIOB,GPIO_PIN_6,GPIO_PIN_7);
      }
      if (I2C_IsSCLLow(hi2c,GPIOB,GPIO_PIN_6,GPIO_PIN_7)) {
        // 如果 SCL 被拉低，调用恢复函数
        I2C_RecoverSCL(hi2c,GPIOB,GPIO_PIN_6,GPIO_PIN_7);
      }
      MX_I2C1_Init();

    // 重新使能侦听模式，保证从机持续响应主机
      HAL_I2C_EnableListen_IT(&hi2c1);
  }
  // 处理其他I2C实例的错误
  else if (hi2c->Instance == hi2c2.Instance)
  {
#if 0
        // 打印错误码
    I2C_DEBUG("I2C Error: Instance=0x%p, ErrorCode=0x%08lX\r\n", hi2c->Instance, hi2c->ErrorCode);

    // 根据错误类型做细致处理
    if (hi2c->ErrorCode & HAL_I2C_ERROR_AF) {
        I2C_DEBUG("I2C Error: NACK received (AF)\r\n");
        // 可做重试或通知主机
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_ARLO) {
        I2C_DEBUG("I2C Error: Arbitration lost (ARLO)\r\n");
        // 多主机场景可做仲裁恢复
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_BERR) {
        I2C_DEBUG("I2C Error: Bus error (BERR)\r\n");
        // 可做总线复位
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_OVR) {
        I2C_DEBUG("I2C Error: Overrun/Underrun (OVR)\r\n");
        // 可清理缓冲区
    }
    if (hi2c->ErrorCode & HAL_I2C_ERROR_TIMEOUT) {
        I2C_DEBUG("I2C Error: Timeout\r\n");
        // 可做超时重试
    }

    // 清理自定义状态变量
    first_byte_state = 1;
    offset = 0;
#endif
    if (I2C_IsSDALow(&hi2c2,GPIOB,GPIO_PIN_10,GPIO_PIN_11)) {
      // 如果 SDA 被拉低，调用恢复函数
      I2C_RecoverSDA(&hi2c2,GPIOB,GPIO_PIN_10,GPIO_PIN_11);
    }
    if (I2C_IsSCLLow(&hi2c2,GPIOB,GPIO_PIN_10,GPIO_PIN_11)) {
      // 如果 SCL 被拉低，调用恢复函数
      I2C_RecoverSCL(&hi2c2,GPIOB,GPIO_PIN_10,GPIO_PIN_11);
    }
    MX_I2C2_Init();
    // 重新使能侦听模式，保证从机持续响应主机
    HAL_I2C_EnableListen_IT(&hi2c2);

  }
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{

}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{     

}

// I2C中断回调
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{

  printf("Master received ...\r\n");
}


void I2C1_ER_IRQHandler(void)
{
  HAL_I2C_ErrorCallback(&hi2c1);
}

void I2C2_ER_IRQHandler(void)
{
  HAL_I2C_ErrorCallback(&hi2c2);
}





void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi_tp) {
        GTB_INFO("[SPI ABORTED]\r\n");
    }
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
  //TIME_DEBUG("INT: %lu ms\r\n", dwt_get_ms());
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
// 电压电流采样回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == ADC_DRDY1_Pin)
  {
#if BSP_VOL_DEBUG
    if (cnt == 0)
    {
      printf("-----%d----\r\n", channel);

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
      printf("cur -----%d----\r\n", channel);

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
    
    if((tp_config_hid.transfer_flag == false) && (tp_config_hid.int_trans == true))
      {
          tp_config_hid.int_flag = true;
      }



#if DEBUG_LIYI == 1
    if (test_flag == 1)
    {
        bsp_DelayMS(1);
        uint8_t data[4] = {0xFF,0xA3,0x00,0x00};
        uint8_t ret_data[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        SSD_SEND_array(0X01, 2, (uint8_t*)&data);
        uint8_t ret = 0;
        ret = SSD_READ_ACK_Report_HS(0x01,0x14,1,&ret_data[0]);
        printf("ret %d \r\n",ret);
        SSD_READ_ACK_Report_HS(0x01,0x15,1,&ret_data[0]);
        SSD_READ_ACK_Report_HS(0x01,0x16,1,&ret_data[2]);
        SSD_READ_ACK_Report_HS(0x01,0x17,1,&ret_data[4]);
        SSD_READ_ACK_Report_HS(0x01,0x18,1,&ret_data[6]);
        SSD_READ_ACK_Report_HS(0x01,0x19,1,&ret_data[8]);
        SSD_READ_ACK_Report_HS(0x01,0x1a,1,&ret_data[10]);
        SSD_READ_ACK_Report_HS(0x01,0x1b,1,&ret_data[12]);
        for (uint8_t i = 0; i < 8; i++)
            printf("0x%x ",ret_data[2*i+1]);
        printf("\r\n");
        set_lcd_clock_freq(89*2, 0);
        test_flag = 0;
    }
#endif

    //printf("te signal \r\n");
   // master_state.int_change_img_flag = 1;

//    if(master_state.int_change_img_flag == 1){
//        //printf("---\r\n");
//        tp_spi_cs_enable(false);
//        bsp_DelayUS(delay_num);
//        show_next_image(&image_status);
//        master_state.int_change_img_flag = 0;
//        tp_spi_cs_enable(true);
//    }
		
		
//    if(master_state.img_run_state == IMG_INT_MODE){
//        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, exit_state);
////        tp_spi_cs_enable(exit_state);
////        exit_state = ~ exit_state;
//        show_next_image(&master_status,&image_status);
////        tp_spi_cs_enable(true);

//        exit_state = ~exit_state;
//    }else if(master_state.img_run_state == IMG_SINGLE_INT_MODE){
//        //printf("1111111111111\r\n");
//        //if(master_state.int_change_img_flag == 1)
//        {
//            show_next_image(&master_status,&image_status);
//            image_trigger_mode(0);
//            //master_state.img_run_state = IMG_TRANSFER_END;
//            printf("1111111111111\r\n");
//        }
//    }

    
  }
}

/* USER CODE END 1 */
