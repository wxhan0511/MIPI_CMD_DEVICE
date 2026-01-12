#include "tp_define.h"
#include "stdint.h"
#include "gtb_com.h"
#include "bsp_gtb.h"
#include "string.h"
#include "gtb_op.h"
#include "gtb_flash_op.h"
#include "stdio.h"
#include "usbd_hid_custom.h"
#include "usbd_cdc_acm_if.h"
#include "usb_device.h"
#include "gtb_op.h"
#include "bsp_dwt.h"
typedef struct _ _;
const uint16_t GTBVersion = 0x1027;
extern USBD_HandleTypeDef hUsbDevice;

// tp_config_t tp_config;
uint8_t gtb_io_level = 0;
extern uint8_t flash_buffer[MAXUSBPACKETSIZE - 8];
uint8_t dynamic_cmd[MAXCMDCOUNT][MAXCMDSIZE] = {0}; // dynamic cmd
uint8_t usb_tx_buffer[MAXUSBPACKETSIZE];
uint8_t usb_rx_buffer[MAXUSBPACKETSIZE];
uint8_t i2c_spi_long_packet_tx_buffer[2048];
uint8_t i2c_spi_long_packet_rx_buffer[2048];
uint8_t touch_data_demo_raw_data[MAXTOUCHDATASIZE];
uint8_t i2c_spi_tx_buf[MAXUSBPACKETSIZE] = {0}; // I2C/SPI send data
uint8_t i2c_spi_rx_buf[MAXUSBPACKETSIZE] = {0}; // I2C/SPI receieve data
// send error code to tool

uint8_t *pCmd;
uint8_t data1[64];

// uint16_t i,j,len,len1;
uint8_t gtb_flag = 0;
uint8_t gtb_fs_transmit(uint8_t *hid_data, uint32_t len, uint8_t com_mode)
{
    //printf("com mode %d \r\n",com_mode);
    uint8_t status = 0;
    if ((GTB_HID == com_mode) || (GTB_MIX == com_mode))
    {
        
        //HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_13, GPIO_PIN_SET);
        //status = USBD_HID_GetReportTrigger(0U, 0U, hid_data, len);
        status = USBD_CUSTOM_HID_SendReport(&hUsbDevice, hid_data, len);
        //HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_13, GPIO_PIN_RESET);
    }
    else if (GTB_CDC == com_mode)
    {
        //HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_13, GPIO_PIN_SET);
        //status = USBD_CDC_ACM_WriteData(0, hid_data, len);
        status = CDC_Transmit(0, hid_data, len);
        //HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_13, GPIO_PIN_RESET);
    }
    return status;
}

uint32_t hid_hs_get_data(uint8_t *get_data, uint32_t data_num)
{
    uint32_t len = 0;
    // USBD_CUSTOM_HID_HandleTypeDef *hhid;
    // hhid = (USBD_CUSTOM_HID_HandleTypeDef*)hUsbDeviceHS.pClassData;
    // memcpy(get_data,hhid->Report_buf,data_num);
    return data_num;
}

void send_error_code(bool mode, uint8_t *output, int32_t error_code, uint8_t com_mode)
{
    output[0] = CMD_I_AM_GTB;
    output[1] = CMD_SEND_ERROR_CODE;
    if (mode == I2C_MODE)
        output[2] = CMD_I2CSPI_MODE_I2C;
    else
        output[2] = CMD_I2CSPI_MODE_SPI;
    output[3] = abs(error_code);
    output[4] = CMD_SEND_ERROR_CODE;
    for (int i = 5; i < MAXUSBPACKETSIZE; i++)
    {
        output[i] = 0;
    }

    gtb_fs_transmit(output, 64, com_mode);
}

// send program status to tool
void send_usb_trans_status(bool mode, uint8_t *output, uint8_t status, uint8_t com_mode)
{
    output[0] = CMD_I_AM_GTB;
    output[1] = CMD_USB_TRANSFER_STATUS;
    if (mode == I2C_MODE)
        output[2] = CMD_I2CSPI_MODE_I2C;
    else
        output[2] = CMD_I2CSPI_MODE_SPI;
    output[3] = status;
    output[4] = CMD_USB_TRANSFER_STATUS;
    for (int i = 5; i < MAXUSBPACKETSIZE; i++)
    {
        output[i] = 0;
    }
    GTB_DEBUG("start %d\r\n",HAL_GetTick());
    gtb_fs_transmit(output, 64, com_mode);
    GTB_DEBUG("end %d\r\n",HAL_GetTick());
}

// send FSPI program status to tool
void send_usb_trans_status_fspi(bool mode, uint8_t *output, uint16_t count, uint8_t result, uint8_t com_mode)
{
    output[0] = CMD_I_AM_GTB;
    output[1] = CMD_FLASH_OPERATION_IDM;
    output[2] = result;
    output[6] = count >> 8;
    output[7] = count;

    for (int i = 8; i < MAXUSBPACKETSIZE; i++)
    {
        output[i] = 0;
    }
    gtb_fs_transmit(output, 64, com_mode);
}

