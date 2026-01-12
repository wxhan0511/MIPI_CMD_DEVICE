//
// Created by Bobby on 2023/4/25.
//
#include "stdio.h"
#include <string.h>
#include "gtb_op.h"
#include "tp_define.h"
#include "bsp_gtb.h"
#include <stdlib.h>
//uint8_t TouchData_Demo_RawData[MAXTOUCHDATASIZE];
extern tp_config_t tp_config;
extern uint8_t dynamic_cmd[MAXCMDCOUNT][MAXCMDSIZE];

int32_t gtb_read_coordination_dynamic(tp_config_t *tp_config,uint8_t* read_data)
{
    uint8_t cmddata[MAXCMDSIZE];
    uint16_t cmdLength;
    uint8_t *pCmd;
    int32_t status;
    uint8_t pCmd2;
    for(uint8_t i = 0;i < tp_config->dynamic_total_cmd_num;i++)
    {
        cmdLength = dynamic_cmd[i][3] << 8;
        cmdLength |= dynamic_cmd[i][4];
        switch(dynamic_cmd[i][2])
        {
            case DYNAMIC_WRITE:
                for(uint8_t j = 0;j < cmdLength;j++)
                {
                    if(j > (MAXCMDSIZE - 1))
                        break;
                    cmddata[j] = dynamic_cmd[i][j + 5];
                }
                status = gtb_write_data(tp_config,tp_config->interface_mode,cmddata,cmdLength);
                if(status != HAL_OK)
                    return status;
                break;
            case DYNAMIC_READ:
                status = gtb_read_data(tp_config,tp_config->interface_mode,&pCmd2,read_data,cmdLength);
                if(status != HAL_OK)
                    return status;
                break;
            case DYNAMIC_DELAY:      
//                if(dynamic_cmd[i][12] == 1)
//                    delay_ms(dynamic_cmd[i][13]);
//                else if(dynamic_cmd[i][12] == 0)
//                    delay_us(dynamic_cmd[i][13]);
                break;
            default:
                break;
        }
    }

    return HAL_OK;
}
//extern uint8_t TouchData_Demo_RawData[MAXTOUCHDATASIZE];
//================================GC==================================//
extern uint8_t touch_data_demo_raw_data[MAXTOUCHDATASIZE];
extern tp_config_t tp_config_hid;
int32_t  gtb_read_raw_data_gc(bool interface,uint8_t ic_type_index,uint16_t len)
{
    int32_t status = 0;

    if(interface == I2C_MODE){
        
        //status = I2C1_ReadData(TouchData_Demo_RawData,len);
        i2c_read_data(tp_config_hid.i2c_slave_address,touch_data_demo_raw_data,len);
    }
    else //if(interface == SPI_MODE)
    {
        uint8_t *pVoid = NULL;
        //printf("read raw %d %d \r\n",ic_type_index,len);
        if((ic_type_index == IC_INDEX_GC_7271) && (len > 1024))
        {
            printf("bb\r\n");
            tp_spi_cs_enable(true);
            //delay_us(10);
            status = spi_read_write_data1(pVoid, &touch_data_demo_raw_data[0], 1024, 0);

            if(status != HAL_OK)
                return status;

            tp_spi_cs_enable(false);
            //osDelay(1);
            tp_spi_cs_enable(true);
            status = spi_read_write_data1(pVoid, &touch_data_demo_raw_data[1024], len - 1024, 0);
            //delay_us(10);
            tp_spi_cs_enable(false);
        }
        else
        {
            tp_spi_cs_enable(true);
            //delay_us(10);
            //status = spi_read_data2(touch_data_demo_raw_data, len);
            status = spi_read_write_data3(pVoid, touch_data_demo_raw_data, len, 0);
            tp_spi_cs_enable(false);
            

        }
    }
    return status;
}

uint16_t gtb_get_demo_data_len(uint8_t ic_type_index)
{
    uint16_t datalength;
    datalength = 0;
    switch(ic_type_index & 0x0f)
    {
        case IC_TYPE_GC:
            datalength = 65;
            break;
    }
    return datalength;
}

uint16_t gtb_get_debug_data_index(uint8_t ic_type_index)
{
    uint16_t datalength;
    datalength = 0;
    switch(ic_type_index & 0x0f)
    {
        case IC_TYPE_GC:
            datalength = 0;
            break;
    }
    return datalength;
}

