/**
 * @file       drv_ra_device.c
 * @brief      MIPI_CMD
 * @author     wxhan
 * @version    1.0.0
 * @date       2025-10-30
 * @copyright  Copyright (c) 2025 gcoreinc
 * @license    MIT License
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "drv_ra_device.h"
#include <stdio.h>
#include <string.h>
#include "drv_defines.h"
#include "bsp_i2c_bus.h"
#include "bsp_multi_channel_sel.h"
#include "debug.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static BSP_STATUS_T drv_ra_i2c_read_reg(uint8_t address,uint8_t command,uint8_t *data);
static BSP_STATUS_T drv_ra_i2c_read_data_int16(uint8_t address,uint8_t command,int16_t *data);
static BSP_STATUS_T drv_ra_i2c_write_reg(uint8_t address,uint8_t command,uint8_t data);
static BSP_STATUS_T drv_ra_i2c_write_data_word(uint8_t address,uint8_t command,uint16_t data);
static BSP_STATUS_T drv_ra_mux_sel(uint8_t address,uint8_t channel,uint8_t en);

/* Private functions -------------------------------------------------------*/

BSP_STATUS_T drv_ra_mux_sel(uint8_t address,uint8_t channel,uint8_t en){
    return bsp_multi_channel_sel(&bsp_i2c_hw_ra,address,channel,en);
}

BSP_STATUS_T drv_ra_i2c_read_reg(uint8_t address,uint8_t command,uint8_t *data){
    return bsp_i2c_hw_ra.read_data(bsp_i2c_hw_ra.handle,address,command,data,1);
}

BSP_STATUS_T drv_ra_i2c_read_data_int16(uint8_t address,uint8_t command,int16_t *data){
    BSP_STATUS_T status;
    uint8_t read_data[3];
    status = bsp_i2c_hw_ra.read_data(bsp_i2c_hw_ra.handle,address,command,read_data,2);
    *data = (int16_t )((read_data[0]<<8)|read_data[1]);
    return status;
}

BSP_STATUS_T drv_ra_i2c_write_reg(uint8_t address,uint8_t command,uint8_t data){
    return bsp_i2c_hw_ra.write_data(bsp_i2c_hw_ra.handle,address,command,&data,1);
}

BSP_STATUS_T drv_ra_i2c_write_data_word(uint8_t address,uint8_t command,uint16_t data){
    return bsp_i2c_hw_ra.write_data(bsp_i2c_hw_ra.handle, address, command, (const uint8_t *) &data, 2);
}