void gtb_generic_com(tp_config_t *tp_config,uint8_t *arg, uint8_t *output, uint8_t com_mode)
{
    //printf("gtb_generic_com cmd: %x,%x,%x,%x,%x,%x,%x\r\n",arg[0],arg[1],arg[2],arg[3],arg[4],arg[5],arg[6]);
    if (arg[3] == CMD_I2CSPI_TRANSFER_HOSTDOWNLOAD) // hostdownoad and IDM
    {
        ex_ti_initial(tp_config,ENABLE);
        //        LED_On(LED_SPI,false);
        //        LED_On(LED_I2C,false);
        if (arg[1] == CMD_SPI_TRANSFER)
            tp_config->interface_mode = SPI_MODE;
        else
            tp_config->interface_mode = I2C_MODE;
        tp_config->download_enable = true;
        tp_config->int_flag = false;
        tp_config->offline_mode = 0;
        tp_config->fw_len = 0;
    }
    else if (arg[3] == CMD_I2CSPI_TRANSFER_IDM)
    {
        tp_config->download_enable = false; // disable hostdownload
        tp_config->transfer_flag = true;    // prevent int after RST
        //        LED_On(LED_SPI,false);
        //        LED_On(LED_I2C,false);
        tp_config->offline_mode = 0;
        tp_config->fw_len = 0;
        output[0] = CMD_I_AM_GTB;

        if (arg[1] == CMD_SPI_TRANSFER)
        {
            tp_config->interface_mode = SPI_MODE;
            output[1] = CMD_SPI_TRANSFER;
            tp_config->cs_high_en = true;
            tp_config->cs_low_en = true;
        }
        else
        {
            tp_config->interface_mode = I2C_MODE;
            output[1] = CMD_I2C_TRANSFER;
        }
        output[2] = CMD_HOSTDOWNLOAD_WAIT_DATA; // tell GTB Tool to send I2C data
        gtb_fs_transmit(output, 64, com_mode);
        return;
    }

    switch (arg[1])
    {
        case CMD_RESET_FLOW: // reset GTB FW flow
            GTB_DEBUG("gtb_generic_com cmd: %x\r\n",arg[1]);
            if (arg[2] == 0xff) {
                gtb_global_var_init(tp_config);
                //                //HW_Initial();
                // USBD_Initialize(0);
                // USBD_Connect(0);
                MX_USB_DEVICE_Init();
            }
            break;
        case CMD_READ_GTB_VERSION: // get GTB FW version
            TIME_DEBUG("ver: %lu \r\n", dwt_get_ms());
            GTB_DEBUG("gtb_generic_com cmd: %x\r\n",arg[1]);            output[0] = arg[0];
            output[1] = arg[1];
            output[2] = (GTBVersion >> 12) & 0x0f;
            output[3] = (GTBVersion >> 8) & 0x0f;
            output[4] = (GTBVersion >> 4) & 0x0f;
            output[5] = GTBVersion & 0x0f;
            for (int i = 6; i < MAXUSBPACKETSIZE; i++)
                output[i] = 0;
            TIME_DEBUG("ver: %lu \r\n", dwt_get_ms());
            gtb_fs_transmit(output, 64, com_mode);
            TIME_DEBUG("verend: %lu \r\n", dwt_get_ms());
            break;
        case CMD_READ_GTB_ID: // get GTB id
            GTB_DEBUG("gtb_generic_com cmd: %x\r\n",arg[1]);            
            output[0] = arg[0];
            output[1] = arg[1];
            output[2] = 0x01;
            for (int i = 3; i < MAXUSBPACKETSIZE; i++)
                output[i] = 0;
            gtb_fs_transmit(output, 64, com_mode);
            break;
        case CMD_WRITE_GTB_GPIO_BIT: // write GTB GPIO bit
            if (arg[2] == 1 && arg[3] == 8)
            {
                //TP reset
                //printf("tp reset %d\r\n",arg[4]);
                //TODO:
                //HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_13, arg[4]);
            }
            //osDelay(1);
        //HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_13, arg[4]);
            // if(arg[2] > 4)
            //     break;
            // if(arg[3] > 15)
            //     break;
            // if(arg[4] != 0)
            //     data = 1;
            // else
            //     data = 0;
            // port = arg[2];
            // bit = arg[3];
            // GPIO_PinWrite(GPIO_PORT[port].port,bit,data);
            //printf("write %x %x %x %x %x %x %x\r\n",arg[0],arg[1],arg[2],arg[3],arg[4],arg[5],arg[6]);
            break;
        case CMD_READ_GTB_GPIO_BIT: // read GTB GPIO bit
            {
                //printf("read gpio bit\r\n");
                // osDelay(100);
                // meter_get_tp_int_voltage();
                //gtb_flag = 0;
                 uint16_t i = 20;
                uint8_t ret = 0;
                //TODO:
                //  while (i--)
                //  {
                //     ret = HAL_GPIO_ReadPin(GPIOK,GPIO_PIN_3);
                //     //printf("ret:%d\r\n",ret);
                //     if (ret == 1)
                //     {
                //         //gtb_flag = 1;
                //         //printf("get irq\r\n");
                //         break;
                //     }
                //      osDelay(1);
                //  }

                // uint8_t bit;
                // uint16_t data;
                // if(arg[4] == 0) 					//read GPIO
                // {
                //     if(arg[2] > 4)
                //         break;
                //     if(arg[3] > 15)
                //         break;
                //     // bit = arg[3];
                //     // port = arg[2];
                //     // data = GPIO_PinRead(GPIO_PORT[port].port,bit);
                // }
                // else//read ADC
                // {
                //     if(arg[3] > 17)
                //         break;
                //     bit = arg[3];
                //     if (bit == 9)
                //     data = Get_Adc(bit);
                // }
                // printf("read %x %x %x %x %x %x %x\r\n",arg[0],arg[1],arg[2],arg[3],arg[4],arg[5],arg[6]);
                for(uint8_t i = 0;i < 5;i++)
                    output[i] = arg[i];

                // if (gtb_flag == 0)
                // {
                //     output[5] = 0x00;
                //     output[6] = 0x1c;
                //     gtb_flag ++;
                //     printf("zero\r\n");
                // }
                // else
                // {
                //     output[5] = 0x08;
                //     output[6] = 0x6b;
                //     // output[5] = 0x07;
                //     // output[6] = 0x08;
                //     gtb_flag = 0;
                //     printf("one\r\n");
                // }
                if (ret == 0)
                {
                    output[5] = 0x00;
                    output[6] = 0x1c;
                    //printf("read gpio bit ZERO\r\n");
                    //gtb_flag ++;
                }
                else
                {
                    if (gtb_io_level == 0)
                    {
                        output[5] = 0x08;
                        output[6] = 0x6b;
                    }
                    else if (gtb_io_level == 1)
                    {
                        output[5] = 0x0d;
                        output[6] = 0xc8;
                    }
                    else if (gtb_io_level == 2)
                    {
                        output[5] = 0x10;
                        output[6] = 0x3e;
                    }
                    // output[5] = 0x07;
                    // output[6] = 0x08;
                    //printf("read gpio bit hIGH\r\n");
                }
                for(int i = 7;i < MAXUSBPACKETSIZE;i++)
                    output[i] = 0;
                // osDelay(200);
                gtb_fs_transmit(output, 64, com_mode);
            }
            break;
        case CMD_CONFIG_GTB_GPIO_PORT:
            //            if(arg[2] > 4)
            //                break;
            //            if(arg[3] > 15)
            //                break;
            //            if(arg[4] > 4)
            //                break;
            //            if(arg[5] > 4)
            //                break;
            //            port = arg[2];
            //            bit = arg[3];
            //            gpio_mode = arg[4];
            //            gpio_config = arg[5];
            //            GPIO_PinConfigure(GPIO_PORT[port].port,bit,gpio_config,gpio_mode);
            break;
        case CMD_MISC_FUNCTION:
            switch (arg[2])
            {
                case CMD_I2C_HW_SW_SELECT: // i2c mode
//                    if (arg[3] == CMD_I2C_SW_ENABLE)
//                        HW_I2C_enable = false;
//                    else
//                        HW_I2C_enable = true;
//                    I2C1_Initial();
                    break;
                case CMD_FW_MODE_SET:

                    break;
                case CMD_TRANSFER_FEEDBACK_SET: // feed back
                    if (arg[3] == CMD_TRANSFER_FEEDBACK_DISABLE)
                        tp_config->transfer_feedback_enable = false;
                    else
                        tp_config->transfer_feedback_enable = true;
                    break;
                case CMD_DEBUG_MODE_SET: // debug mode
                    if (arg[3] == CMD_DEBUG_DISABLE)
                        tp_config->debug_enable = false;
                    else
                        tp_config->debug_enable = true;
                    break;
                case CMD_INT_TRANSFER_SET: // int transfer set
                    if (arg[3] == CMD_INT_TRANSFER_DISABLE)
                        tp_config->int_trans = false;
                    else
                        tp_config->int_trans = true;
                    //printf("int tran set %d %d \r\n",arg[3],tp_config->int_trans);
                    break;
                default:
                    break;
            }
            break;
        case CMD_I2CSPIUSART_CHANGE_INFO: // change i2c or spi or usart info
            switch (arg[2])
            {
                case CMD_I2CSPIUSART_CHANGE_INFO_I2C1: // i2c1
                    if (arg[3] == CMD_I2CSPI_CHANGE_SPEED) {
                        tp_config->i2c_speed_mode = arg[5];
                        tp_i2c_set_speed(arg[5]);
                    } else if (arg[3] == CMD_I2CSPI_CHANGE_SLAVE_ADDRESS) {
                        tp_config->i2c_slave_address = arg[4];
                    }
                    break;
                case CMD_I2CSPIUSART_CHANGE_INFO_SPI2: // i2c2
                    if (arg[3] == CMD_I2CSPI_CHANGE_SPEED)
                    {
//                    if(arg[5] < 8){
//
//                    }
                        tp_spi_set_speed(arg[5]);
                    }
                    else if (arg[3] == CMD_I2CSPI_CHANGE_SPI_CONFIG)
                    {
                        tp_spi_set_mode(arg[6]);
                    }
                    else if (arg[3] == CMD_I2CSPI_DEINIT_SPI)
                    {
                        // SPI_I2S_DeInit(SPI2);
                    }
                    break;
                case CMD_I2CSPIUSART_CHANGE_INFO_USART2: // usart2
//                    gBaudRate = arg[8] << 24;
//                    gBaudRate |= arg[9] << 16;
//                    gBaudRate |= arg[10] << 8;
//                    gBaudRate |= arg[11];
//
//                    Galaxycore_COMInit(gBaudRate);
                    break;
                default:
                    break;
            }
            break;
        case CMD_I2C_TRANSFER: // i2c transfer
        case CMD_SPI_TRANSFER: // spi transfer
            tp_config->test_count = 0;
            //bool tp_config->interface_mode = false;
            if (arg[1] == CMD_I2C_TRANSFER)
                tp_config->interface_mode = I2C_MODE;
            else
                tp_config->interface_mode = SPI_MODE;

            switch (arg[3])
            {
                case CMD_I2CSPI_TRANSFER_WRITE: // write
                    if (arg[4] == CMD_I2CSPI_TRANSFER_DATA)
                    {
                        if ((arg[5] == 0xdd) && (arg[6] == 0xee) && (arg[7] == 0xff)) // end
                        {
                            if (tp_config->interface_mode == SPI_MODE)
                            {
                                tp_config->cs_high_en = true;
                                tp_config->cs_low_en = true;
                            }
                            GTB_DEBUG("long packet enable(1:true,):%x\r\n",tp_config->long_packet_enable);
                            if (tp_config->long_packet_enable == true)
                            {
                                GTB_DEBUG("long packet enable\r\n");
                                GTB_DEBUG("i2c_spi_long_packet_tx_buffer:\r\n");
                                for(int i = 0; i < 4; i++)
                                {
                                    GTB_DEBUG("%x, ",i2c_spi_long_packet_tx_buffer[i]);
                                }
                                GTB_DEBUG("\r\n");
                                GTB_DEBUG("long_packet_count: %d\r\n",tp_config->long_packet_count);
                                GTB_DEBUG("tp_config->interface_mode(1:SPI MODE):%x\r\n",tp_config->interface_mode);

                                tp_config->long_packet_enable = false;
                                tp_config->transfer_status = gtb_write_data(tp_config,tp_config->interface_mode, i2c_spi_long_packet_tx_buffer,
                                                                            tp_config->long_packet_count);

                                GTB_DEBUG("tp_config->transfer_status(HAL_OK:0): %d\r\n", tp_config->transfer_status);                                            
                                if (tp_config->transfer_status != HAL_OK)
                                {
                                    send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                                    break;
                                }
                                else if (tp_config->transfer_feedback_enable == true)//前一次收到主机发的40，fc，02,01后才会进入这里
                                    send_usb_trans_status(tp_config->interface_mode, output, CMD_USB_TRANSFER_OK, com_mode);
                                if (arg[2] == CMD_FLASH_OPERATION_LONG_PACKET)
                                {
                                    ex_ti_initial(tp_config,ENABLE);
                                    tp_config->int_flag = false;

                                    // waiting for block program done
                                    tp_config->test_count = 0;
                                    while (tp_config->int_flag == false)
                                    {
                                        if (tp_config->test_count < CMD_FLASH_PROGRAM_READ_TIMEOUT)
                                            tp_config->test_count++;
                                        else
                                            break;
                                        // delay_us(7);
                                    }
                                    if (tp_config->int_flag == true)
                                    {
                                        // delay_us(10);
                                        tp_config->int_flag = false;
                                        tp_config->transfer_status = gtb_read_data(tp_config,tp_config->interface_mode, pCmd, data1, 8);
                                        if (tp_config->transfer_status == HAL_OK)
                                        {
                                            if ((data1[0] == CMD_I_AM_GTB) &&
                                                (data1[1] == CMD_USB_TRANSFER_STATUS) &&
                                                (data1[3] == CMD_USB_TRANSFER_NEXT))
                                            {
                                                send_usb_trans_status(tp_config->interface_mode, output,
                                                                      CMD_USB_TRANSFER_NEXT, com_mode); // tell Tool to send next flash data
                                            }
                                            else
                                            {
                                                send_error_code(tp_config->interface_mode, output, 1, com_mode);
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if (tp_config->interface_mode == SPI_MODE)
                                {
                                    tp_config->one_cs_for_host_download_enable = false;
                                    tp_spi_cs_enable(false);
                                    tp_config->transfer_flag = false;
                                }
                                else
                                {
                                    // LED_On(LED_I2C, true);
                                }
                            }
                            break;
                        }

                        tp_config->spi_i2c_data_len = arg[6] << 8;
                        tp_config->spi_i2c_data_len |= arg[7];
                        tp_config->spi_i2c_count = arg[2] << 8;
                        tp_config->spi_i2c_count |= arg[5];

                        if (tp_config->spi_i2c_data_len > 56) // in case of data overflow
                        {    
                            break;
                        }
                        GTB_DEBUG("spi_i2c_data_len: %d\r\n",tp_config->spi_i2c_data_len);
                        GTB_DEBUG("spi_i2c_count: %d\r\n",tp_config->spi_i2c_count);
                        GTB_DEBUG("tp_config->long_packet_count: %x\r\n",tp_config->long_packet_count);
                        if (tp_config->long_packet_enable == true)
                        {
                            if ((tp_config->long_packet_count + tp_config->spi_i2c_data_len) > MAXI2CSPIBUFFERSIZE)
                            {
                                tp_config->long_packet_enable = false;
                                send_error_code(tp_config->interface_mode, output, HAL_ERROR, com_mode);
                                break;
                            }
                            GTB_DEBUG("i2c_spi_long_packet_tx_buffer:\r\n");
                            for (int i = 0; i < tp_config->spi_i2c_data_len; i++)
                            {
                                i2c_spi_long_packet_tx_buffer[tp_config->long_packet_count + i] = arg[i + 8];
                                GTB_DEBUG("%x, ",i2c_spi_long_packet_tx_buffer[tp_config->long_packet_count + i]);
                            }
                            tp_config->long_packet_count += tp_config->spi_i2c_data_len;
                            break;
                        }
                        else
                        {
                            if (tp_config->interface_mode == SPI_MODE)
                            {
                                if ((tp_config->spi_i2c_count == 0) && (tp_config->one_cs_for_host_download_enable == true))
                                {
                                    tp_config->cs_high_en = true;
                                    tp_config->cs_low_en = false;
                                    tp_config->one_cs_for_host_download_enable = false;
                                }
                            }
                            for (int e = 0; e < tp_config->spi_i2c_data_len; e++)
                            {
                                i2c_spi_tx_buf[e] = arg[e + 8];
                            }
                        }
                        tp_config->transfer_status = gtb_write_data(tp_config,tp_config->interface_mode, i2c_spi_tx_buf, tp_config->spi_i2c_data_len);
                        if (tp_config->transfer_status != HAL_OK)
                        {
                            send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                        }
                        else if (tp_config->transfer_feedback_enable == true)
                        {
                            send_usb_trans_status(tp_config->interface_mode, output, CMD_USB_TRANSFER_NEXT, com_mode);
                        }

                        if (i2c_spi_tx_buf[1] == 0xaa)
                        {
                            tp_config->raw_data_delay_time = 10;
                        }
                    }
                    break;
                case CMD_I2CSPI_TRANSFER_READ: // read
                    if (arg[4] == CMD_I2CSPI_TRANSFER_DATA)
                    {
                        if ((arg[5] == 0xdd) && (arg[6] == 0xee) &&
                            (arg[7] == 0xff)) // hostdownload end
                        {
                            if (tp_config->interface_mode == SPI_MODE)
                            {
                                tp_config->cs_high_en = true;
                                tp_config->cs_low_en = true;
                            }
                            if (tp_config->long_packet_enable == true)
                            {
                                tp_config->long_packet_enable = false;
                                if (arg[2] == CMD_FLASH_OPERATION_LONG_PACKET)
                                {
                                    ex_ti_initial(tp_config,ENABLE);
                                    tp_config->int_flag = false;
                                    tp_config->long_packet_count = arg[14] << 8;
                                    tp_config->long_packet_count |= arg[15];
                                    tp_config->transfer_status = gtb_write_data(tp_config,tp_config->interface_mode, &arg[8], 8);
                                    if (tp_config->transfer_status != HAL_OK)
                                    {
                                        send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                                        break;
                                    }

                                    // waiting for block read done
                                    tp_config->test_count = 0;
                                    while (tp_config->int_flag == false)
                                    {
                                        if (tp_config->test_count < CMD_FLASH_PROGRAM_READ_TIMEOUT)
                                            tp_config->test_count++;
                                        else
                                            break;
                                        // delay_us(7);
                                    }
                                    if (tp_config->int_flag == true)
                                    {
                                        // delay_us(10);
                                        tp_config->int_flag = false;
                                        tp_config->transfer_status = gtb_read_data(tp_config,tp_config->interface_mode, pCmd,
                                                                                   i2c_spi_long_packet_rx_buffer,
                                                                                   tp_config->long_packet_count);
                                        if (tp_config->transfer_status == HAL_OK)
                                        {
                                            if ((i2c_spi_long_packet_rx_buffer[0] == CMD_I_AM_GTB) &&
                                                (i2c_spi_long_packet_rx_buffer[2] == CMD_FLASH_READ_END) &&
                                                (i2c_spi_long_packet_rx_buffer[3] == CMD_FLASH_READ_OK))
                                            {
                                            }
                                            else
                                            {
                                                break;
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    tp_config->long_packet_count = arg[8] << 8;
                                    tp_config->long_packet_count |= arg[9];
                                    tp_config->transfer_status = gtb_read_data(tp_config,tp_config->interface_mode, i2c_spi_long_packet_tx_buffer,
                                                                               i2c_spi_long_packet_rx_buffer,
                                                                               tp_config->long_packet_count);
                                }

                                if (tp_config->transfer_status != HAL_OK)
                                {
                                    send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                                }
                                else
                                {
                                    tp_config->fw_data_len = tp_config->long_packet_count / 56;
                                    tp_config->fw_data_len1 = tp_config->long_packet_count % 56;

                                    output[0] = CMD_I_AM_GTB;
                                    output[1] = CMD_I2C_TRANSFER;
                                    output[2] = 0x00;
                                    output[3] = CMD_I2CSPI_TRANSFER_READ;
                                    output[4] = CMD_I2CSPI_TRANSFER_DATA;
                                    for (int j = 0; j < tp_config->fw_data_len; j++)
                                    {
                                        output[5] = j;
                                        output[6] = 0;
                                        output[7] = 56;
                                        for (int i = 0; i < 56; i++)
                                        {
                                            output[i + 8] = i2c_spi_long_packet_rx_buffer[i + j * 56];
                                        }
                                        gtb_fs_transmit(output, 64, com_mode);
                                        // delay_ms(2);
                                    }
                                    if (tp_config->fw_data_len1 != 0)
                                    {
                                        output[5] = tp_config->fw_data_len;
                                        output[6] = tp_config->fw_data_len1 >> 8;
                                        output[7] = tp_config->fw_data_len1 & 0xff;
                                        for (int i = 0; i < tp_config->fw_data_len1; i++)
                                        {
                                            output[i + 8] = i2c_spi_long_packet_rx_buffer[i + tp_config->fw_data_len * 56];
                                        }
                                        gtb_fs_transmit(output, 64, com_mode);
                                    }
                                }
                            }
                            break;
                        }

                        tp_config->spi_i2c_data_len = arg[6] << 8;
                        tp_config->spi_i2c_data_len |= arg[7];

                        if (tp_config->spi_i2c_data_len > 56) // in case of data overflow
                            break;

                        for (int i = 0; i < tp_config->spi_i2c_data_len; i++)
                        {
                            i2c_spi_tx_buf[i] = arg[i + 8];
                        }

                        for (int i = 0; i < 8; i++)
                            output[i] = arg[i];
                        if (gtb_read_data(tp_config,tp_config->interface_mode, i2c_spi_tx_buf, i2c_spi_rx_buf, tp_config->spi_i2c_data_len) !=
                            HAL_OK)
                        {
                            output[2] = 0x01;
                        }
                        else
                        {
                            output[2] = 0x00;
                        }

                        for (int i = 0; i < (MAXUSBPACKETSIZE - 8); i++)
                            output[i + 8] = i2c_spi_rx_buf[i];
                        gtb_fs_transmit(output, 64, com_mode);
                    }
                    break;
                case CMD_I2CSPI_TRANSFER_LONG_PACKET:
                    tp_config->long_packet_enable = true;
                    tp_config->spi_i2c_data_len = 0;
                    tp_config->long_packet_count = 0;
                    memset(i2c_spi_long_packet_tx_buffer, 0, sizeof(i2c_spi_long_packet_tx_buffer));
                    break;

                default:
                    break;
            }
            break;
        case CMD_SPI_DYNAMIC_CONTROL:
        case CMD_I2C_DYNAMIC_CONTROL:
            if (arg[1] == CMD_I2C_DYNAMIC_CONTROL)
                tp_config->interface_mode = I2C_MODE;
            else if (arg[1] == CMD_SPI_DYNAMIC_CONTROL)
                tp_config->interface_mode = SPI_MODE;

            tp_config->ic_type_index = arg[3];
            if (((tp_config->ic_type_index & 0x70) >> 4) == IC_INDEX_GC_7271)
            {
                if ((tp_config->ic_type_index & 0x80) == 0)
                    tp_config->ipx_format = true;
                else
                    tp_config->ipx_format = false;
            }

            tp_config->fw_mode = arg[2];
            if (tp_config->fw_mode == FWMODE_DISABLE)
            {
                fw_mode_switch(tp_config,tp_config->ic_type_index, tp_config->interface_mode, FWMODE_DISABLE);
                //ex_ti_initial(tp_config,DISABLE);
                HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
                tp_config->ex_ti_flag = false;
                tp_config->int_flag = false;
                break;
            }
            else
            {
                ex_ti_initial(tp_config,ENABLE);
                tp_config->int_flag = false;
                if (tp_config->fw_mode != FWMODE_DEMO)
                {
                    if ((arg[10] == 1) &&
                        ((tp_config->fw_mode == FWMODE_RAWDATA) || (tp_config->fw_mode == FWMODE_DEBUG))) // data length
                    {
                        tp_config->ic_touch_data_len = (arg[11] << 8) | arg[12];
                        tp_config->ic_touch_data_len1 = tp_config->ic_touch_data_len;
                    }

                    if (tp_config->ic_touch_data_len == 0) // prevent error data length
                        tp_config->fw_mode = FWMODE_DISABLE;

                    if (tp_config->ic_touch_data_len > MAXTOUCHDATASIZE) // in case of data overflow
                        tp_config->ic_touch_data_len = tp_config->ic_touch_data_len;
                    tp_config->raw_data_delay_time = 0;
                }
            }

            tp_config->dynamic_cmd_count = arg[5];
            if (arg[10] == 1) // cmd read
            {
                tp_config->dynamic_cmd_data_len = 0;
            }
            else // cmd write
            {
                tp_config->dynamic_cmd_data_len = arg[6] << 8;
                tp_config->dynamic_cmd_data_len |= arg[7];
            }

            if (tp_config->dynamic_cmd_count >= MAXCMDCOUNT) // in case of data array overflow
                break;

            tp_config->i2c_slave_address = arg[9];
            if (tp_config->dynamic_cmd_count == tp_config->dynamic_total_cmd_num)
                tp_config->dynamic_total_cmd_num++;

            for (int i = 0; i < (tp_config->dynamic_cmd_data_len + 5); i++)
            {
                if (i > MAXCMDSIZE)
                    break;
                dynamic_cmd[tp_config->dynamic_cmd_count][i] = arg[8 + i];
            }

            if ((tp_config->dynamic_cmd_count + 1) == arg[4])
            {
                output[0] = CMD_I_AM_GTB;
                output[1] = CMD_I2C_DYNAMIC_CONTROL;
                if ((tp_config->dynamic_cmd_count + 1) == tp_config->dynamic_total_cmd_num)
                {
                    output[2] = tp_config->fw_mode;
                    output[3] = 0x01;
                    tp_config->report_num = 0;

                    if ((tp_config->ic_type_index & 0x0f) == IC_TYPE_NT)
                    {
                        tp_config->raw_data_flag = true;
                    }
                    fw_mode_switch(tp_config,tp_config->ic_type_index, tp_config->interface_mode, tp_config->fw_mode);
                }
                else
                    output[3] = 0x00;
            }
            break;

        case CMD_COM_OPERATION: // usart debug
//            if (arg[2] == CMD_COM_OPEN) {
//                bCOMDebug = true;
//                if (arg[3] == CMD_COM_SEND) {
//                    COMDataLength = arg[6] << 8;
//                    COMDataLength |= arg[7];
//                    len = COMDataLength / 56;
//                    len1 = COMDataLength % 56;
//                    if (len == 0) {
//                        for (i = 0; i < COMDataLength; i++) {
//                            gCOMBuf_TX[i] = arg[8 + i];
//                            if (arg[8 + i] == 0x0a)
//                                break;
//                        }
//                    } else {
//                        for (i = 0; i < len; i++) {
//                            for (j = 0; j < 56; j++) {
//                                gCOMBuf_TX[i * 56 + j] = arg[8 + j];
//                                if (arg[8 + j] == 0x0a)
//                                    break;
//                            }
//                        }
//                        if (len1 != 0) {
//                            for (j = 0; j < len1; j++) {
//                                gCOMBuf_TX[len1 * 56 + j] = arg[8 + j];
//                                if (arg[8 + j] == 0x0a)
//                                    break;
//                            }
//                        }
//                    }
//                    gtb_fs_transmit(gCOMBuf_TX);
//                    //Galaxycore_COMSendData(USART2, gCOMBuf_TX, COMDataLength);
//                }
//            } else if (arg[2] == CMD_COM_CLOSE) {
//                bCOMDebug = false;
//            }
            break;

        case CMD_INA226_GET_DATA:
//            if ((arg[2] != INA226_SLAVE_ADDRESS_1) && (arg[2] != INA226_SLAVE_ADDRESS_2) &&
//                (arg[2] != INA226_SLAVE_ADDRESS_3)) {
//                break;
//            }
//            GetINA226Result(arg[2], ((arg[3] << 8) | arg[4]));
//            for (i = 0; i < 4; i++)
//                output[i] = arg[i];
//            for (i = 0; i < 4; i++)
//                output[4 + i] = ina226_data.Error_status[i];;
//
//            output[8] = ina226_data.BUSVoltage[0];
//            output[9] = ina226_data.BUSVoltage[1];
//            output[10] = ina226_data.Current[0];
//            output[11] = ina226_data.Current[1];
//            output[12] = ina226_data.Power[0];
//            output[13] = ina226_data.Power[1];
//            output[14] = ina226_data.ShuntVoltage[0];
//            output[15] = ina226_data.ShuntVoltage[1];
//            gtb_fs_transmit(output,64);
            break;

        case CMD_FLASH_OPERATION_IDM:
            if (arg[2] != CMD_FLASH_PROGRAM_OFFLINE_IDM)
                break;
            uint16_t prog_unit = (arg[6] << 8) | arg[7];
            tp_config->fw_len = 65536;
            //                PC10_ON;
            //                PA15_ON;
            //                //delay_ms(500);
            //                PC10_OFF;
            //                PA15_OFF;
            // Enter IDM
            i2c_spi_tx_buf[0] = 0x95;
            i2c_spi_tx_buf[1] = 0x27;
            i2c_spi_tx_buf[2] = 0x0F;
            i2c_spi_tx_buf[3] = 0xF0;
            tp_config->transfer_status = gtb_write_data(tp_config,tp_config->interface_mode, i2c_spi_tx_buf, 4);
            if (tp_config->transfer_status != HAL_OK)
            {
                send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                break;
            }

            // initial FSPI
            fspi_init(tp_config);

            // wakeup flash
            fspi_cs_enable(tp_config,true);
            i2c_spi_long_packet_tx_buffer[0] = 0xab;
            fspi_write(tp_config,i2c_spi_long_packet_tx_buffer, 1);
            fspi_cs_enable(tp_config,false);
            // delay_ms(2);

            // block erase
            fspi_cs_enable(tp_config,true);
            i2c_spi_long_packet_tx_buffer[0] = 0x06;
            fspi_write(tp_config,i2c_spi_long_packet_tx_buffer, 1);
            fspi_cs_enable(tp_config,false);

            fspi_cs_enable(tp_config,true);
            i2c_spi_long_packet_tx_buffer[0] = 0x05;
            fspi_write(tp_config,i2c_spi_long_packet_tx_buffer, 1);
            if (fspi_flash_write_wait_for_enable_latch(tp_config) == false)
                break;

            // BE
            fspi_cs_enable(tp_config,true);
            i2c_spi_long_packet_tx_buffer[0] = 0xd8;
            i2c_spi_long_packet_tx_buffer[1] = 0;
            i2c_spi_long_packet_tx_buffer[2] = 0;
            i2c_spi_long_packet_tx_buffer[3] = 0;
            fspi_write(tp_config,i2c_spi_long_packet_tx_buffer, 4);
            fspi_cs_enable(tp_config,false);

            fspi_cs_enable(tp_config,true);
            i2c_spi_long_packet_tx_buffer[0] = 0x05;
            fspi_write(tp_config,i2c_spi_long_packet_tx_buffer, 1);
            if (fspi_flash_wait_for_write_end(tp_config,3000) == false)
            {
                send_usb_trans_status_fspi(I2C_MODE, output, 0, 0, com_mode);
                break;
            }
            send_usb_trans_status_fspi(I2C_MODE, output, 0, 1, com_mode);

            // delay_ms(50);

            // program
            tp_config->fw_data_len = tp_config->fw_len / prog_unit;
            tp_config->fw_data_len1 = tp_config->fw_len % prog_unit;
            for (uint16_t i = 0; i < tp_config->fw_data_len; i++)
            {
                tp_config->flash_address = i * prog_unit;
                if (tp_config->flash_address > 0xffff)
                    break;

                // WREN
                fspi_cs_enable(tp_config,true);
                i2c_spi_long_packet_tx_buffer[0] = 0x06;
                fspi_write(tp_config,i2c_spi_long_packet_tx_buffer, 1);
                fspi_cs_enable(tp_config,false);

                fspi_cs_enable(tp_config,true);
                i2c_spi_long_packet_tx_buffer[0] = 0x05;
                fspi_write(tp_config,i2c_spi_long_packet_tx_buffer, 1);

                if (fspi_flash_write_wait_for_enable_latch(tp_config) == false)
                    break;

                // GTBSpiFlashRead(tp_config->flash_address, i2c_spi_long_packet_rx_buffer, prog_unit);
                // PP
                fspi_cs_enable(tp_config,true);
                i2c_spi_long_packet_tx_buffer[0] = 0x02;
                i2c_spi_long_packet_tx_buffer[1] = tp_config->flash_address >> 16;
                i2c_spi_long_packet_tx_buffer[2] = tp_config->flash_address >> 8;
                i2c_spi_long_packet_tx_buffer[3] = tp_config->flash_address;
                for (uint16_t j = 0; j < prog_unit; j++)
                {
                    i2c_spi_long_packet_tx_buffer[4 + j] = i2c_spi_long_packet_rx_buffer[j];
                }
                fspi_write(tp_config,i2c_spi_long_packet_tx_buffer, 4 + prog_unit);
                fspi_cs_enable(tp_config,false);

                fspi_cs_enable(tp_config,true);
                i2c_spi_long_packet_tx_buffer[0] = 0x05;
                fspi_write(tp_config,i2c_spi_long_packet_tx_buffer, 1);
                if (fspi_flash_wait_for_write_end(tp_config,5) == false)
                {
                    send_usb_trans_status_fspi(I2C_MODE, output, i, 0, com_mode);
                    break;
                }
                send_usb_trans_status_fspi(I2C_MODE, output, i, 2, com_mode);
                // delay_ms(5);
            }
            if (tp_config->fw_data_len1 != 0)
            {
                tp_config->flash_address = tp_config->fw_data_len * prog_unit;
                // WREN
                fspi_cs_enable(tp_config,true);
                i2c_spi_long_packet_tx_buffer[0] = 0x06;
                fspi_write(tp_config,i2c_spi_long_packet_tx_buffer, 1);
                fspi_cs_enable(tp_config,false);

                fspi_cs_enable(tp_config,true);
                i2c_spi_long_packet_tx_buffer[0] = 0x05;
                fspi_write(tp_config,i2c_spi_long_packet_tx_buffer, 1);
                if (fspi_flash_write_wait_for_enable_latch(tp_config) == false)
                    break;

                // GTBSpiFlashRead(tp_config->flash_address, i2c_spi_long_packet_rx_buffer, len1);
                // PP
                fspi_cs_enable(tp_config,true);
                i2c_spi_long_packet_tx_buffer[0] = 0x02;
                i2c_spi_long_packet_tx_buffer[1] = tp_config->flash_address >> 16;
                i2c_spi_long_packet_tx_buffer[2] = tp_config->flash_address >> 8;
                i2c_spi_long_packet_tx_buffer[3] = tp_config->flash_address;
                for (int j = 0; j < tp_config->fw_data_len1; j++)
                {
                    i2c_spi_long_packet_tx_buffer[4 + j] = i2c_spi_long_packet_rx_buffer[j];
                }
                fspi_write(tp_config,i2c_spi_long_packet_tx_buffer, 4 + tp_config->fw_data_len1);
                fspi_cs_enable(tp_config,false);

                fspi_cs_enable(tp_config,true);
                i2c_spi_long_packet_tx_buffer[0] = 0x05;
                fspi_write(tp_config,i2c_spi_long_packet_tx_buffer, 1);
                if (fspi_flash_wait_for_write_end(tp_config,5) == false)
                {
                    send_usb_trans_status_fspi(I2C_MODE, output, tp_config->fw_data_len, 0, com_mode);
                    break;
                }
                send_usb_trans_status_fspi(I2C_MODE, output, tp_config->fw_data_len, 2, com_mode);
            }
            // delay_ms(2);
            break;
        default:
            break;
    }

    // program flash(EX,OB,OF)
    if ((arg[1] == CMD_FLASH_OPERATION_START) || (arg[2] == CMD_FLASH_PROGRAM_START) ||
        (arg[2] == CMD_FLASH_READ_START))
        tp_config->flash_program_flash = true;
    else
        tp_config->flash_program_flash = false;

    if (tp_config->flash_program_flash == true)
    {
        switch (arg[2])
        {
            case CMD_FLASH_ID_START:
                switch (arg[3])
                {
                    case CMD_FLASH_EX: // EX-flash
                        tp_config->flash_id = drv_spi_flash_read_id();
                        break;
                    case CMD_FLASH_OB: // OB-flash
                        // tp_config->flash_id = GTBSpiFlashReadId();
                        break;
                    case CMD_FLASH_OF_SPI: // OF-flash(SPI)
                    case CMD_FLASH_OF_I2C: // OF-flash(I2C)
                        if (arg[3] == CMD_FLASH_OF_SPI)
                            tp_config->interface_mode = SPI_MODE;
                        else
                            tp_config->interface_mode = I2C_MODE;

                        tp_config->transfer_status = gtb_write_data(tp_config,tp_config->interface_mode, arg, 8);
                        if (tp_config->transfer_status != HAL_OK)
                        {
                            send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                            break;
                        }

                        // waiting for read ID done
                        ex_ti_initial(tp_config,ENABLE);
                        tp_config->int_flag = false;
                        tp_config->retry_count = 0;
                        while (tp_config->int_flag == false)
                        {
                            if (tp_config->retry_count < CMD_FLASH_PROGRAM_READ_TIMEOUT)
                                tp_config->retry_count++;
                            else
                                break;
                            // delay_us(8);
                        }
                        if (tp_config->int_flag == true)
                        {
                            // delay_us(20);
                            tp_config->int_flag = false;
                            tp_config->transfer_status = gtb_read_data(tp_config,tp_config->interface_mode, pCmd, data1, 11);
                            if (tp_config->transfer_status == HAL_OK)
                            {
                                if ((data1[0] == CMD_I_AM_GTB) && (data1[1] == CMD_FLASH_OPERATION_START) &&
                                    (data1[2] == CMD_FLASH_ID_END))
                                {
                                    tp_config->flash_id = (data1[8] << 16) | (data1[9] << 8) | data1[10];
                                }
                                else
                                {
                                    send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                                    break;
                                }
                            }
                            else
                            {
                                send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                                break;
                            }
                        }
                        else
                            send_error_code(tp_config->interface_mode, output, 3, com_mode); // timeout
                        break;
                    default:
                        break;
                }
                output[0] = arg[0];
                output[1] = arg[1];
                output[2] = CMD_FLASH_ID_END;
                output[8] = (uint8_t)((tp_config->flash_id >> 16) & 0xff);
                output[9] = (uint8_t)((tp_config->flash_id >> 8) & 0xff);
                output[10] = (uint8_t)(tp_config->flash_id & 0xff);
                gtb_fs_transmit(output, 64, com_mode);
                break;
            case CMD_FLASH_ERASE_START:
                output[0] = arg[0];
                output[1] = arg[1];
                output[2] = CMD_FLASH_ERASE_END;
                output[3] = arg[3];

                if (arg[4] > 31)
                    arg[4] = 31;
                output[7] = 0;
                switch (arg[3])
                {
                    case CMD_FLASH_EX: // EX-flash
                        output[7] = drv_spi_flash_block_64_erase(arg[4]);
                        break;
                    case CMD_FLASH_OB: // OB-flash
                        // output[7] = GTBSpiFlashBlock64Erase(arg[4]);
                        break;
                    case CMD_FLASH_OF_SPI: // OF-flash(SPI)
                    case CMD_FLASH_OF_I2C: // OF-flash(I2C)
                        if (arg[3] == CMD_FLASH_OF_SPI)
                            tp_config->interface_mode = SPI_MODE;
                        else
                            tp_config->interface_mode = I2C_MODE;

                        ex_ti_initial(tp_config,ENABLE);
                        tp_config->int_flag = false;
                        tp_config->transfer_status = gtb_write_data(tp_config,tp_config->interface_mode, arg, 8);
                        if (tp_config->transfer_status != HAL_OK)
                        {
                            send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                            break;
                        }
                        // waiting for erase done
                        tp_config->retry_count = 0;
                        while (tp_config->int_flag == false)
                        {
                            if (tp_config->retry_count < CMD_FLASH_ERASE_TIMEOUT)
                                tp_config->retry_count++;
                            else
                                break;
                            // delay_us(100);
                        }
                        if (tp_config->int_flag == true)
                        {
                            // delay_us(100);
                            tp_config->int_flag = false;
                            tp_config->transfer_status = gtb_read_data(tp_config,tp_config->interface_mode, pCmd, data1, 8);
                            if (tp_config->transfer_status == HAL_OK)
                            {
                                if ((data1[0] == CMD_I_AM_GTB) && (data1[1] == CMD_FLASH_OPERATION_START) &&
                                    (data1[2] == CMD_FLASH_ERASE_END) && (data1[7] == 1))
                                    output[7] = 1;
                            }
                            else
                            {
                                send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                                break;
                            }
                        }
                        else
                            send_error_code(tp_config->interface_mode, output, 3, com_mode); // timeout
                        break;
                    default:
                        break;
                }
                gtb_fs_transmit(output, 64, com_mode);
                tp_config->test_count = 0;
                break;
            case CMD_FLASH_PROGRAM_START:
                tp_config->flash_address = arg[1] << 16;
                tp_config->flash_address |= arg[4] << 8;
                tp_config->flash_address |= arg[5];
                tp_config->flash_data_len = arg[6] << 8;
                tp_config->flash_data_len |= arg[7];

                if (tp_config->flash_data_len > 56) // in case of data length overflow
                    break;

                if (tp_config->flash_address > 0x7fffff) // in case of address overflow
                    break;

                for (int i = 0; i < tp_config->flash_data_len; i++)
                {
                    flash_buffer[i] = arg[8 + i];
                }

                tp_config->interface_mode = SPI_MODE;
                tp_config->flash_ret = CMD_USB_TRANSFER_FAIL;
                switch (arg[3])
                {
                    case CMD_FLASH_EX: // EX-flash
                        tp_config->flash_ret = drv_spi_flash_write(tp_config->flash_address, flash_buffer, tp_config->flash_data_len);
                        break;
                    case CMD_FLASH_OB: // OB-flash
                        // tp_config->flash_ret =  GTBSpiFlashWrite(tp_config->flash_address, flash_buffer, tp_config->flash_data_len);
                        break;
                    case CMD_FLASH_OF_SPI: // OF-flash(SPI)
                    case CMD_FLASH_OF_I2C: // OF-flash(I2C)
                        if (arg[3] == CMD_FLASH_OF_SPI)
                            tp_config->interface_mode = SPI_MODE;
                        else
                            tp_config->interface_mode = I2C_MODE;

                        ex_ti_initial(tp_config,ENABLE);
                        tp_config->int_flag = false;
                        tp_config->transfer_status = gtb_write_data(tp_config,tp_config->interface_mode, arg, 64);
                        if (tp_config->transfer_status != HAL_OK)
                        {
                            send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                            break;
                        }

                        // waiting for block program done
                        tp_config->retry_count = 0;
                        while (tp_config->int_flag == false)
                        {
                            if (tp_config->retry_count < CMD_FLASH_PROGRAM_READ_TIMEOUT)
                                tp_config->retry_count++;
                            else
                                break;
                            // delay_us(7);
                        }
                        if (tp_config->int_flag == true)
                        {
                            // delay_us(10);
                            tp_config->int_flag = false;
                            tp_config->transfer_status = gtb_read_data(tp_config,tp_config->interface_mode, pCmd, data1, 8);
                            if (tp_config->transfer_status == HAL_OK)
                            {
                                if ((data1[0] == CMD_I_AM_GTB) && (data1[1] == CMD_USB_TRANSFER_STATUS) &&
                                    (data1[3] == CMD_USB_TRANSFER_NEXT))
                                    tp_config->flash_ret = CMD_USB_TRANSFER_OK;
                            }
                        }
                        break;
                    default:
                        break;
                }
                if (tp_config->flash_ret == 0)
                {
                    send_error_code(tp_config->interface_mode, output, 1, com_mode);
                }
                else
                {
                    send_usb_trans_status(tp_config->interface_mode, output,
                                          CMD_USB_TRANSFER_NEXT, com_mode); // tell Tool to send next flash data
                }
                //                        if (tp_config->interface_mode == SPI_MODE)
                //                            LED_toggle(LED_SPI);
                //                        else
                //                            LED_toggle(LED_I2C);
                break;
            case CMD_FLASH_READ_START:
                tp_config->flash_address = arg[1] << 16;
                tp_config->flash_address |= arg[4] << 8;
                tp_config->flash_address |= arg[5];
                tp_config->flash_data_len = arg[6] << 8;
                tp_config->flash_data_len |= arg[7];

                if (tp_config->flash_data_len > 56) // in case of data length overflow
                    break;

                if (tp_config->flash_address > 0x7fffff) // in case of address overflow
                    break;

                tp_config->flash_ret = CMD_FLASH_READ_FAIL;
                switch (arg[3])
                {
                    case CMD_FLASH_EX: // EX-flash
                        drv_spi_flash_read(tp_config->flash_address, flash_buffer, tp_config->flash_data_len);
                        tp_config->flash_ret = CMD_FLASH_READ_OK;
                        break;
                    case CMD_FLASH_OB: // OB-flash
                        // GTBSpiFlashRead(tp_config->flash_address, flash_buffer, tp_config->flash_data_len);
                        tp_config->flash_ret = CMD_FLASH_READ_OK;
                        break;
                    case CMD_FLASH_OF_SPI: // OF-flash(SPI)
                    case CMD_FLASH_OF_I2C: // OF-flash(I2C)
                        if (arg[3] == CMD_FLASH_OF_SPI)
                            tp_config->interface_mode = SPI_MODE;
                        else
                            tp_config->interface_mode = I2C_MODE;

                        tp_config->int_flag = false;
                        tp_config->transfer_status = gtb_write_data(tp_config,tp_config->interface_mode, arg, 8);
                        if (tp_config->transfer_status != HAL_OK)
                        {
                            send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                            break;
                        }

                        // waiting for block read done
                        tp_config->retry_count = 0;
                        while (tp_config->int_flag == false)
                        {
                            if (tp_config->retry_count < CMD_FLASH_PROGRAM_READ_TIMEOUT)
                                tp_config->retry_count++;
                            else
                                break;
                            // delay_us(7);
                        }
                        if (tp_config->int_flag == true)
                        {
                            // delay_us(10);
                            tp_config->int_flag = false;
                            tp_config->transfer_status = gtb_read_data(tp_config,tp_config->interface_mode, pCmd, data1, 64);
                            if (tp_config->transfer_status == HAL_OK)
                            {
                                if ((data1[0] == CMD_I_AM_GTB) && (data1[2] == CMD_FLASH_READ_END))
                                {
                                    if (data1[3] == CMD_FLASH_READ_OK)
                                    {
                                        for (int i = 0; i < tp_config->flash_data_len; i++)
                                        {
                                            flash_buffer[i] = data1[i + 8];
                                        }
                                        tp_config->flash_ret = CMD_FLASH_READ_OK;
                                    }
                                }
                            }
                            else
                            {
                                send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                                break;
                            }
                        }
                        break;
                    default:
                        break;
                }

                for (int i = 0; i < 8; i++)
                {
                    output[i] = arg[i];
                }
                output[2] = CMD_FLASH_READ_END;
                output[3] = tp_config->flash_ret;

                // send back result
                for (int i = 0; i < tp_config->flash_data_len; i++)
                {
                    output[i + 8] = flash_buffer[i];
                }
                gtb_fs_transmit(output, 64, com_mode);
                //                        if (tp_config->interface_mode == SPI_MODE)
                //                            LED_toggle(LED_SPI);
                //                        else
                //                            LED_toggle(LED_I2C);
                break;
            default:
                break;
        }
    }

}

void gtb_fw_mode_com(tp_config_t *tp_config,uint8_t *arg, uint8_t *output, uint8_t com_mode)
{
    if ((tp_config->download_enable == true) && (tp_config->int_flag == true))
    {
        // delay_us(10);
        tp_config->transfer_flag = true;
        if (tp_config->interface_mode == SPI_MODE)
        {
            tp_spi_cs_enable(true);
            tp_config->transfer_status = spi_read_write_data1( &tp_config->fw_spi_data_out,&tp_config->fw_report_cmd, 1, 0);
            tp_spi_cs_enable(false);
            if (tp_config->transfer_status != HAL_OK)
            {
                send_error_code(SPI_MODE, output, tp_config->transfer_status, com_mode);
                tp_config->int_flag = false;
                // continue;
            }
        }
        else
        {
            tp_config->i2c_slave_address = 0x26;
            //tp_config->i2c_slave_address = DEFAULT_SLAVE_ADDRESS; // i2c address = 0x26
            tp_config->transfer_status = gtb_read_data(tp_config,I2C_MODE, pCmd, &tp_config->fw_report_cmd, 1);
            if (tp_config->transfer_status != HAL_OK)
            {
                send_error_code(I2C_MODE, output, tp_config->transfer_status, com_mode);
                tp_config->int_flag = false;
                // continue;
            }
        }
        if (tp_config->fw_report_cmd == 0xb1)
        {
            tp_config->download_complete = true;
            output[0] = CMD_I_AM_GTB;
            if (tp_config->interface_mode == SPI_MODE)
            {
                output[1] = CMD_SPI_TRANSFER;
                tp_config->one_cs_for_host_download_enable = true;
            }
            else
                output[1] = CMD_I2C_TRANSFER;
            output[2] = CMD_HOSTDOWNLOAD_WAIT_DATA; // tell GTB Tool to send SPI or I2C data
            // delay_ms(2);//waiting for 0xb1 reading end
            gtb_fs_transmit(output, 64, com_mode);
        }
        else
        {
            if (tp_config->download_complete == true)
            {
                if ((tp_config->fw_report_cmd == 0xb3) || (tp_config->fw_report_cmd == 0xb2))
                {
                    tp_config->download_complete = false;
                    tp_config->download_enable = false;
                    tp_config->offline_mode = 0;
                    tp_config->fw_len = 0;
                    ex_ti_initial(tp_config,DISABLE);
                    tp_config->ex_ti_flag = false;
                    if (tp_config->fw_report_cmd == 0xb3) // ok
                    {
                        //                        if(tp_config->interface_mode == SPI_MODE)
                        //                            LED_On(LED_SPI,true);
                        //                        else
                        //                            LED_On(LED_I2C,true);

                        output[3] = CMD_USB_TRANSFER_OK;
                    }
                    else // fail
                    {
                        output[3] = CMD_USB_TRANSFER_FAIL;
                    }

                    output[0] = CMD_I_AM_GTB;
                    output[1] = CMD_USB_TRANSFER_STATUS;
                    if (tp_config->interface_mode == SPI_MODE)
                        output[2] = CMD_SPI_TRANSFER;
                    else
                        output[2] = CMD_I2C_TRANSFER;
                    gtb_fs_transmit(output, 64, com_mode);
                }
                else // in case of transfer error
                {
                    output[0] = CMD_I_AM_GTB;
                    output[1] = CMD_USB_TRANSFER_STATUS;
                    if (tp_config->interface_mode == SPI_MODE)
                        output[2] = CMD_SPI_TRANSFER;
                    else
                        output[2] = CMD_I2C_TRANSFER;
                    output[3] = CMD_USB_TRANSFER_FAIL; // tell GTB Tool download fail

                    gtb_fs_transmit(output, 64, com_mode);
                }
            }
            else
            {
                tp_config->download_enable = false;
                output[0] = CMD_I_AM_GTB;
                output[1] = CMD_USB_TRANSFER_STATUS;
                if (tp_config->interface_mode == SPI_MODE)
                    output[2] = CMD_SPI_TRANSFER;
                else
                    output[2] = CMD_I2C_TRANSFER;
                output[3] = CMD_USB_TRANSFER_FAIL; // tell GTB Tool download fail
                gtb_fs_transmit(output, 64, com_mode);
            }
        }
        tp_config->int_flag = false;
        tp_config->transfer_flag = false;
    }
    
    // for demo,rawdata or debug mode
    switch (tp_config->fw_mode)
    {
        case FWMODE_DEMO:
            if (tp_config->int_flag == true)
            {
                //printf("fw_mode: %d\r\n", tp_config->fw_mode);
                //printf("---- 1 ---- \r\n");
                tp_config->int_flag = false;
                tp_config->transfer_flag = true;
                // delay_us(50);
                tp_config->transfer_status = gtb_read_coordination_dynamic(tp_config,touch_data_demo_raw_data);
                if (tp_config->transfer_status != HAL_OK)
                {
                    //printf("demo read error\r\n");
                    send_error_code(tp_config->interface_mode, output, tp_config->transfer_status, com_mode);
                    break;
                }
                if ((touch_data_demo_raw_data[0] == 0) && (touch_data_demo_raw_data[1] == 0) && (touch_data_demo_raw_data[2] == 0) && (touch_data_demo_raw_data[3] == 0))
                    break;

                //if((touch_data_demo_raw_data[1] == 0xff)&&(touch_data_demo_raw_data[2] == 0xff))
                //{
                //    LED_On(LED_I2C,false);
                //}
                //else
                //{
                //    LED_On(LED_I2C,true);
                //}
                //printf("!! %d %d\r\n",tp_config->ic_type_index,tp_config->ipx_format);
                if ((tp_config->ic_type_index & 0x0f) == IC_TYPE_GC && tp_config->ipx_format == true)
                {
                    //printf("??0\r\n");
                    if (touch_data_demo_raw_data[0] == 0xef &&
                        touch_data_demo_raw_data[1] == 0xbf &&
                        touch_data_demo_raw_data[2] == 0xad &&
                        touch_data_demo_raw_data[3] == 0xde &&
                        touch_data_demo_raw_data[4] == 0x20)
                    {
                        //printf("??1\r\n");
                        uint8_t touch_Data_tmp[54];
                        touch_Data_tmp[0] = touch_data_demo_raw_data[36];
                        touch_Data_tmp[1] = touch_data_demo_raw_data[37];
                        touch_Data_tmp[2] = touch_data_demo_raw_data[44];
                        touch_Data_tmp[3] = touch_data_demo_raw_data[45];
                        for (int i = 0; i < 5; i++)
                        {
                            for (int j = 0; j < 6; j++)
                                touch_Data_tmp[i * 10 + 4 + j] = touch_data_demo_raw_data[46 + i * 30 + j];

                            touch_Data_tmp[i * 10 + 4 + 6] = touch_data_demo_raw_data[46 + i * 30 + 24];
                            touch_Data_tmp[i * 10 + 4 + 7] = touch_data_demo_raw_data[46 + i * 30 + 25];
                            touch_Data_tmp[i * 10 + 4 + 8] = touch_data_demo_raw_data[46 + i * 30 + 28];
                            touch_Data_tmp[i * 10 + 4 + 9] = touch_data_demo_raw_data[46 + i * 30 + 29];
                        }
                        for (int i = 0; i < 54; i++)
                        {
                            touch_data_demo_raw_data[i] = touch_Data_tmp[i];
                        }
                        tp_config->ic_touch_data_len = 54; // send 5 points packet
                    }
                    else
                        break;
                }
                else
                {
                    tp_config->ic_touch_data_len = gtb_get_demo_data_len(tp_config->ic_type_index);
                }
                output[0] = CMD_I_AM_GTB;
                if (tp_config->interface_mode == I2C_MODE)
                    output[1] = CMD_I2C_DYNAMIC_CONTROL;
                else
                    output[1] = CMD_SPI_DYNAMIC_CONTROL;
                output[2] = tp_config->fw_mode;
                output[3] = tp_config->ic_type_index;
                if (tp_config->ic_touch_data_len > 56)
                {
                    tp_config->fw_data_len = tp_config->ic_touch_data_len / 56;
                    tp_config->fw_data_len1 = tp_config->ic_touch_data_len % 56;

                    for (int j = 0; j < tp_config->fw_data_len; j++)
                    {
                        output[5] = j;
                        output[6] = 0;
                        output[7] = 56;
                        for (int i = 0; i < 56; i++)
                        {
                            output[i + 8] = touch_data_demo_raw_data[i + j * 56];
                        }
                        //printf("a\r\n");
                        gtb_fs_transmit(output, 64, com_mode);
                        osDelay(2);
                        // delay_us(500);
                    }
                    if (tp_config->fw_data_len1 != 0)
                    {
                        output[5] = tp_config->fw_data_len;
                        output[6] = tp_config->fw_data_len1 >> 8;
                        output[7] = tp_config->fw_data_len1 & 0xff;
                        for (int i = 0; i < tp_config->fw_data_len1; i++)
                        {
                            output[i + 8] = touch_data_demo_raw_data[i + tp_config->fw_data_len * 56];
                        }
                        //printf("b\r\n");
                        gtb_fs_transmit(output, 64, com_mode);
                    }
                }
                else
                {
                    output[5] = 0;
                    output[6] = 0;
                    output[7] = tp_config->ic_touch_data_len;
                    for (int i = 0; i < tp_config->ic_touch_data_len; i++)
                    {
                        output[i + 8] = touch_data_demo_raw_data[i];
                    }
                    //printf("c\r\n");
                    gtb_fs_transmit(output, 64, com_mode);
                }
                tp_config->transfer_flag = false;
            }
            break;
        case FWMODE_RAWDATA:

        case FWMODE_DEBUG:
            //printf("fw_mode: %d\r\n", tp_config->fw_mode);

            if ((tp_config->ic_type_index & 0x0f) == IC_TYPE_NT)
            {
                if (tp_config->raw_data_flag == true)
                    tp_config->int_flag = true;
            }
            //printf("tp_config->int_flag: %d\r\n", tp_config->int_flag);
            if (tp_config->int_flag == true)
            {
                if (tp_config->ic_touch_data_len != 0) // prevent error int
                {
                    //printf("tp_config->ic_touch_data_len: %d\r\n", tp_config->ic_touch_data_len);
                    tp_config->transfer_flag = true;
                    // delay_us(100);
                    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
                    //printf("??\r\n");
                    
                    tp_config->transfer_status = gtb_read_raw_data(tp_config,tp_config->interface_mode, tp_config->ic_type_index, tp_config->ic_touch_data_len1);
                    //printf("tp_config->transfer_status: %d\r\n", tp_config->transfer_status);
                    if (tp_config->transfer_status != HAL_OK)
                    {
                        //printf("raw/debug read error\r\n");
                        tp_config->int_flag = false;
                        tp_config->transfer_flag = false;
                        break;
                    }
                    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
                    if ((tp_config->fw_mode == FWMODE_DEBUG) && ((tp_config->ic_type_index & 0x0f) == IC_TYPE_GC) && (tp_config->ipx_format == true))
                    {
                        if ((touch_data_demo_raw_data[0] = 0xef) &&
                            (touch_data_demo_raw_data[1] = 0xbf) &&
                            (touch_data_demo_raw_data[2] = 0xad) &&
                            (touch_data_demo_raw_data[3] = 0xde) &&
                            (touch_data_demo_raw_data[4] = 0x20))
                        {
                            uint8_t touch_Data_tmp[54];
                            touch_Data_tmp[0] = touch_data_demo_raw_data[36];
                            touch_Data_tmp[1] = touch_data_demo_raw_data[37];
                            touch_Data_tmp[2] = touch_data_demo_raw_data[44];
                            touch_Data_tmp[3] = touch_data_demo_raw_data[45];
                            tp_config->ic_touch_data_index = 516 - 54;
                            for (int i = 0; i < 5; i++)
                            {
                                for (int j = 0; j < 6; j++)
                                    touch_Data_tmp[i * 10 + 4 + j] = touch_data_demo_raw_data[46 + i * 30 + j];

                                touch_Data_tmp[i * 10 + 4 + 6] = touch_data_demo_raw_data[46 + i * 30 + 24];
                                touch_Data_tmp[i * 10 + 4 + 7] = touch_data_demo_raw_data[46 + i * 30 + 25];
                                touch_Data_tmp[i * 10 + 4 + 8] = touch_data_demo_raw_data[46 + i * 30 + 28];
                                touch_Data_tmp[i * 10 + 4 + 9] = touch_data_demo_raw_data[46 + i * 30 + 29];
                            }
                            for (int i = 0; i < 54; i++)
                            {
                                touch_data_demo_raw_data[i + tp_config->ic_touch_data_index] = touch_Data_tmp[i];
                            }
                            tp_config->ic_touch_data_len = tp_config->ic_touch_data_len1 - tp_config->ic_touch_data_index;
                        }
                        else{
                            printf("error\r\n");
                            break;
                        }
                    }
                    else
                    {
                        tp_config->ic_touch_data_index = gtb_get_debug_data_index(tp_config->ic_type_index);
                    }
                    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET);
                    output[0] = 0x40;
                    if (tp_config->interface_mode == I2C_MODE)
                        output[1] = CMD_I2C_DYNAMIC_CONTROL;
                    else
                        output[1] = CMD_SPI_DYNAMIC_CONTROL;

                    tp_config->fw_data_len = tp_config->ic_touch_data_len / 56;
                    tp_config->fw_data_len1 = tp_config->ic_touch_data_len % 56;
                    output[2] = tp_config->fw_mode;
                    output[3] = tp_config->ic_type_index;
                    //printf("tp_config->fw_data_len=%d, tp_config->fw_data_len1=%d\r\n",tp_config->fw_data_len,tp_config->fw_data_len1);
                    if (com_mode == GTB_HID)
                    {
                        for (int j = 0; j < tp_config->fw_data_len; j++)
                        {
                            output[5] = j;
                            output[6] = 0;
                            output[7] = 56;
                            for (int i = 0; i < 56; i++)
                            {
                                output[8 + i] = touch_data_demo_raw_data[j * 56 + i + tp_config->ic_touch_data_index];
                            }
                            
                            gtb_fs_transmit(output, 64, com_mode);
                            //printf("222\r\n");
                            //printf("j=%d\r\n", j);
                            osDelay(1);

                            if ((j != (tp_config->fw_data_len - 1)) || (tp_config->fw_data_len1 != 0))
                            {
                                osDelay(1 + tp_config->raw_data_delay_time);
                                //	                            delay_us(200);
                            }
                        }
                        // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
                        if (tp_config->fw_data_len1 != 0)
                        {
                            output[5] = tp_config->fw_data_len;
                            output[6] = 0;
                            output[7] = tp_config->fw_data_len1;
                            for (int i = 0; i < tp_config->fw_data_len1; i++)
                            {
                                output[8 + i] = touch_data_demo_raw_data[tp_config->fw_data_len * 56 + i + tp_config->ic_touch_data_index];
                            }
                            for (int i = tp_config->fw_data_len1; i < 56; i++)
                            {
                                output[8 + i] = 0xcc;
                            }
                            gtb_fs_transmit(output, 64, com_mode);
                        }
                        // osDelay(1);
                        // osDelay(8 + tp_config->raw_data_delay_time); 			//time waiting for tool UI
                        //	                    if(tp_config->interface_mode == I2C_MODE)
                        //	                    {
                        //	                        LED_toggle(LED_I2C);
                        //	                    }
                        //	                    else
                        //	                    {
                        //	                        LED_toggle(LED_SPI);
                        //	                    }
                    }
                    else if (com_mode == GTB_CDC)
                    {
                        output[5] = 0x00;
                        output[6] = tp_config->ic_touch_data_len >> 8;
                        output[7] = tp_config->ic_touch_data_len & 0xff;
                        memcpy(&output[8], &touch_data_demo_raw_data[tp_config->ic_touch_data_index], tp_config->ic_touch_data_len);
                        //gtb_fs_transmit(output, tp_config->ic_touch_data_len+8, com_mode);
                        gtb_fs_transmit(output, 1152+8, com_mode);
                        osDelay(1);
                    }
                    else if(com_mode == GTB_MIX){

                        output[5] = 0x00;
                        output[6] = tp_config->ic_touch_data_len >> 8;
                        output[7] = tp_config->ic_touch_data_len & 0xff;
                        memcpy(&output[8], &touch_data_demo_raw_data[tp_config->ic_touch_data_index], tp_config->ic_touch_data_len);
                        //HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_13, GPIO_PIN_SET);
                        //USBD_CDC_ACM_WriteData(0, output,  tp_config->ic_touch_data_len+8);
                        //HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_13, GPIO_PIN_RESET);
                        osDelay(1);
                    }
                    tp_config->transfer_flag = false;
                }
                tp_config->int_flag = false;
            }
            break;
        default:
            break;
    }
}