int32_t gtb_read_raw_data(tp_config_t *tp_config,bool interface,uint8_t ic_type_index,uint16_t len)
{
    int32_t status = 0;
    //printf("interface=%x\r\n",interface);
    //printf("ic_type_index=0x%x\r\n",ic_type_index);
    //printf("gtb_read_raw_data: len=%d\r\n",len);
    //printf("tp_config->ic_type_index & 0x0f=0x%x\r\n",(tp_config->ic_type_index & 0x0f));
    if(len > MAXTOUCHDATASIZE)
        len = MAXTOUCHDATASIZE;
    switch(tp_config->ic_type_index & 0x0f)
    {
        case IC_TYPE_GC:
            status = gtb_read_raw_data_gc(interface,ic_type_index,len);
            //printf("after gtb_read_raw_data_gc\r\n");
            break;
        default:
            break;
    }
    return status;
}

void gtb_read_debug_data(tp_config_t *tp_config,bool interface,uint8_t ic_type_index,uint16_t len)
{
    if(len > MAXTOUCHDATASIZE)
        len = MAXTOUCHDATASIZE;
    switch(ic_type_index & 0x0f)
    {
        case IC_TYPE_GC:

            break;
        default:
            break;
    }
}

void fw_mode_switch(tp_config_t *tp_config,uint8_t ic_type_index,bool interface,uint8_t mode)
{
    uint8_t cmd1[1];
    switch(ic_type_index & 0x0f)
    {
        case IC_TYPE_GC:
#if false
            switch((tp_config->ic_type_index & 0xf0) >> 4)
			{
				case IC_INDEX_GC_7271:
					if(interface == I2C_MODE)
					{

					}
					else
					{

					}
					break;
				case IC_INDEX_GC_7371:
					if(interface == I2C_MODE)
					{
						cmd1[0] = 0x95;
						I2C1_WriteReg_GC(0x0020,cmd1,1);
						I2C1_WriteReg_GC(0x0026,&mode,1);
					}
					else
					{
						cmd1[0] = 0x95;
						SPI2_WriteReg_GC(0x0020,cmd1,1);
						SPI2_WriteReg_GC(0x0026,&mode,1);
				}
					break;
				default:
					break;
			}
#endif
            break;
        default:
            break;
    }
}
//gloable variables initial
void gtb_global_var_init(tp_config_t *tp_config)
{
    tp_config->debug_enable = false;
    tp_config->flash_program_flash = false;
    tp_config->int_trans = true;
    tp_config->fw_mode = FWMODE_DISABLE;
    tp_config->raw_data_flag = false;
    tp_config->long_packet_enable = true;
    tp_config->long_packet_count = 0;
    tp_config->ic_touch_data_len = 0;
    tp_config->ic_touch_data_index = 0;
    tp_config->ic_type_index = 0xff;
    tp_config->transfer_flag = false;
    tp_config->cs_high_en = true;
    tp_config->cs_low_en = true;
    tp_config->one_cs_for_host_download_enable = false;
    tp_config->int_flag = false;
    tp_config->ex_ti_flag = false;
    tp_config->download_enable = false;
    tp_config->download_complete = false;
    tp_config->offline_mode = 0;
    tp_config->fw_len = 0;
    tp_config->transfer_feedback_enable = false;
//    gbSPI_TX_Flag = false;
//    gbSPI_RX_Flag = false;
//    gI2CSPIRegAddress = 0;
    tp_config->i2c_slave_address = 0x4c;
    tp_config->spi_i2c_data_len = 0;
    tp_config->spi_i2c_count = 0;
    tp_config->interface_mode = I2C_MODE;
    tp_config->dynamic_cmd_count = 0;
    tp_config->dynamic_total_cmd_num = 0;
    tp_config->dynamic_cmd_data_len = 0;
    tp_config->transfer_status = 0;
    tp_config->fw_data_len = 0;
    tp_config->fw_data_len1 = 0;
    for(int i = 0;i < MAXCMDCOUNT;i++)
    {
        for(int j = 0;j < MAXCMDSIZE;j++)
            dynamic_cmd[i][j] = 0;
    }

//    for(int i = 0;i < (MAXUSBPACKETSIZE - 8);i++)
//        flash_Buffer[i] = 0;
//
//    memset(gUSBBuf_TX,0,sizeof(gUSBBuf_TX));
//    for(int i = 0;i < sizeof(gUSBBuf_RX);i++)
//    {
//        gUSBBuf_RX[i] = 0;
//    }
//    memset(gI2CSPIBuf_TX,0,sizeof(gI2CSPIBuf_TX));
//    memset(gI2CSPIBuf_RX,0,sizeof(gI2CSPIBuf_RX));
//    memset(SPI_Buffer_Tx,0,sizeof(SPI_Buffer_Tx));
//    memset(SPI_Buffer_Rx,0,sizeof(SPI_Buffer_Rx));
//    memset(gCOMBuf_TX,0,sizeof(gCOMBuf_TX));
//    memset(gCOMBuf_RX,0,sizeof(gCOMBuf_RX));

}

