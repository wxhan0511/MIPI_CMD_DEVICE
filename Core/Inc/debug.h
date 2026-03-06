/**
 * @file       debug.h
 * @brief      MIPI_CMD
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-10-10
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
/* Exported types ------------------------------------------------------------*/


/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

#ifdef __cplusplus
}
#endif

/* -------------------- Debug Macros -------------------- */
#define MIPI_CMD_INFO(fmt, ...)    printf(fmt, ##__VA_ARGS__)
#define MIPI_CMD_WARN(fmt, ...)    printf(fmt, ##__VA_ARGS__)
#define MIPI_CMD_ERROR(fmt, ...)   printf( fmt, ##__VA_ARGS__)
//#define MIPI_CMD_DEBUG_ENABLE 1
#if MIPI_CMD_DEBUG_ENABLE
    #define MIPI_CMD_PRINTF(fmt, ...)  printf("[MIPI_CMD] " fmt, ##__VA_ARGS__)
    #define MIPI_CMD_DEBUG(fmt, ...)   printf("[MIPI_CMD][DEBUG] " fmt, ##__VA_ARGS__)
#else
    #define MIPI_CMD_PRINTF(fmt, ...)  do {} while(0)
    #define MIPI_CMD_DEBUG(fmt, ...)   do {} while(0)
#endif
#define TIME_DEBUG_ENABLE
#ifdef TIME_DEBUG_ENABLE
    #define TIME_DEBUG(fmt, ...)         printf("[TIME DEBUG] " fmt, ##__VA_ARGS__)
    #define TIME_INFO(fmt, ...)          printf("[TIME INFO] " fmt, ##__VA_ARGS__)
#else
    #define TIME_DEBUG(fmt, ...)         do {} while(0)
    #define TIME_INFO(fmt, ...)          do {} while(0)
#endif  

#define GTB_INFO(fmt, ...)           printf(fmt, ##__VA_ARGS__)
//#define GTB_DEBUG_ENABLE
#ifdef GTB_DEBUG_ENABLE
    #define GTB_DEBUG(fmt, ...)          printf(fmt, ##__VA_ARGS__)

#else
    #define GTB_DEBUG(fmt, ...)          do {} while(0)
#endif

#define I2C_INFO(fmt, ...)          printf("[I2C INFO] " fmt, ##__VA_ARGS__)
#define I2C_ERROR(fmt, ...)          printf("[I2C ERROR] " fmt, ##__VA_ARGS__)
#define I2C_DEBUG_ENABLE
#ifdef I2C_DEBUG_ENABLE
    #define I2C_DEBUG(fmt, ...)          printf(fmt, ##__VA_ARGS__)
    
#else
    #define I2C_DEBUG(fmt, ...)          do {} while(0)
#endif

// #define AD_DATA_DEBUG_ENABLE
#ifdef AD_DATA_DEBUG_ENABLE
    #define AD_DATA_DEBUG(fmt, ...)      printf("[AD DATA DEBUG] " fmt, ##__VA_ARGS__)
    #define AD_DATA_INFO(fmt, ...)       printf("[AD DATA INFO] " fmt, ##__VA_ARGS__)
#else
    #define AD_DATA_DEBUG(fmt, ...)      do {} while(0)
    #define AD_DATA_INFO(fmt, ...)       do {} while(0)
#endif

//#define USB_DEBUG_ENABLE
#ifdef USB_DEBUG_ENABLE
    #define USB_DEBUG(fmt, ...)          printf(fmt, ##__VA_ARGS__)
    #define USB_INFO(fmt, ...)           printf("[USB INFO] " fmt, ##__VA_ARGS__)
    #define USB_ERROR(fmt, ...)          printf("[USB ERROR] " fmt, ##__VA_ARGS__)
#else
    #define USB_DEBUG(fmt, ...)          do {} while(0)
    #define USB_INFO(fmt, ...)           do {} while(0)
    #define USB_ERROR(fmt, ...)          do {} while(0)
#endif
// #define CDC_DEBUG_ENABLE
#ifdef CDC_DEBUG_ENABLE
    #define CDC_DEBUG(fmt, ...)          printf( fmt, ##__VA_ARGS__)
    #define CDC_INFO(fmt, ...)           printf("[CDC INFO] " fmt, ##__VA_ARGS__)
    #define CDC_ERROR(fmt, ...)          printf("[CDC ERROR] " fmt, ##__VA_ARGS__)
#else
    #define CDC_DEBUG(fmt, ...)          do {} while(0)
    #define CDC_INFO(fmt, ...)           do {} while(0)
    #define CDC_ERROR(fmt, ...)          do {} while(0)
#endif
#define ADS1256_INFO(fmt, ...)           printf("[ADS1256 INFO] " fmt, ##__VA_ARGS__)
#define ADS1256_ERROR(fmt, ...)          printf("[ADS1256 ERROR] " fmt, ##__VA_ARGS__)
// #define ADS1256_DEBUG_ENABLE
#ifdef ADS1256_DEBUG_ENABLE
    #define ADS1256_DEBUG(fmt, ...)          printf( fmt, ##__VA_ARGS__)
#else
    #define ADS1256_DEBUG(fmt, ...)          do {} while(0)
#endif
#define W25Q256JVEQ_INFO(fmt, ...)           printf(fmt, ##__VA_ARGS__)
#define W25Q256JVEQ_ERROR(fmt, ...)          printf(fmt, ##__VA_ARGS__)
// #define W25Q256JVEQ_DEBUG_ENABLE
#ifdef W25Q256JVEQ_DEBUG_ENABLE
    #define W25Q256JVEQ_DEBUG(fmt, ...)          printf( fmt, ##__VA_ARGS__)
#else
    #define W25Q256JVEQ_DEBUG(fmt, ...)          do {} while(0)
#endif


#endif /* __DEBUG_H */