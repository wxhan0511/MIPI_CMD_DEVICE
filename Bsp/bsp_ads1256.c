//
// Created by xuebin on 24-8-16.
//

#include "bsp_ads1256.h"
#include "bsp_ads1256_ctl.h"
#include "bsp.h"
#include "bsp_dwt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>

#include "main.h"

ads1256_dev_t dev_vol = {
    .cs_group = ADC_SPI_CS1_GPIO_Port,
    .cs_pin = ADC_SPI_CS1_Pin,
    .drdy_group = ADC_DRDY1_GPIO_Port,
    .drdy_pin = ADC_DRDY1_Pin,
    .cs_control = HAL_GPIO_WritePin,
    .read_reg = bsp_ads1256_read_reg,
    .write_reg = bsp_ads1256_write_reg,
    .read_byte = bsp_ads1256_read_byte,
    .write_byte = bsp_ads1256_write_byte,
    .channel_en = 0,
    .work_mode = NORMAL_MODE,
    .work_channel = 0,
    .sample_res_gear = {0, 0, 0, 0, 0, 0, 0, 0},
    .step_cnt = 1,
    .sample_cnt = 0,
    .r_en = 0,
    .res_cali_en = 1,
};

ads1256_dev_t dev_cur = {
    .cs_group = ADC_SPI_CS1_GPIO_Port,
    .cs_pin = ADC_SPI_CS1_Pin,
    .drdy_group = ADC_DRDY1_GPIO_Port,
    .drdy_pin = ADC_DRDY1_Pin,
    .cs_control = HAL_GPIO_WritePin,
    .read_reg = bsp_ads1256_read_reg,
    .write_reg = bsp_ads1256_write_reg,
    .read_byte = bsp_ads1256_read_byte,
    .write_byte = bsp_ads1256_write_byte,
    .channel_en = 0,
    .work_mode = NORMAL_MODE,
    .work_channel = 0,
    .sample_res_gear = {0, 0, 0, 0, 0, 0, 0, 0},
    .step_cnt = 1,
    .sample_cnt = 0,
};

#define ADC_DRDY() HAL_GPIO_ReadPin(handle->drdy_group, handle->drdy_pin)
/**
 * @brief Wait for the ADS1256 data-ready (DRDY) signal to go low
 * @param handle ADS1256 device handle
 */
static void bsp_ads1256_wait_drdy(const ads1256_dev_t *handle)
{
    if (handle->work_mode == NORMAL_MODE)
    {
        uint32_t i = 40000000;
        while (ADC_DRDY())
        {
            if (i-- == 0)
            {
                printf("Wait DRDY timeout\r\n");
                break;
            }
        }
    }
}
/**
 * @brief Write one byte of data to the ADS1256 via SPI
 * @param handle ADS1256 device handle
 * @param data Data to write
 * @retval BSP_OK/BSP_ERROR
 */
BSP_STATUS bsp_ads1256_write_byte(const ads1256_dev_t *handle, uint8_t data)
{
    const HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, &data, 1, 1000);
    if (status != HAL_OK)
    {
        return BSP_ERROR;
    }
    return BSP_OK;
}
/**
 * @brief Read one byte of data from the ADS1256 via SPI
 * @param handle ADS1256 device handle
 * @param data Pointer to the read data
 * @retval BSP_OK/BSP_ERROR
 */
BSP_STATUS bsp_ads1256_read_byte(const ads1256_dev_t *handle, uint8_t *data)
{
    const HAL_StatusTypeDef status = HAL_SPI_Receive(&hspi1, data, 1, 1000);
    if (status != HAL_OK)
    {
        return BSP_ERROR;
    }
    return BSP_OK;
}
/**
 * @brief Read the content of ADS1256 registers
 * @param handle ADS1256 device handle
 * @param first_cmd Start register address
 * @param read_data Pointer to the read data
 * @param reg_num Number of registers to read
 * @retval BSP_OK/BSP_ERROR
 */
BSP_STATUS bsp_ads1256_read_reg(const ads1256_dev_t *handle, const uint8_t first_cmd, uint8_t *read_data,
                                const uint8_t reg_num)
{
    uint8_t send_data[2];
    send_data[0] = CMD_RREG | (first_cmd & 0x0f);
    send_data[1] = reg_num - 1;

    handle->cs_control(handle->cs_group, handle->cs_pin, 0);

    const HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, send_data, 2, 1000);

    bsp_delay_us(5);

    HAL_SPI_Receive(&hspi1, read_data, reg_num, 1000);

    handle->cs_control(handle->cs_group, handle->cs_pin, 1);

    if (status != HAL_OK)
    {
        printf("spi1 error \r\n");
        return BSP_ERROR;
    }
    return BSP_OK;
}
/**
 * @brief Write ADS1256 registers
 * @param handle ADS1256 device handle
 * @param first_cmd Start register address
 * @param write_data Pointer to the data to write
 * @param reg_num Number of registers to write
 * @retval BSP_OK/BSP_ERROR
 */
