//
// Created by 薛斌 on 24-8-19.
//

#ifndef RETARGET_H
#define RETARGET_H

#include "stm32f4xx_hal.h"

#include <sys/stat.h>

void bsp_retarget_init(UART_HandleTypeDef *huart);
int _isatty(int fd);
int _write(int fd, char* ptr, int len);
int _close(int fd);
int _lseek(int fd, int ptr, int dir);
int _read(int fd, char* ptr, int len);
int _fstat(int fd, struct stat* st);
void bsp_retarget_init(UART_HandleTypeDef *huart);
#endif //RETARGET_H
