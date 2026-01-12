//
// Created by Bobby on 24-5-23.
//

#ifndef AE_TOOL_MOTHER_BOARD_DRV_DEFINES_H
#define AE_TOOL_MOTHER_BOARD_DRV_DEFINES_H

#include <stdint.h>

#include "bsp.h"

typedef uint8_t BSP_STATUS_T;

#define	DRV_SET_BIT(REG, BIT)     	    (REG |= (1 << BIT))
#define DRV_CLEAR_BIT(REG, BIT)         ((REG) &= ~(1 << BIT))

#endif //AE_TOOL_MOTHER_BOARD_DRV_DEFINES_H
