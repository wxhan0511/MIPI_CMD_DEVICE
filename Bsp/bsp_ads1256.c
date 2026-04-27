//
// Created by 薛斌 on 24-8-16.
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

uint8_t first_loop_flag[8] = {0};


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
    .r_en=0,
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
 * @brief 等待ADS1256数据就绪（DRDY）信号为低电平
 * @param handle ADS1256设备句柄
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
 * @brief 通过SPI向ADS1256写入一个字节数据
 * @param handle ADS1256设备句柄
 * @param data 待写入的数据
 * @retval BSP_OK/BSP_ERROR
 */
BSP_STATUS bsp_ads1256_write_byte(const ads1256_dev_t *handle, uint8_t data)
{
    // dev->ops->cs_control(dev->cs_group,dev->cs_pin,0);
    const HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, &data, 1, 1000);
    // dev->ops->cs_control(dev->cs_group,dev->cs_pin,1);
    if (status != HAL_OK)
    {
        return BSP_ERROR;
    }
    return BSP_OK;
}
/**
 * @brief 通过SPI从ADS1256读取一个字节数据
 * @param handle ADS1256设备句柄
 * @param data 读取到的数据指针
 * @retval BSP_OK/BSP_ERROR
 */
BSP_STATUS bsp_ads1256_read_byte(const ads1256_dev_t *handle, uint8_t *data)
{
    // dev->ops->cs_control(dev->cs_group,dev->cs_pin,0);

    const HAL_StatusTypeDef status = HAL_SPI_Receive(&hspi1, data, 1, 1000);
    // dev->ops->cs_control(dev->cs_group,dev->cs_pin,1);
    if (status != HAL_OK)
    {
        return BSP_ERROR;
    }
    return BSP_OK;
}
/**
 * @brief 读取ADS1256的寄存器内容
 * @param handle ADS1256设备句柄
 * @param first_cmd 起始寄存器地址
 * @param read_data 读取到的数据指针
 * @param reg_num 读取寄存器数量
 * @retval BSP_OK/BSP_ERROR
 */
