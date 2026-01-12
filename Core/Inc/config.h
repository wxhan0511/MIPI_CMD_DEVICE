/**
 * @file       config.h
 * @brief      RA_PowerEX
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-10-10
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/


/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

#ifdef __cplusplus
}
#endif

#define SLAVE_ADDR            0x80 // Default slave address

#define SLAVE_ADDR1           0x20
#define DATA_SIZE             256
#define I2C_TIMEOUT           100

// #define I2C1_MASTER            //屏幕显示和DAC控制(master),levelshift输出(slave)
// #ifndef I2C1_MASTER
//   #define I2C1_SLAVE
// #endif

// #define I2C2_MASTER 
// #ifndef I2C2_MASTER
//   #define I2C2_SLAVE
// #endif


#define I2C1_IRQ
#ifdef I2C1_IRQ
  #define I2C_SLAVE_I2C1_LISTEN
#endif

//#define I2C2_IRQ
#ifdef I2C2_IRQ
  #define I2C_SLAVE_I2C2_LISTEN
#endif

#define GTB

//#define USE_OLED

//only one can be defined
#define CDC
// #define CHID


//#define RA_POWERSUPPLY_FOR_IC_7272
#define RA_POWERSUPPLY_FOR_IC_7275

#endif /* __CONFIG_H */
