/**
 * @brief
 */

#ifndef __PWM_CTRL_H
#define __PWM_CTRL_H

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void pwm_ctrl_task(void *argument);
void pulse_check_task(void);
void pwm_ctrl_task_init(void);

#endif /* __PWM_CTRL_H */
