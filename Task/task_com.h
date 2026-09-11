/* ==================== 1. Header guard ==================== */
#ifndef _TASK_COM_H_
#define _TASK_COM_H_

/* ==================== 2. Includes ==================== */
#include <stdint.h>

/* ==================== 3. Macros ==================== */
/* None */

/* ==================== 4. Type definitions ==================== */
/* None */

/* ==================== 5. External global variable declarations ==================== */
extern volatile uint8_t meter_com_flag;

/* ==================== 6. External function declarations ==================== */
void task_com_init(void);
void task_com_suspend(void);
void task_com_resume(void);

/* ==================== 7. End of header guard ==================== */
#endif /* _TASK_COM_H_ */
