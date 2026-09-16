/**
 * @file      sysmem.c
 * @brief     Minimal newlib heap support for the bootloader (sbrk).
 */

#include <errno.h>
#include <stdint.h>

static uint8_t *__sbrk_heap_end = NULL;

void *_sbrk(ptrdiff_t incr)
{
  extern uint8_t _end;            /* Symbol defined in the linker script */
  extern uint8_t _estack;         /* Symbol defined in the linker script */
  extern uint32_t _Min_Stack_Size; /* Symbol defined in the linker script */
  const uint32_t stack_limit = (uint32_t)&_estack - (uint32_t)&_Min_Stack_Size;
  const uint8_t *max_heap = (uint8_t *)stack_limit;
  uint8_t *prev_heap_end;

  if (__sbrk_heap_end == NULL)
  {
    __sbrk_heap_end = &_end;
  }

  prev_heap_end = __sbrk_heap_end;
  if ((uint32_t)__sbrk_heap_end + (uint32_t)incr > (uint32_t)max_heap)
  {
    errno = ENOMEM;
    return (void *)-1;
  }

  __sbrk_heap_end += incr;
  return (void *)prev_heap_end;
}