BSP_STATUS bsp_ads1256_write_reg(const ads1256_dev_t *handle, const uint8_t first_cmd, const uint8_t *write_data,
                                 const uint8_t reg_num)
{
    uint8_t send_data[reg_num + 2];
    send_data[0] = CMD_WREG | first_cmd & 0x0f;
    send_data[1] = reg_num;
    memcpy(&send_data[2], write_data, reg_num);

    handle->cs_control(handle->cs_group, handle->cs_pin, 0);

    const HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, send_data, 2 + reg_num, 1000);

    handle->cs_control(handle->cs_group, handle->cs_pin, 1);

    if (status != HAL_OK)
    {
        return BSP_ERROR;
    }
    return BSP_OK;
}
/**
 * @brief Reset and initialize the ADS1256 chip, performing self-calibration and register configuration
 * @param handle ADS1256 device handle
 */
void bsp_ads1256_init(const ads1256_dev_t *handle)
{
    uint8_t data[5];
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = ADC_DRDY1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ADC_DRDY1_GPIO_Port, &GPIO_InitStruct);

    // ADC_Reset PA0
    HAL_GPIO_WritePin(ADC_RESET_GPIO_Port, ADC_RESET_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ADC_RESET_GPIO_Port, ADC_RESET_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ADC_RESET_GPIO_Port, ADC_RESET_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(handle->cs_group, handle->cs_pin, GPIO_PIN_SET);

    bsp_delay_ms(100);

    bsp_ads1256_wait_drdy(handle);
    //  read ads1256 reg status
    handle->read_reg(handle, REG_STATUS, &data[0], 5);
    ADS1256_DEBUG("ADS1256 REG_STATUS:0x%02X\r\n", data[0]);
    ADS1256_DEBUG("ADS1256 REG_MUX:0x%02X\r\n", data[1]);
    ADS1256_DEBUG("ADS1256 REG_ADCON:0x%02X\r\n", data[2]);
    ADS1256_DEBUG("ADS1256 REG_DRATE:0x%02X\r\n", data[3]);
    ADS1256_DEBUG("ADS1256 REG_IO:0x%02X\r\n", data[4]);
    handle->cs_control(handle->cs_group, handle->cs_pin, 0);

    handle->write_byte(handle, CMD_SELFCAL);

    handle->cs_control(handle->cs_group, handle->cs_pin, 1);

    bsp_delay_ms(100);

    // wait ready
    bsp_ads1256_wait_drdy(handle);

    data[0] = 0x04;
    data[1] = 0x08;
    data[2] = 0x00;
    data[3] = 0x82; // 100 SPS
    data[4] = 0x00;
    handle->write_reg(handle, 0x00, data, 5);
    handle->read_reg(handle, REG_STATUS, &data[0], 5);
    bsp_delay_ms(100);
}
/**
 * @brief Read the 24-bit conversion result of the ADS1256 and perform sign extension
 * @param handle ADS1256 device handle
 * @param val Pointer to the conversion result
 * @retval BSP_OK/BSP_ERROR
 */
BSP_STATUS bsp_ads1256_read_data(const ads1256_dev_t *handle, int32_t *val)
{
    uint8_t read_data[3] = {0, 0, 0};
    uint32_t convert_data = 0;
    if (handle == NULL)
    {
        return BSP_ERROR;
    }

    handle->cs_control(handle->cs_group, handle->cs_pin, 0);

    BSP_STATUS status = handle->write_byte(handle, CMD_RDATA);

    bsp_delay_us(7);

    status |= handle->read_byte(handle, &read_data[2]);
    status |= handle->read_byte(handle, &read_data[1]);
    status |= handle->read_byte(handle, &read_data[0]);

    handle->cs_control(handle->cs_group, handle->cs_pin, 1);

    convert_data = (read_data[2] << 16) | (read_data[1] << 8) | read_data[0];
    //
    if (convert_data & 0x800000)
    {
        convert_data += 0xFF000000;
    }

    *val = (int32_t)convert_data;
    return status;
}
/**
 * @brief Configure the ADS1256 gain and sample rate (not implemented)
 * @param gain Gain
 * @param sps Sample rate
 */
void bsp_ads1256_config(ADS1256_GAIN gain, ADS1256_SPS sps)
{
    // wait ready
    //  bsp_ads1256_wait_drdy(handle);
}
/**
 * @brief Read the ADS1256 chip ID
 * @param handle ADS1256 device handle
 * @param id Pointer to the read ID
 * @retval BSP_OK/BSP_ERROR
 */
