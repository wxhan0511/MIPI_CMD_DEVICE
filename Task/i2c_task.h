#ifndef __I2C_TASK_H
#define __I2C_TASK_H
/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"
/* Exported variables ---------------------------------------------*/
extern osThreadId_t slaveTxTaskHandle;
extern osThreadId_t slaveRxTaskHandle;

extern const osThreadAttr_t slaveTxTask_attributes;
extern const osThreadAttr_t slaveRxTask_attributes;

extern osThreadId_t masterTxTaskHandle;
extern osThreadId_t masterRxTaskHandle;

extern osMutexId_t g_i2c1Mutex;
extern osMutexId_t g_i2c2Mutex;

extern uint8_t i2c1_tx_buf[64];
extern uint8_t i2c1_rx_buf[64];
/* Exported functions prototypes ---------------------------------------------*/
void SlaveTxTask(void *argument);
void SlaveRxTask(void *argument);

void MasterTxTask(void *argument);
void MasterRxTask(void *argument);

void slave_rx_task_init();
void slave_tx_task_init();

void master_tx_task_init();
void master_rx_task_init();

void I2C1_Mutex_Init(void);
void I2C2_Mutex_Init(void);

osStatus_t I2C1_Lock(uint32_t timeout);
osStatus_t I2C1_Unlock(void);
osStatus_t I2C2_Lock(uint32_t timeout);
osStatus_t I2C2_Unlock(void);
#endif