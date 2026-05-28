#include "pwm_ctrl.h"
#include "main.h"
#include "task_manage.h"
#include "tim.h"
static uint32_t pulse_start_tick = 0; // 记录开始高电平时间
static uint8_t pulse_valid_flag = 0;  // 防止1秒内重复加
uint8_t test_pulse = 10;              // 你的变量

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

void pulse_check_task(void)
{
    // 读取IO电平
    if (HAL_GPIO_ReadPin(PULSE_A_GPIO_Port, PULSE_A_Pin) == GPIO_PIN_SET)
    {
        printf("Pulse A detected high\r\n");
        // 第一次检测到高电平，记录时间
        if (pulse_valid_flag == 0)
        {
            pulse_start_tick = HAL_GetTick();
            pulse_valid_flag = 1;
        }

        // 判断持续高电平 ≥1秒 (1000ms)
        if (HAL_GetTick() - pulse_start_tick >= 1000)
        {
            printf("Pulse A valid for >1s\r\n");
            // 满足1秒，执行一次 +10
            disableTim1PWMOutput();
            disableTim2PWMOutput();
            bsp_led_pwm_init(test_pulse);
            bsp_blasi_pwm_init(test_pulse);
            enableTim1PWMOutput();
            enableTim2PWMOutput();

            test_pulse += 10; // 真正满足1秒才+10
            // printf("Pulse detected >1s, Duty Cycle set to: %d %%\n", test_pulse / 1050);
            if (test_pulse > 100)
            {
                test_pulse = 10;
                disableTim1PWMOutput();
                disableTim2PWMOutput();
                printf("Before disableTim1PWMOutput: htim1.Instance=0x%08lX\n", (unsigned long)htim1.Instance);
                printf("Before disableTim2PWMOutput: htim2.Instance=0x%08lX\n", (unsigned long)htim2.Instance);
                printf("Duty Cycle reset to 0%%\n");
            }
            pulse_valid_flag = 0; // 重置，准备下一次
        }
    }
    else
    {
        // 一旦松开，立即清零，不触发
        pulse_valid_flag = 0;
    }
}
