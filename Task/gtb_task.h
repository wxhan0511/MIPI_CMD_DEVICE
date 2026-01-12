/**
 * @file       gtb.h
 * @brief      GTB module header.
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-10-09
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

#ifndef __GTB_H
#define __GTB_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os2.h"
#include "usbd_def.h"
#include <stdbool.h>
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/* 用户自定义变量声明 */
extern osThreadId_t thread_id_gtb;
extern const osThreadAttr_t server_gtb_attr;
extern uint8_t get_data_fs[64];
extern uint8_t send_data_fs[64];  
extern bool hid_state_fs;
#ifdef __cplusplus
}
#endif
/* Exported functions prototypes ---------------------------------------------*/
void server_gtb(void *argument);
void server_gtb_suspend();
void server_gtb_init();
void test_gtb_task(void);
#endif /* __GTB_H */
