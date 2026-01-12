//
// Created by 17333 on 25-6-26.
//

#ifndef BSP_POWER_H
#define BSP_POWER_H

#include <bsp.h>
#include "bsp_latch.h"

#define BSP_VOL_WORK 1
#define BSP_VOL_DEBUG 0
#define BSP_CUR_WORK 1
#define BSP_CUR_DEBUG 0

#define VDD_ENABLE()    do { g_bsp_latch_qstatus.s_latch2_qdata.bits.vdd_en = 1; bsp_latch_write(&g_bsp_latch_qstatus, 2); } while(0)
#define VDD_DISABLE()   do { g_bsp_latch_qstatus.s_latch2_qdata.bits.vdd_en = 0; bsp_latch_write(&g_bsp_latch_qstatus, 2); } while(0)
#define ELVSS_ENABLE()  do { g_bsp_latch_qstatus.s_latch2_qdata.bits.elvss_en = 1; bsp_latch_write(&g_bsp_latch_qstatus, 2); } while(0)
#define ELVSS_DISABLE() do { g_bsp_latch_qstatus.s_latch2_qdata.bits.elvss_en = 0; bsp_latch_write(&g_bsp_latch_qstatus, 2); } while(0)
#define ELVDD_ENABLE()  do { g_bsp_latch_qstatus.s_latch2_qdata.bits.elvdd_en = 1; bsp_latch_write(&g_bsp_latch_qstatus, 2); } while(0)
#define ELVDD_DISABLE() do { g_bsp_latch_qstatus.s_latch2_qdata.bits.elvdd_en = 0; bsp_latch_write(&g_bsp_latch_qstatus, 2); } while(0) 
#define VBAT_ENABLE()   do { g_bsp_latch_qstatus.s_latch2_qdata.bits.vbat_en = 1; bsp_latch_write(&g_bsp_latch_qstatus, 2); } while(0) 
#define VBAT_DISABLE()  do { g_bsp_latch_qstatus.s_latch2_qdata.bits.vbat_en = 0; bsp_latch_write(&g_bsp_latch_qstatus, 2); } while(0)

#define VSN_ENABLE()    do { g_bsp_latch_qstatus.s_latch1_qdata.bits.vsn_en = 1; bsp_latch_write(&g_bsp_latch_qstatus, 1); } while(0)
#define VSN_DISABLE()   do { g_bsp_latch_qstatus.s_latch1_qdata.bits.vsn_en = 0; bsp_latch_write(&g_bsp_latch_qstatus, 1); } while(0)
#define VCC_ENABLE()    do { g_bsp_latch_qstatus.s_latch1_qdata.bits.vcc_en = 1; bsp_latch_write(&g_bsp_latch_qstatus, 1); } while(0)
#define VCC_DISABLE()   do { g_bsp_latch_qstatus.s_latch1_qdata.bits.vcc_en = 0; bsp_latch_write(&g_bsp_latch_qstatus, 1); } while(0)
#define IOVCC_ENABLE()  do { g_bsp_latch_qstatus.s_latch1_qdata.bits.iovcc_en = 1; bsp_latch_write(&g_bsp_latch_qstatus, 1); } while(0)
#define IOVCC_DISABLE() do { g_bsp_latch_qstatus.s_latch1_qdata.bits.iovcc_en = 0; bsp_latch_write(&g_bsp_latch_qstatus, 1); } while(0)
#define VSP_ENABLE()    do { g_bsp_latch_qstatus.s_latch1_qdata.bits.vsp_en = 1; bsp_latch_write(&g_bsp_latch_qstatus, 1); } while(0)  
#define VSP_DISABLE()   do { g_bsp_latch_qstatus.s_latch1_qdata.bits.vsp_en = 0; bsp_latch_write(&g_bsp_latch_qstatus, 1); } while(0)
#endif //BSP_POWER_H