BSP_STATUS bsp_ads1256_read_reg(const ads1256_dev_t *handle, const uint8_t first_cmd, uint8_t *read_data,
                                const uint8_t reg_num)
{
    uint8_t send_data[2];
    send_data[0] = CMD_RREG | (first_cmd & 0x0f);
    send_data[1] = reg_num - 1;

    // const ads1256_dev_t *dev = (ads1256_dev_t *)handle;
    handle->cs_control(handle->cs_group, handle->cs_pin, 0);

    const HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, send_data, 2, 1000);

    bsp_delay_us(5);

    HAL_SPI_Receive(&hspi1, read_data, reg_num, 1000);
    // HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi1, send_data,read_data, 3, 1000);

    handle->cs_control(handle->cs_group, handle->cs_pin, 1);

    if (status != HAL_OK)
    {
        printf("spi1 error \r\n");
        return BSP_ERROR;
    }
    return BSP_OK;
}
/**
 * @param handle ADS1256设备句柄
 * @param first_cmd 起始寄存器地址
 * @param write_data 待写入的数据指针
 * @param reg_num 写入寄存器数量
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
 * @brief 复位并初始化ADS1256芯片，进行自校准和寄存器配置
 * @param handle ADS1256设备句柄
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
    // read ads1256 reg status
    handle->read_reg(handle, REG_STATUS, &data[0], 5);
    // ADS1256_DEBUG("ADS1256 REG_STATUS:0x%02X\r\n", data[0]);
    // ADS1256_DEBUG("ADS1256 REG_MUX:0x%02X\r\n", data[1]);
    // ADS1256_DEBUG("ADS1256 REG_ADCON:0x%02X\r\n", data[2]);
    // ADS1256_DEBUG("ADS1256 REG_DRATE:0x%02X\r\n", data[3]);
    // ADS1256_DEBUG("ADS1256 REG_IO:0x%02X\r\n", data[4]);
    handle->cs_control(handle->cs_group, handle->cs_pin, 0);

    handle->write_byte(handle, CMD_SELFCAL);

    handle->cs_control(handle->cs_group, handle->cs_pin, 1);

    bsp_delay_ms(100);

    // wait ready
    bsp_ads1256_wait_drdy(handle);

    data[0] = 0x04;
    data[1] = 0x08;
    data[2] = 0x00;
    data[3] = 0x63; // 50sps
    // data[3] = 0x82;//500sps
    data[4] = 0x00;
    handle->write_reg(handle, 0x00, data, 5);
    handle->read_reg(handle, REG_STATUS, &data[0], 5);
    // ADS1256_DEBUG("ADS1256 REG_STATUS:0x%02X\r\n", data[0]);
    // ADS1256_DEBUG("ADS1256 REG_MUX:0x%02X\r\n", data[1]);
    // ADS1256_DEBUG("ADS1256 REG_ADCON:0x%02X\r\n", data[2]);
    // ADS1256_DEBUG("ADS1256 REG_DRATE:0x%02X\r\n", data[3]);
    // ADS1256_DEBUG("ADS1256 REG_IO:0x%02X\r\n", data[4]);
    bsp_delay_ms(100);
}
/**
 * @brief 读取ADS1256的转换数据（24位），并进行符号扩展
 * @param handle ADS1256设备句柄
 * @param val 读取到的转换结果指针
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

    // //wait ready
    // bsp_ads1256_wait_drdy(handle);

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
    // printf("raw data %f \r\n", *val*0.596/1000);
    // printf("vol %f ,cur %f\r\n",*val*0.596,*val*0.596-3766);//3766

    // printf("read data converted %d Vol %f ,Cur %f,0x%x 0x%x 0x%x\n",  convert_data,*val*0.596/1000,(*val*0.596/1000-1344)/10,read_data[0],read_data[1],read_data[2]);
    return status;
}
/**
 * @brief 配置ADS1256的增益和采样速率（未实现）
 * @param gain 增益
 * @param sps 采样速率
 */
void bsp_ads1256_config(ADS1256_GAIN gain, ADS1256_SPS sps)
{
    // wait ready
    //  bsp_ads1256_wait_drdy(handle);
}
/**
 * @brief 读取ADS1256的芯片ID
 * @param handle ADS1256设备句柄
 * @param id 读取到的ID指针
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
 * @brief 设置ADS1256为单端采样模式，并选择采样通道
 * @param handle ADS1256设备句柄
 * @param channel 通道号 0-7
 * @retval BSP_OK/BSP_ERROR
 */
BSP_STATUS bsp_ads1256_set_single_channel(const ads1256_dev_t *handle, const uint8_t channel)
{
    if (channel > 7)
        return BSP_ERROR;

    // //wait ready
    // bsp_ads1256_wait_drdy(handle);

    const uint8_t data = channel << 4 | 0x08;
    const BSP_STATUS status = bsp_ads1256_write_reg(handle, REG_MUX, &data, 1);
    return status;
}
/**
 * @brief 使能ADS1256中断（未实现）
 * @param handle ADS1256设备句柄
 */
void bsp_ads1256_irq_enable(const ads1256_dev_t *handle)
{
}

/**
 * @brief 设置需要采样的通道使能位
 * @param handle ADS1256设备句柄
 * @param channel_en 通道使能位
 */
void bsp_ads1256_set_sample_channel(ads1256_dev_t *handle, const uint8_t channel_en)
{
    handle->channel_en = channel_en;
}

/**
 * @brief 获取当前采样通道的使能位
 * @param handle ADS1256设备句柄
 * @retval 通道使能位
 */