BSP_STATUS_T drv_ra_set_power_vol(drv_ra_dev_t *dev,uint8_t power_name,uint8_t power_value){
    uint8_t status;
    uint8_t temp_data;
    switch (power_name) {
        case RA_POWER_VSP:
            if((power_value < 24) || (power_value > 84))
            {
                MIPI_CMD_INFO("The vsp voltage is %d *100*mv and is not set within the range of 24 to 84, error\r\n",power_value);
                return BSP_ERROR;
            }
            else{
                if(power_value >= 40)
                    temp_data = power_value - 40;
                else
                    temp_data = power_value - 24 + 32;
                status = dev->sgm3804->write(RA_SGM3804_ADDRESS,RA_SGM3804_VSP_CMD,temp_data);
            }
            //printf("    SET POWER VSP %d status %d\r\n",power_value,status);
            break;
        case RA_POWER_VSN:
            if((power_value < 24) || (power_value > 64))
            {
                MIPI_CMD_INFO("The vsn voltage is %d *100*mv and is not set within the range of 24 to 64, error\r\n",power_value);
                return BSP_ERROR;
            }
            else{
                if(power_value >= 40)
                    temp_data = power_value - 40;
                else
                    temp_data = power_value - 24 + 32;
                status = dev->sgm3804->write(RA_SGM3804_ADDRESS,RA_SGM3804_VSN_CMD,temp_data);
            }
            //printf("    SET POWER VSN %d status %d\r\n",power_value,status);
            break;
        case RA_POWER_VCI:
            if((power_value < 10) || (power_value > 35))
            {
                MIPI_CMD_INFO("The vci voltage is %d *100*mv and is not set within the range of 10 to 35, error\r\n",power_value);

                return BSP_ERROR;
            }
            else{
                temp_data = power_value - 10;
                status = dev->lp3907->write(RA_LP3907_1_ADDRESS,0x10,0x75);
                status = status | dev->lp3907->write(RA_LP3907_1_ADDRESS,RA_LP3907_1_VCI_CMD,temp_data);
            }
            //printf("    SET POWER VCI %d status %d\r\n",power_value,status);
            break;
        case RA_POWER_IOVCC:
            if((power_value < 10) || (power_value > 35))
            {
                MIPI_CMD_INFO("The iovcc_xb voltage is %d *100*mv and is not set within the range of 10 to 35, error\r\n",power_value);
                return BSP_ERROR;
            }
            else{
                temp_data = power_value - 10;
                status = dev->lp3907->write(RA_LP3907_1_ADDRESS,0x10,0x75);
                status = status | dev->lp3907->write(RA_LP3907_1_ADDRESS,RA_LP3907_1_IOVCC_CMD,temp_data);
                status = status | dev->lp3907->read(RA_LP3907_1_ADDRESS,RA_LP3907_1_IOVCC_CMD,&temp_data);
            }
            //printf("    SET POWER IOVCC %d status %d\r\n",power_value,status);
            break;
        case RA_POWER_MVDD:
            if((power_value < 10) || (power_value > 35))
            {
                MIPI_CMD_INFO("The mvdd voltage is %d *100*mv and is not set within the range of 10 to 35, error\r\n",power_value);
                return BSP_ERROR;
            }
            else{
                temp_data = power_value - 10;
                status = dev->lp3907->write(RA_LP3907_2_ADDRESS,0x10,0x75);
                status = status | dev->lp3907->write(RA_LP3907_2_ADDRESS,RA_LP3907_2_MVDD_CMD,temp_data);
            }
            //printf("    SET POWER MVDD %d status %d\r\n",power_value,status);
            break;
        case RA_POWER_VDDIO:
            if((power_value < 10) || (power_value > 35))
            {
                MIPI_CMD_INFO("The vddio voltage is %d *100*mv and is not set within the range of 10 to 35, error\r\n",power_value);
                return BSP_ERROR;
            }
            else{
                temp_data = power_value - 10;
                status = dev->lp3907->write(RA_LP3907_2_ADDRESS,0x10,0x75);
                status = status | dev->lp3907->write(RA_LP3907_2_ADDRESS,RA_LP3907_2_VDDIO_CMD,temp_data);
            }
            break;
        default:
            return BSP_ERROR;
    }

    return status;
}

BSP_STATUS_T drv_ra_set_power_off(drv_ra_dev_t *dev,uint8_t power_name,uint8_t en){
    BSP_STATUS_T status;
    uint8_t data;
    switch (power_name){
        case RA_POWER_VSP:
            status = dev->tca9554->sel(RA_TCA9554_POWER_OFF,CHANNEL_2,en);
            //printf("    SET POWER VSP OFF status %d\r\n",status);
            //p2
            break;
        case RA_POWER_VSN:
            status = dev->tca9554->sel(RA_TCA9554_POWER_OFF,CHANNEL_6,1);//-12V
            status = status| dev->tca9554->sel(RA_TCA9554_POWER_OFF,CHANNEL_3,en);
            //printf("    SET POWER VSN OFF status %d\r\n",status);
            //p3
            break;
        case RA_POWER_VCI://no use
//            status = dev->lp3907->read(RA_LP3907_1_ADDRESS,RA_LP3907_BKLDOEN_CMD,&temp_data[0]);
//            printf("vci current reg %d\r\n",temp_data[0]);
//            if(en == 0)
//                temp_data[1] = CLEAR_BIT(temp_data[0],1<<4);
//            else
//                temp_data[1] = SET_BIT(temp_data[0],1<<4);
//            printf("vci new reg %d\r\n",temp_data[1]);
            status = dev->lp3907->write(RA_LP3907_1_ADDRESS,RA_LP3907_BKLDOEN_CMD, 0x25);
            //printf("    SET POWER VCI OFF status %d\r\n",status);
            //p1 //lp3907_1
            break;
        case RA_POWER_IOVCC://no use
//            osDelay(100);
//            status = dev->lp3907->read(RA_LP3907_1_ADDRESS,RA_LP3907_BKLDOEN_CMD,&temp_data[0]);
//            printf("    read iovcc reg 0x%x \r\n",temp_data[0]);
//            if(en == 0)
//                temp_data[1] = CLEAR_BIT(temp_data[0],1<<6);
//            else
//                temp_data[1] = SET_BIT(temp_data[0],1<<6);
//            printf("    write iovcc reg 0x%x \r\n",temp_data[1]);

            status = dev->lp3907->write(RA_LP3907_1_ADDRESS,RA_LP3907_BKLDOEN_CMD, 0x25);
//            dev->lp3907->read(RA_LP3907_1_ADDRESS,RA_LP3907_BKLDOEN_CMD,&data);
//            printf("IOVCC power off 0x%x \r\n",data);
//            dev->lp3907->read(RA_LP3907_1_ADDRESS,0x39,&data);
//            printf("IOVCC power off 0x%x \r\n",data);
            //status = status | dev->tca9554->sel(RA_TCA9554_POWER_OFF,CHANNEL_7,en);//RESET
//            osDelay(100);
//            printf("    SET POWER IOVCC OFF status %d\r\n",status);
//            status = dev->lp3907->read(RA_LP3907_1_ADDRESS,RA_LP3907_BKLDOEN_CMD,&temp_data[0]);
//            printf("    read iovcc reg 0x%x \r\n",temp_data[0]);
//            osDelay(100);
            //p0 //lp3907_1
            break;
        case RA_POWER_MVDD://no use
//            status = dev->lp3907->read(RA_LP3907_2_ADDRESS,RA_LP3907_BKLDOEN_CMD,&temp_data[0]);
//            if(en == 0)
//                temp_data[1] = CLEAR_BIT(temp_data[0],1<<4);
//            else
//                temp_data[1] = SET_BIT(temp_data[0],1<<4);
            status = dev->lp3907->write(RA_LP3907_2_ADDRESS,RA_LP3907_BKLDOEN_CMD, 0x65);
            //printf("    SET POWER MVDD OFF status %d\r\n",status);
            //p5
            break;
        case RA_POWER_VDDIO://no use
//            status = dev->lp3907->read(RA_LP3907_2_ADDRESS,RA_LP3907_BKLDOEN_CMD,&temp_data[0]);
//            if(en == 0)
//                temp_data[1] = CLEAR_BIT(temp_data[0],1<<4);
//            else
//                temp_data[1] = SET_BIT(temp_data[0],1<<4);
            status = dev->lp3907->write(RA_LP3907_2_ADDRESS,RA_LP3907_BKLDOEN_CMD, 0x25);
            //printf("    SET POWER VDDIO OFF status %d\r\n",status);
            //p4
            break;
        case RA_POWER_12V://0 open 1 close
            status = dev->tca9554->sel(RA_TCA9554_POWER_OFF,CHANNEL_6,~en);
            //printf("    SET POWER 12V OFF status %d\r\n",status);
            break;
        default:
            return BSP_ERROR;
    }
    return status;
}

