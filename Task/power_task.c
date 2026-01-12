/**
 * @file       power_task.c
 * @brief      RA_PowerEX
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-11-05
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

/* Includes ------------------------------------------------------------------*/
#include "power_task.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "bsp.h"
#include "bsp_ads1256.h"
#include "bsp_mcp4728.h"
#include "bsp_power.h"
#include "drv_ra_device.h"
#include "bsp_calibration.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
osThreadId_t powertaskhandle = NULL;

const osThreadAttr_t powertask_attributes = {
    .name = "PowerTask",
    .priority = osPriorityHigh7,
    .stack_size = 128 * 4};
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
void power_task_init()
{
    powertaskhandle = osThreadNew(PowerTask, NULL, &powertask_attributes);
    if (powertaskhandle == NULL)
        RA_POWEREX_ERROR("powertaskhandle create failed\r\n");
}

void PowerTask(void *argument)
{
    BSP_STATUS ADJ_ELVDD_status;
    BSP_STATUS status;
    uint8_t falling_edge_detected = 1;
    uint8_t temp = 0;
    for (;;)
    {
        // if (HAL_GPIO_ReadPin(ELVDD_EN_GPIO_Port, ELVDD_EN_Pin) == GPIO_PIN_RESET)
        // {
        //     if (HAL_GPIO_ReadPin(TP_RESET_GPIO_Port, TP_RESET_Pin) == GPIO_PIN_RESET)
        //     {

        //         if (falling_edge_detected >= 2)
        //         {
        //             ADJ_ELVDD_status = bsp_dac_single_voltage_set(&dac_dev, 0, dac_dev.val[0], 0);
        //             if (ADJ_ELVDD_status != BSP_OK)
        //             {
        //                 RA_POWEREX_DEBUG("bsp_dac_single_voltage_set channel 0 ELVDD failed\r\n");
        //             }
        //             osDelay(10);
        //             RA_POWEREX_DEBUG("bsp_dac_single_voltage_set channel 0 ELVDD: %d\r\n", dac_dev.val[0]);
        //             ELVDD_ENABLE(); // 重新使能ELVDD
        //             HAL_GPIO_WritePin(LDAC_Port, LDAC_Pin, GPIO_PIN_RESET);
        //             RA_POWEREX_DEBUG("ELVDD output enabled\r\n");
        //         }
        //         falling_edge_detected = 2;
        //         HAL_Delay(10000); // 消抖延时
        //     }
        // }
        // else
        // {
        //     if(HAL_GPIO_ReadPin(ELVSS_EN_GPIO_Port, ELVSS_EN_Pin) == GPIO_PIN_SET)
        //     {
        //         temp = float_to_uint8_round(g_calibration_manager.data.vsn_last_voltage / 100);
        //         status = ra_dev_main_0.ops->set_power_vol(ra_dev_main_0.dev,RA_POWER_VSN, temp);
        //         if (status != BSP_OK)
        //         {
        //             printf("VSN set power failed\r\n");
        //         }

        //         status = ra_dev_main_0.ops->set_power_en(ra_dev_main_0.dev, RA_POWER_VSN, 1);
        //         if (status != BSP_OK)
        //         {
        //             RA_POWEREX_DEBUG("[drv ra ops] main 0x%x vsn power off\r\n", ra_dev_main_0.main_address);
        //         }
        //         HAL_Delay(40);
        //         ELVSS_ENABLE();
        //         RA_POWEREX_DEBUG("ELVDD_EN pin high detected, VSP and ELVSS enabled\r\n");
        //     }
        // }

        osDelay(200);
    }
}