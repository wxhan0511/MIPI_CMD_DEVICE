//
// Created by Xue Bin on 24-8-19.
//

#include "retarget.h"

/*
 * retarget.c
 *
 *  Created on: 2023-06-09
 *      Author: Bobby
 */


#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/times.h>
#include <limits.h>
#include <signal.h>
#include "retarget.h"
#include <stdint.h>
#include <stdio.h>
#include "cmsis_os.h"
#include "FreeRTOS.h"

#if !defined(OS_USE_SEMIHOSTING)

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

UART_HandleTypeDef *gHuart;
static osMutexId_t retarget_mutex;
static StaticSemaphore_t retarget_mutex_control_block;
static const osMutexAttr_t retarget_mutex_attributes = {
    .name = "retarget_mutex",
    .attr_bits = osMutexRecursive | osMutexPrioInherit,
    .cb_mem = &retarget_mutex_control_block,
    .cb_size = sizeof(retarget_mutex_control_block),
};

static int retarget_can_lock(void)
{
    return retarget_mutex != NULL &&
           osKernelGetState() == osKernelRunning &&
           __get_IPSR() == 0U;
}

static void retarget_lock(void)
{
    if (retarget_can_lock())
        (void)osMutexAcquire(retarget_mutex, osWaitForever);
}

static void retarget_unlock(void)
{
    if (retarget_can_lock())
        (void)osMutexRelease(retarget_mutex);
}

// This function initializes the retargeting of the standard I/O streams
// to the specified UART handle. It disables I/O buffering for the STDOUT stream
void bsp_retarget_init(UART_HandleTypeDef *huart) {
    gHuart = huart;
    setvbuf(stdout, NULL, _IONBF, 0);
}

void bsp_retarget_rtos_init(void)
{
    retarget_mutex = osMutexNew(&retarget_mutex_attributes);
}

void __wrap___retarget_lock_acquire_recursive(_LOCK_RECURSIVE_T lock)
{
    (void)lock;
    retarget_lock();
}

void __wrap___retarget_lock_release_recursive(_LOCK_RECURSIVE_T lock)
{
    (void)lock;
    retarget_unlock();
}

int _isatty(int fd) {
    if (fd >= STDIN_FILENO && fd <= STDERR_FILENO)
        return 1;

    errno = EBADF;
    return 0;
}

int _write(int fd, char* ptr, int len) {
    HAL_StatusTypeDef hstatus;

    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        if (gHuart == NULL) {
            errno = ENODEV;
            return -1;
        }
        retarget_lock();
        hstatus = HAL_UART_Transmit(gHuart, (uint8_t *) ptr, len, HAL_MAX_DELAY);
        retarget_unlock();
        if (hstatus == HAL_OK) {
            return len;
        }
        errno = EIO;
        return -1;
    }
    errno = EBADF;
    return -1;
}

int _close(int fd) {
    if (fd >= STDIN_FILENO && fd <= STDERR_FILENO)
        return 0;

    errno = EBADF;
    return -1;
}

int _lseek(int fd, int ptr, int dir) {
    (void) fd;
    (void) ptr;
    (void) dir;

    errno = EBADF;
    return -1;
}

int _read(int fd, char* ptr, int len) {
    HAL_StatusTypeDef hstatus;

    if (fd == STDIN_FILENO) {
        if (gHuart == NULL) {
            errno = ENODEV;
            return -1;
        }
        hstatus = HAL_UART_Receive(gHuart, (uint8_t *) ptr, 1, HAL_MAX_DELAY);
        if (hstatus == HAL_OK)
            return 1;
        else
            return EIO;
    }
    errno = EBADF;
    return -1;
}

int _fstat(int fd, struct stat* st) {
    if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) {
        st->st_mode = S_IFCHR;
        return 0;
    }

    errno = EBADF;
    return 0;
}

#endif //#if !defined(OS_USE_SEMIHOSTING)
