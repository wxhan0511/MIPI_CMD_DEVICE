/**
 * @file       crc.c
 * @brief      MIPI_CMD
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-12-30
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

/* Includes ------------------------------------------------------------------*/
#include "crc.h"
#include "stdio.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc = {0};
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/**
 * @brief CRC外设初始化
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef MX_CRC_Init(void)
{
    __HAL_RCC_CRC_CLK_ENABLE();
    hcrc.Instance = CRC;
    
    if (HAL_CRC_Init(&hcrc) != HAL_OK)
    {
        printf("CRC init fail\r\n");
        return HAL_ERROR;
    }

    return HAL_OK;
}