uint8_t bsp_ads1256_get_sample_channel(const ads1256_dev_t *handle)
{
    return handle->channel_en;
}
/**
 * @brief ADS1256采样流程状态机处理函数，依次完成通道选择、同步、数据读取、通道切换、数据平均等步骤
 * @param handle ADS1256设备句柄
 */
void bsp_ads1256_irq_handle(ads1256_dev_t* handle)
{
    
    if (handle->step_cnt == 0)
    {
        //1、通道选择
        bsp_ads1256_set_single_channel(handle, handle->work_channel);
        bsp_delay_us(5);
    }
    else if (handle->step_cnt == 1)
    {
        //2、同步唤醒
        bsp_ads1256_sync_wakeup(handle);
    }
    else if (handle->step_cnt == 2)
    {
        //3、读取数据

        bsp_ads1256_read_data(
            handle, &handle->data_buffer[handle->work_channel][handle->sample_cnt[handle->work_channel]]);
        //printf("%d %d: %d \r\n", handle->work_channel,handle->sample_cnt,handle->data_buffer[handle->work_channel][handle->sample_cnt]);
    }
    else if (handle->step_cnt == 3)
    {
        //4、选择下一个通道
        handle->last_channel = handle->work_channel;
        handle->work_channel += 1;
        while (1)
        {
            //q:解释下一条语句
            //a: 如果当前通道使能位为1且不是第8个通道，则继续循环
            //   否则，跳出循环，准备进行下一步操作
            if ((handle->channel_en >> handle->work_channel & 0x01) == 1 && handle->work_channel != 8)
            {
                break;
            }
            handle->work_channel += 1;
            if (handle->work_channel == 8)
            {
                handle->work_channel = 0;
            }
        }
    }
    else if (handle->step_cnt == 4)
    {
        //5、结束采样
        //设置采样平均数
        handle->sample_cnt[handle->last_channel] += 1;
        if (handle->sample_cnt[handle->last_channel] == AVG_CNT)
        {
            double sum = 0;
            for (uint8_t i = 0; i < AVG_CNT; i++)
            {
                sum += handle->data_buffer[handle->last_channel][i];
                //printf("%d \r\n",handle->data_buffer[handle->last_channel][i]);
            }

            handle->data_buffer_avg[handle->last_channel] = sum / AVG_CNT;
            handle->sample_cnt[handle->last_channel] = 0;
            //printf("channel %d ,vol %f \r\n",handle->last_channel,handle->data_buffer_avg[handle->last_channel]);
        }

    }
    else if(handle->step_cnt == 5)
    {
        // if(handle->vol_en == 1)
        {   
            const double raw_data = handle->data_buffer_avg[handle->last_channel]*ADC_RATIO*0.000001;
            //printf("channel %d raw data %f \r\n",handle->last_channel,raw_data);
            if(raw_data != 0.0)
            {
                raw_data_queue_push(raw_data , handle->last_channel);   //push data and index(corresponding channel) to ring queue
            }
            //AD_DATA_DEBUG("channel %d raw data %f \r\n",handle->last_channel,raw_data);

            //const double compare = bsp_adc_vol_convert_64pin(handle->vol_gear,raw_data,handle->single_vol_cali_en);
            //handle->data_buffer_avg[handle->last_channel] = compare;
            //printf("vol raw data %d gear %d ,%f %.3f \r\n",handle->last_channel,handle->vol_gear,raw_data,compare);
        }
    }
    else if(handle->step_cnt == 6)
    {
        handle->step_cnt = 0;
        return;
    }
    handle->step_cnt += 1;
}



/**
 * @brief 向ADS1256发送同步（SYNC）和唤醒（WAKEUP）命令
 * @param handle ADS1256设备句柄
 */
void bsp_ads1256_sync_wakeup(const ads1256_dev_t *handle)
{
    handle->write_byte(handle, CMD_SYNC);
    bsp_delay_us(5);

    handle->write_byte(handle, CMD_WAKEUP);
    bsp_delay_us(25);
}