BSP_STATUS_T drv_ra_init_power_on_setting(drv_ra_dev_t *dev,uint8_t seq){
    BSP_STATUS_T status;
    status = dev->tca9554->write(RA_TCA9554_POWER_OFF,0x01,seq);
    if(status != BSP_OK)
        return BSP_ERROR;
    status = dev->tca9554->write(RA_TCA9554_POWER_OFF,0x03,0x00);
    if(status != BSP_OK)
        return BSP_ERROR;
    status = dev->adc121c027->write(RA_ADC121C027_ADDRESS,0x02,(0x00 & 0x07)<<5);
    if(status != BSP_OK)
        return BSP_ERROR;
    else
        return BSP_OK;
}

ra_ina3221_t ra_ina3221 = {
        .read = &drv_ra_i2c_read_data_int16,
        .write = &drv_ra_i2c_write_data_word,
};

ra_tca9554_t ra_tac9554 = {
        .read = &drv_ra_i2c_read_reg,
        .write = &drv_ra_i2c_write_reg,
        .sel = &drv_ra_mux_sel,
};

ra_sgm3804_t ra_sgm3804 = {
        .write = &drv_ra_i2c_write_reg,
};

ra_lp3907_t ra_lp3907 = {
        .read = &drv_ra_i2c_read_reg,
        .write = &drv_ra_i2c_write_reg,
};

ra_adc121c027_t ra_adc121c027 = {
        .read = &drv_ra_i2c_read_data_int16,
        .write = &drv_ra_i2c_write_reg,
};

drv_ra_dev_t ra_sub_dev = {
//        .tca9554 = &ra_tca9554,
        .tca9554 = &ra_tac9554,
        .ina3221 = &ra_ina3221,
        .sgm3804 = &ra_sgm3804,
        .lp3907 =  &ra_lp3907,
        .adc121c027 = &ra_adc121c027,
};

drv_ra_ops_t ra_sub_ops = {
        .init_power_on = drv_ra_init_power_on_setting,
        .set_power_vol = drv_ra_set_power_vol,
        .set_power_en = drv_ra_set_power_off,
};

drv_ra_t ra_dev_main_0 = {
    .main_address = 0x40,
    .dev = &ra_sub_dev,
    .ops = &ra_sub_ops,
};
/* Exported functions --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

