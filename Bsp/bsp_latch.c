#include "bsp_latch.h"
#include <stdio.h>
#include <string.h>

bsp_latch_cfg_t s_latch_cfg[BSP_LATCH_MAX_COUNT] = {
    {.cp_port = LATCH0_CP_GPIO_Port, .cp_pin = LATCH0_CP_Pin, .oe_port = OE_GPIO_Port, .oe_pin = OE_Pin}, // Latch 0
    {.cp_port = LATCH1_CP_GPIO_Port, .cp_pin = LATCH1_CP_Pin, .oe_port = OE_GPIO_Port, .oe_pin = OE_Pin}, // Latch 1
    {.cp_port = LATCH2_CP_GPIO_Port, .cp_pin = LATCH2_CP_Pin, .oe_port = OE_GPIO_Port, .oe_pin = OE_Pin}, // Latch 2
};

bsp_latch_qstatus_t g_bsp_latch_qstatus = {
    .s_latch0_qdata = {.val = 0x00},
    .s_latch1_qdata = {.val = 0x00},
    .s_latch2_qdata = {.val = 0x00},
};
/* Writes an 8-bit value to the shared data bus (D0..D7). */
static void latch_bus_write(uint8_t value)
{
    HAL_GPIO_WritePin(LATCH_D0_GPIO_Port, LATCH_D0_Pin, (value & 0x01U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LATCH_D1_GPIO_Port, LATCH_D1_Pin, (value & 0x02U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LATCH_D2_GPIO_Port, LATCH_D2_Pin, (value & 0x04U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LATCH_D3_GPIO_Port, LATCH_D3_Pin, (value & 0x08U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LATCH_D4_GPIO_Port, LATCH_D4_Pin, (value & 0x10U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LATCH_D5_GPIO_Port, LATCH_D5_Pin, (value & 0x20U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LATCH_D6_GPIO_Port, LATCH_D6_Pin, (value & 0x40U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LATCH_D7_GPIO_Port, LATCH_D7_Pin, (value & 0x80U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
/* Generates a pulse on the CP pin of the specified latch configuration. */
static void latch_pulse_cp(const bsp_latch_cfg_t *cfg)
{
    if (cfg == NULL)
        return;
    HAL_GPIO_WritePin(cfg->cp_port, cfg->cp_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(cfg->cp_port, cfg->cp_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(cfg->cp_port, cfg->cp_pin, GPIO_PIN_RESET);
}
/* Initializes the latch system: data bus and per-latch CP/OE pins. */
void bsp_latch_init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Data bus D0..D7 as push-pull outputs, default low */
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStruct.Pin = LATCH_D0_Pin;
    HAL_GPIO_Init(LATCH_D0_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = LATCH_D1_Pin;
    HAL_GPIO_Init(LATCH_D1_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = LATCH_D2_Pin;
    HAL_GPIO_Init(LATCH_D2_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = LATCH_D3_Pin;
    HAL_GPIO_Init(LATCH_D3_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = LATCH_D4_Pin;
    HAL_GPIO_Init(LATCH_D4_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = LATCH_D5_Pin;
    HAL_GPIO_Init(LATCH_D5_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = LATCH_D6_Pin;
    HAL_GPIO_Init(LATCH_D6_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = LATCH_D7_Pin;
    HAL_GPIO_Init(LATCH_D7_GPIO_Port, &GPIO_InitStruct);

    latch_bus_write(0x00U);

    /* Per-latch CP/OE pins */
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    for (size_t i = 0; i < BSP_LATCH_MAX_COUNT; ++i)
    {
        GPIO_InitStruct.Pin = s_latch_cfg[i].cp_pin;
        HAL_GPIO_Init(s_latch_cfg[i].cp_port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = s_latch_cfg[i].oe_pin;
        HAL_GPIO_Init(s_latch_cfg[i].oe_port, &GPIO_InitStruct);

        /* Default: CP low, OE# low (outputs tri-stated) */
        HAL_GPIO_WritePin(s_latch_cfg[i].cp_port, s_latch_cfg[i].cp_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(s_latch_cfg[i].oe_port, s_latch_cfg[i].oe_pin, GPIO_PIN_RESET);
    }
}

/* Sets the output enable (OE#) of the specified latch.
 * Note: OE# is active-high; setting enable=true drives outputs, false tri-states them.
 */
void bsp_latch_set_output(uint8_t id, bool enable)
{
    HAL_GPIO_WritePin(s_latch_cfg[id].oe_port, s_latch_cfg[id].oe_pin, enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* Writes an 8-bit value to the specified latch, latching it on CP rising edge. 
   @param id    Latch index (0..count-1). ff:ALL
 * @param value Data to latch.
*/
void bsp_latch_write(bsp_latch_qstatus_t* qstatus,uint8_t latch_id)
{
    if (qstatus == NULL)
    {
        return;
    }
    switch(latch_id)
    {
        case 0:
            printf("bsp_latch_write: writing to Latch 0\r\n");
            latch_bus_write(qstatus->s_latch0_qdata.val);
            latch_pulse_cp(&s_latch_cfg[0]);
            memcpy(&g_bsp_latch_qstatus.s_latch0_qdata, &qstatus->s_latch0_qdata, sizeof(latch0_t));
            break;
        case 1:
            printf("bsp_latch_write: writing to Latch 1\r\n");
            latch_bus_write(qstatus->s_latch1_qdata.val);
            latch_pulse_cp(&s_latch_cfg[1]);
            memcpy(&g_bsp_latch_qstatus.s_latch1_qdata, &qstatus->s_latch1_qdata, sizeof(latch1_t));
            break;
        case 2:
            printf("bsp_latch_write: writing to Latch 2\r\n");
            latch_bus_write(qstatus->s_latch2_qdata.val);
            latch_pulse_cp(&s_latch_cfg[2]);
            memcpy(&g_bsp_latch_qstatus.s_latch2_qdata, &qstatus->s_latch2_qdata, sizeof(latch2_t));
            break;
        case 0xFF:
            printf("bsp_latch_write: writing to ALL Latches\r\n");
            // Latch 0
            latch_bus_write(qstatus->s_latch0_qdata.val);
            latch_pulse_cp(&s_latch_cfg[0]);
            memcpy(&g_bsp_latch_qstatus.s_latch0_qdata, &qstatus->s_latch0_qdata, sizeof(latch0_t));
            // Latch 1
            latch_bus_write(qstatus->s_latch1_qdata.val);
            latch_pulse_cp(&s_latch_cfg[1]);
            memcpy(&g_bsp_latch_qstatus.s_latch1_qdata, &qstatus->s_latch1_qdata, sizeof(latch1_t));
            // Latch 2
            latch_bus_write(qstatus->s_latch2_qdata.val);
            latch_pulse_cp(&s_latch_cfg[2]);
            memcpy(&g_bsp_latch_qstatus.s_latch2_qdata, &qstatus->s_latch2_qdata, sizeof(latch2_t));
            break;
        default:
            printf("bsp_latch_write: invalid latch id %u\r\n", latch_id);
            return;
    }
    bsp_latch_set_output(latch_id, true);
}