BSP_STATUS bsp_ads1256_read_id(const ads1256_dev_t *handle, uint8_t *id)
{
    uint8_t reg_val = 0;

    // wait ready
    bsp_ads1256_wait_drdy(handle);

    const BSP_STATUS ret = handle->read_reg(handle, REG_STATUS, &reg_val, 1);
    *id = reg_val;
    return ret;
}

/**
 * @brief Set the ADS1256 to single-ended sampling mode and select the sampling channel
 * @param handle ADS1256 device handle
 * @param channel Channel number 0-7
 * @retval BSP_OK/BSP_ERROR
 */
BSP_STATUS bsp_ads1256_set_single_channel(const ads1256_dev_t *handle, const uint8_t channel)
{
    if (channel > 7)
        return BSP_ERROR;

    const uint8_t data = channel << 4 | 0x08;
    const BSP_STATUS status = bsp_ads1256_write_reg(handle, REG_MUX, &data, 1);
    return status;
}
/**
 * @brief Enable the ADS1256 interrupt (not implemented)
 * @param handle ADS1256 device handle
 */
void bsp_ads1256_irq_enable(const ads1256_dev_t *handle)
{
}

/**
 * @brief Set the channel enable bits for sampling
 * @param handle ADS1256 device handle
 * @param channel_en Channel enable bits
 */
void bsp_ads1256_set_sample_channel(ads1256_dev_t *handle, const uint8_t channel_en)
{
    handle->channel_en = channel_en;
}

/**
 * @brief Get the enable bits of the currently sampled channels
 * @param handle ADS1256 device handle
 * @retval Channel enable bits
 */
uint8_t bsp_ads1256_get_sample_channel(const ads1256_dev_t *handle)
{
    return handle->channel_en;
}

BSP_STATUS bsp_ads1256_start_scan(ads1256_dev_t *handle)
{
    if (handle == NULL || handle->channel_en == 0U)
        return BSP_ERROR;

    uint8_t first_channel = handle->work_channel;
    if (first_channel >= 8U || (handle->channel_en & (1U << first_channel)) == 0U)
    {
        first_channel = 0U;
        while ((handle->channel_en & (1U << first_channel)) == 0U)
            first_channel++;
    }

    if (bsp_ads1256_set_single_channel(handle, first_channel) != BSP_OK)
        return BSP_ERROR;

    handle->work_channel = first_channel;
    handle->last_channel = first_channel;
    handle->step_cnt = 0U;
    bsp_ads1256_sync_wakeup(handle);
    __HAL_GPIO_EXTI_CLEAR_IT(handle->drdy_pin);
    return BSP_OK;
}
/**
 * @brief Read the completed channel and immediately start the next conversion
 * @param handle ADS1256 device handle
 */
void bsp_ads1256_irq_handle(ads1256_dev_t *handle)
{
    if (handle == NULL || handle->channel_en == 0U)
        return;

    const uint8_t current_channel = handle->work_channel;
    if (current_channel >= 8U)
        return;

    uint8_t sample_index = handle->sample_cnt[current_channel];
    if (sample_index >= AVG_CNT)
        sample_index = 0U;

    if (bsp_ads1256_read_data(handle, &handle->data_buffer[current_channel][sample_index]) == BSP_OK)
    {
        sample_index++;
        handle->sample_cnt[current_channel] = sample_index;
        if (sample_index >= AVG_CNT)
        {
            double sum = 0.0;
            for (uint8_t i = 0; i < AVG_CNT; i++)
                sum += handle->data_buffer[current_channel][i];

            handle->data_buffer_avg[current_channel] = sum / AVG_CNT;
            handle->sample_cnt[current_channel] = 0U;
            const double raw_data = handle->data_buffer_avg[current_channel] * ADC_RATIO * 0.000001;

            /* A zero averaged code is an invalid conversion frame during
             * ADS1256 mux settling. Do not publish it as a real rail value;
             * otherwise VCC can be overwritten with 0 mV intermittently. */
            if (raw_data != 0.0)
                raw_data_queue_push(raw_data, current_channel);
        }
    }

    uint8_t next_channel = current_channel;
    do
    {
        next_channel = (next_channel + 1U) & 0x07U;
    } while ((handle->channel_en & (1U << next_channel)) == 0U);

    handle->last_channel = current_channel;
    if (bsp_ads1256_set_single_channel(handle, next_channel) == BSP_OK)
    {
        handle->work_channel = next_channel;
        bsp_ads1256_sync_wakeup(handle);
    }
}

/**
 * @brief Send the SYNC and WAKEUP commands to the ADS1256
 * @param handle ADS1256 device handle
 */
void bsp_ads1256_sync_wakeup(const ads1256_dev_t *handle)
{
    handle->write_byte(handle, CMD_SYNC);
    bsp_delay_us(5);

    handle->write_byte(handle, CMD_WAKEUP);
    bsp_delay_us(25);
}
