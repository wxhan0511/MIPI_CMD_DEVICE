//
// Created by 17333 on 25-6-27.
//

#ifndef COM_HANDLE_H
#define COM_HANDLE_H

#include <task_manage.h>
#include <bsp.h>
typedef enum
{
    METER_OK = 0,
    METER_ERROR = 1,
    METER_TIMEOUT = 2,
} METER_STATE;

void com_handle_i2c(const uint8_t *rx, uint8_t *tx);
void com_handle_i2c_init(void);
#endif //COM_HANDLE_H
