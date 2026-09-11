#include "pwm_ctrl.h"
#include "main.h"
#include "task_manage.h"
#include "tim.h"
static uint32_t pulse_start_tick = 0; // Tick at which the high level started
static uint8_t pulse_valid_flag = 0;  // Prevents repeated increments within 1 second
static uint8_t test_pulse = 10;       // User variable

void pwm_ctrl_task_init(void)
{
    pwmctrlTaskHandle = osThreadNew(pwm_ctrl_task, NULL, &pwmTask_attributes);
}
/* pwm_ctrl_task function */
/**
 * @brief  Function implementing the pwm_ctrl_task.
 * @param  argument: Not used
 * @retval None
 */
void pwm_ctrl_task(void *argument)
{
    for (;;)
    {
        printf("111111111111111111\r\n");
        pulse_check_task();
        osDelay(100);
    }
}

// void pulse_check_task(void)
// {
//     // Read the IO level
//     if (HAL_GPIO_ReadPin(PULSE_A_GPIO_Port, PULSE_A_Pin) == GPIO_PIN_SET)
//     {
//         printf("Pulse A detected high\r\n");
//         // First detection of the high level, record the time
//         if (pulse_valid_flag == 0)
//         {
//             pulse_start_tick = HAL_GetTick();
//             pulse_valid_flag = 1;
//         }

//         // Check whether the high level persists for >= 1 second (1000 ms)
//         if (HAL_GetTick() - pulse_start_tick >= 1000)
//         {
//             printf("Pulse A valid for >1s\r\n");
//             // Held for 1 second, execute a single +10 step
//             disableTim1PWMOutput();
//             disableTim2PWMOutput();
//             bsp_led_pwm_init(test_pulse);
//             bsp_blasi_pwm_init(test_pulse);
//             enableTim1PWMOutput();
//             enableTim2PWMOutput();

//             test_pulse += 10; // Increment by 10 only after a full 1 second
//             // printf("Pulse detected >1s, Duty Cycle set to: %d %%\n", test_pulse / 1050);
//             if (test_pulse > 100)
//             {
//                 test_pulse = 10;
//                 disableTim1PWMOutput();
//                 disableTim2PWMOutput();
//                 printf("Before disableTim1PWMOutput: htim1.Instance=0x%08lX\n", (unsigned long)htim1.Instance);
//                 printf("Before disableTim2PWMOutput: htim2.Instance=0x%08lX\n", (unsigned long)htim2.Instance);
//                 printf("Duty Cycle reset to 0%%\n");
//             }
//             pulse_valid_flag = 0; // Reset, ready for the next cycle
//         }
//     }
//     else
//     {
//         // Cleared immediately on release, no trigger
//         pulse_valid_flag = 0;
//     }
// }
