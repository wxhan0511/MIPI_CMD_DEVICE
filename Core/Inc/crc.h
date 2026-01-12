/**
 * @file       crc.h
 * @brief      RA_PowerEX
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-12-30
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

/*Modular, it can be used simply by including the header file
*/

#ifndef __CRC_H
#define __CRC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
extern CRC_HandleTypeDef hcrc;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
HAL_StatusTypeDef MX_CRC_Init(void);
#ifdef __cplusplus
}
#endif

#endif /* __CRC_H */
