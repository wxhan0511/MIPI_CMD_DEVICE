//
// Created by Bobby on 2023/12/20.
//

#include "bsp_multi_channel_sel.h"


BSP_STATUS_T bsp_multi_channel_sel(bsp_i2c_hw_t* i2c_bus,uint8_t address,multi_channel channel,multi_channel_en en){
    uint8_t status = HAL_OK;
    uint8_t data[2];
    status = i2c_bus->read_data(i2c_bus->handle,address,0x01,&data[0],1);
    if(status == BSP_OK){
        //printf("[bsp] read address %d data %d \r\n",address,data[0]);
        if(en == MULTI_CHANNEL_OFF)
            data[1] = CLEAR_BIT(data[0],channel);
        else
            data[1] = SET_BIT(data[0],channel);
        status = i2c_bus->write_data(i2c_bus->handle,address,0x01,&data[1],1);
        if(status == BSP_OK){
            data[1] = 0;
            status = i2c_bus->write_data(i2c_bus->handle,address,0x03,&data[1],1);
            if(status == BSP_OK){
                return i2c_bus->read_data(i2c_bus->handle, address, 0x01, &data[0], 1);
                printf("[warn] tca9554 status 0x%x \r\n",data[0]);
            }
        }
    }
//    else{
//        printf("[bsp] multi channel sel fail \r\n");
//    }
    return BSP_ERROR;
}

