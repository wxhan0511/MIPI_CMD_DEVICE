//
// Created by Bobby on 2023/8/9.
//


#include <stdlib.h>
#include "gtb_flash_op.h"
#include "bsp_gtb.h"
#include "tp_define.h"
uint8_t flash_buffer[MAXUSBPACKETSIZE - 8];
extern void send_error_code(tp_config_t *tp_config,bool mode,int32_t error_code);
extern uint8_t i2c_spi_tx_buf[MAXUSBPACKETSIZE];
extern uint8_t i2c_spi_rx_buf[MAXUSBPACKETSIZE];
extern uint8_t i2c_spi_long_packet_tx_buffer[MAXI2CSPIBUFFERSIZE];
extern uint8_t i2c_spi_long_packet_rx_buffer[MAXI2CSPIBUFFERSIZE];
//FSPI operation for 7271
bool fspi_init(tp_config_t *tp_config)
{
    i2c_spi_tx_buf[0] = 0x95;
    i2c_spi_tx_buf[1] = 0x27;
    i2c_spi_tx_buf[2] = 0x05;
    i2c_spi_tx_buf[3] = 0xFA;
    i2c_spi_tx_buf[4] = 0x40;
    i2c_spi_tx_buf[5] = 0x00;
    i2c_spi_tx_buf[6] = 0x01;
    i2c_spi_tx_buf[7] = 0xC4;
    i2c_spi_tx_buf[8] = 0x5E;
    tp_config->transfer_status = gtb_write_data(tp_config,tp_config->spi_i2c_count,i2c_spi_tx_buf,9);
    if(tp_config->transfer_status != HAL_OK)
    {
        send_error_code(tp_config,tp_config->spi_i2c_count,tp_config->transfer_status);
        return false;
    }

    i2c_spi_tx_buf[0] = 0x95;
    i2c_spi_tx_buf[1] = 0x27;
    i2c_spi_tx_buf[2] = 0x05;
    i2c_spi_tx_buf[3] = 0xFA;
    i2c_spi_tx_buf[4] = 0x40;
    i2c_spi_tx_buf[5] = 0x00;
    i2c_spi_tx_buf[6] = 0x01;
    i2c_spi_tx_buf[7] = 0xCC;
    i2c_spi_tx_buf[8] = 0x31;
    tp_config->transfer_status = gtb_write_data(tp_config,tp_config->spi_i2c_count,i2c_spi_tx_buf,9);
    if(tp_config->transfer_status != HAL_OK)
    {
        send_error_code(tp_config,tp_config->spi_i2c_count,tp_config->transfer_status);
        return false;
    }

    return true;
}

bool fspi_cs_enable(tp_config_t *tp_config,bool en)
{
    i2c_spi_tx_buf[0] = 0x95;
    i2c_spi_tx_buf[1] = 0x27;
    i2c_spi_tx_buf[2] = 0x05;
    i2c_spi_tx_buf[3] = 0xFA;
    i2c_spi_tx_buf[4] = 0x40;
    i2c_spi_tx_buf[5] = 0x00;
    i2c_spi_tx_buf[6] = 0x01;
    i2c_spi_tx_buf[7] = 0xCC;
    if(en == true)
        i2c_spi_tx_buf[8] = 0x30;
    else
        i2c_spi_tx_buf[8] = 0x31;
    tp_config->transfer_status = gtb_write_data(tp_config,tp_config->spi_i2c_count,i2c_spi_tx_buf,9);
    if(tp_config->transfer_status != HAL_OK)
    {
        send_error_code(tp_config,tp_config->spi_i2c_count,tp_config->transfer_status);
        return false;
    }

    return true;
}

//========================flash operation===================================//
//Functions for external flash
#define DUMMY_BYTE                  0xFF
#define BUSY                        0x1
#define DMA_ENABLE									false
//#define WAIT_FOR_SPI_TRY_COUNT			if(retry_count < RETRY_TIMEOUT) {retry_count++;	}else break;}

uint8_t * pData_out,*pData_in;
uint32_t trycount;
void drv_flash_wait_for_write_end( void )
{
    uint8_t wip = 0;
    trycount = 0;
    uint16_t speed = SPI2->CR1;
    speed >>= 3;
    speed &= 0x7;
    double time_out = 3000000.0 / (4.1 + (8.0 * (speed + 1)) / 18.0);
//    debug(speed);
//    debug16((uint32_t)time_out >> 16);
//    debug16((uint32_t)time_out & 0xffff);
    tp_spi_cs_enable(true);
    tp_spi_read_write_byte( 0x05 );   // Read Status Register
    do
    {
        if(trycount < time_out)
            trycount++;
        else
            break;
        wip = ( tp_spi_read_write_byte( DUMMY_BYTE) & 0x01 );
    }while( ( wip ) == BUSY);
    tp_spi_cs_enable(false);
}

void drv_flash_wait_for_write_enable_latch( void )
{
    uint8_t wel = 0;

    do
    {
        tp_spi_cs_enable(true);
        tp_spi_read_write_byte( 0x06 );
        tp_spi_cs_enable(false);
        tp_spi_cs_enable(true);
        tp_spi_read_write_byte( 0x05 );
        wel = ( tp_spi_read_write_byte( DUMMY_BYTE) & 0x02 );
        tp_spi_cs_enable(false);
    }while( ( wel ) != 0x02);
}

uint8_t drv_flash_read_security_reg( void )
{
    uint8_t f_fail = 0;

    tp_spi_cs_enable(true);
    tp_spi_read_write_byte( 0x05 );
    tp_spi_read_write_byte( 0xff );
    f_fail = tp_spi_read_write_byte( 0x2b );
    tp_spi_cs_enable(false);

    return f_fail;
}

void drv_flash_write_enable( void )
{
    tp_spi_cs_enable(true);
    tp_spi_read_write_byte( 0x06 );
    tp_spi_cs_enable(false);
}

uint8_t drv_flash_sector_erase( uint32_t sector_addr )
{
    drv_flash_wait_for_write_enable_latch();

    tp_spi_cs_enable(true);
    tp_spi_read_write_byte( 0x20 );
    tp_spi_read_write_byte( ( sector_addr & 0xFF0000 ) >> 16 );
    tp_spi_read_write_byte( ( sector_addr & 0xFF00 ) >> 8 );
    tp_spi_read_write_byte( sector_addr & 0xFF );
    tp_spi_cs_enable(false);
    drv_flash_wait_for_write_end();

    if( (drv_flash_read_security_reg() & 0x20) != 0)
        return 0;
    else
        return 1;
}

uint8_t drv_spi_flash_block_64_erase( uint32_t block_addr )
{
    drv_flash_wait_for_write_enable_latch();

    tp_spi_cs_enable(true);
#if (DMA_ENABLE == true)
    pData_out = malloc(4);
		pData_in = malloc(4);
		pData_out[0] = 0xD8;
		pData_out[1] = ( blockAddr & 0xFF0000 ) >> 16;
		pData_out[2] = ( blockAddr & 0xFF00 ) >> 8;
		pData_out[3] = blockAddr & 0xFF;
		SPI2_ReadWriteData(pData_out,pData_in,4);
		free(pData_out);
		free(pData_in);
    tp_spi_cs_enable(false);
    drv_flash_wait_for_write_end();
#else
    tp_spi_read_write_byte( 0xD8 );
    tp_spi_read_write_byte( ( block_addr & 0xFF0000 ) >> 16 );
    tp_spi_read_write_byte( ( block_addr & 0xFF00 ) >> 8 );
    tp_spi_read_write_byte( block_addr & 0xFF );
    tp_spi_cs_enable(false);
    drv_flash_wait_for_write_end();
#endif

    if( (drv_flash_read_security_reg() & 0x20) != 0)
        return 0;
    else
        return 1;
}

uint8_t drv_spi_flash_page_write( uint32_t write_addr, uint8_t* write_buffer, uint32_t num )
{
    drv_flash_wait_for_write_enable_latch();

    tp_spi_cs_enable(true);
    tp_spi_read_write_byte( 0x02 );
    tp_spi_read_write_byte( ( write_addr & 0xFF0000 ) >> 16 );
    tp_spi_read_write_byte( ( write_addr & 0xFF00 ) >> 8 );
    tp_spi_read_write_byte( write_addr & 0xFF );

    while( num-- )
    {
        tp_spi_read_write_byte( *write_buffer++ );
    }

    tp_spi_cs_enable(false);
    drv_flash_wait_for_write_end();

    if( (drv_flash_read_security_reg() & 0x20) != 0)
        return 0;
    else
        return 1;
}

//ok:2,fail:0
uint8_t drv_spi_flash_write(uint32_t write_addr, uint8_t* write_buffer, uint16_t num)
{
    uint16_t page_remain;

    page_remain = 256 - write_addr % 256; // the remaining space on the page where writing will start

    if( num <= page_remain )
    {
        page_remain = num;    // only one page to program.
    }

    while(1)
    {
        if(drv_spi_flash_page_write( write_addr, write_buffer, page_remain ) == 0)
            return CMD_USB_TRANSFER_FAIL;

        if( num == page_remain )
        {
            break;                      // writing on one page is over
        }
        else                            // numByteToWrite > page_remain
        {
            write_buffer += page_remain;
            write_addr += page_remain;
            num -= page_remain;       // subtract the part that has been written above

            if( num > 256 )
            {
                page_remain = 256;               // write another 256 bytes
            }
            else
            {
                page_remain = num;    // the last part to be written
            }
        }
    }

    return CMD_USB_TRANSFER_OK;
}

void drv_spi_flash_read( uint32_t addr, uint8_t *readBuffer, uint32_t numByteRead )
{
    tp_spi_cs_enable(true);
    tp_spi_read_write_byte( 0x0b) ;
    tp_spi_read_write_byte( ( addr & 0xFF0000 ) >> 16 );
    tp_spi_read_write_byte( ( addr & 0xFF00 ) >> 8 );
    tp_spi_read_write_byte( addr & 0xFF );
    tp_spi_read_write_byte( 0xFF );//if use 0x0b,send one dummy

    while( numByteRead-- )
    {
        *readBuffer++ = (uint8_t)( tp_spi_read_write_byte( DUMMY_BYTE ) & 0xFF );
    }

    tp_spi_cs_enable(false);
}

uint32_t drv_spi_flash_read_id( void )
{
    uint32_t id;

    tp_spi_cs_enable(true);

    uint8_t *pOut,*PIn;
    pOut = malloc(4);
    PIn = malloc(4);
    pOut[0] = 0x9f;
    pOut[1] = 0xff;
    pOut[2] = 0xff;
    pOut[3] = 0xff;
    spi_read_write_data1(pOut,PIn,4,0);
    id = (PIn[1] << 16) | (PIn[2] << 8) | PIn[3];
    //id = PIn[1];
    tp_spi_cs_enable(false);

    free(pOut);
    free(PIn);
    return id;
}

uint16_t drv_spi_flash_crc_16( unsigned char *pcDataStream, unsigned short sNumberOfDataBytes )
{
    unsigned short gsCRC16GenerationCode = 0x8408;
    unsigned short sByteCounter;
    unsigned char cBitCounter;
    unsigned char cCurrentData;
    unsigned short sCRC16Result = 0xFFFF;

    if ( sNumberOfDataBytes > 0 )
    {
        for ( sByteCounter = 0; sByteCounter < sNumberOfDataBytes; sByteCounter++ )
        {
            cCurrentData = *( pcDataStream + sByteCounter );

            for ( cBitCounter = 0; cBitCounter < 8; cBitCounter++ )
            {
                if ( ( ( sCRC16Result & 0x0001 ) ^ ( cCurrentData & 0x0001 ) ) > 0 )
                {
                    sCRC16Result = ( ( sCRC16Result >> 1 ) & 0x7FFF ) ^ gsCRC16GenerationCode;
                }
                else
                {
                    sCRC16Result = ( sCRC16Result >> 1 ) & 0x7FFF;
                }

                cCurrentData = (cCurrentData >> 1 ) & 0x7F;
            }
        }
    }

    return sCRC16Result;
}

void drv_spi_flash_power_down( void )
{
    tp_spi_cs_enable(true);
    tp_spi_read_write_byte( 0xB9) ;
    tp_spi_cs_enable(false);
    // according to MX specifications, minimum time is 10+30us
    // according to Zbit specifications, maximum time is 3us
    //(100);
}

void drv_spi_flash_power_wake_up( void )
{
    tp_spi_cs_enable(true);
    tp_spi_read_write_byte( 0xAB);    // according to MX specifications, minimum time is 20ns
    tp_spi_cs_enable(false);
    // according to MX specifications, minimum time is 35us
    // according to Zbit specifications, maximum time is 3us
    //delay_us(100);
}

bool fspi_addr_write(tp_config_t *tp_config,uint32_t addr,uint8_t *data,uint16_t length)
{
    i2c_spi_tx_buf[0] = 0x95;
    i2c_spi_tx_buf[1] = 0x27;
    i2c_spi_tx_buf[2] = 0x05;
    i2c_spi_tx_buf[3] = 0xFA;
    i2c_spi_tx_buf[4] = 0x40;
    i2c_spi_tx_buf[5] = addr >> 16;
    i2c_spi_tx_buf[6] = addr >> 8;
    i2c_spi_tx_buf[7] = addr;
    for (int i = 0; i < length; i++)
        i2c_spi_tx_buf[8 + i] = data[i];
    tp_config->transfer_status = gtb_write_data(tp_config,tp_config->spi_i2c_count,i2c_spi_tx_buf,8 + length);
    if(tp_config->transfer_status != HAL_OK)
    {
        send_error_code(tp_config,tp_config->spi_i2c_count,tp_config->transfer_status);
        return false;
    }

    return true;
}

bool fspi_write(tp_config_t *tp_config,uint8_t *data,uint16_t length)
{
    i2c_spi_tx_buf[0] = 0x95;
    i2c_spi_tx_buf[1] = 0x27;
    i2c_spi_tx_buf[2] = 0x05;
    i2c_spi_tx_buf[3] = 0xFA;
    i2c_spi_tx_buf[4] = 0x40;
    i2c_spi_tx_buf[5] = 0x00;
    i2c_spi_tx_buf[6] = 0x01;
    i2c_spi_tx_buf[7] = 0xC8;
    for (int i = 0; i < length; i++)
        i2c_spi_tx_buf[8 + i] = data[i];
    tp_config->transfer_status = gtb_write_data(tp_config,tp_config->spi_i2c_count,i2c_spi_tx_buf,8 + length);
    if(tp_config->transfer_status != HAL_OK)
    {
        send_error_code(tp_config,tp_config->spi_i2c_count,tp_config->transfer_status);
        return false;
    }

    return true;
}

bool fspi_read(tp_config_t *tp_config,uint8_t *data)
{
    uint8_t tmp[1];

    i2c_spi_tx_buf[0] = 0x95;
    i2c_spi_tx_buf[1] = 0x27;
    i2c_spi_tx_buf[2] = 0x04;
    i2c_spi_tx_buf[3] = 0xFB;
    i2c_spi_tx_buf[4] = 0x40;
    i2c_spi_tx_buf[5] = 0x00;
    i2c_spi_tx_buf[6] = 0x01;
    i2c_spi_tx_buf[7] = 0xC8;
    tp_config->transfer_status = gtb_write_data(tp_config,tp_config->spi_i2c_count,i2c_spi_tx_buf,8);
    if(tp_config->transfer_status != HAL_OK)
    {
        send_error_code(tp_config,tp_config->spi_i2c_count,tp_config->transfer_status);
        return false;
    }

    tp_config->transfer_status = gtb_read_data(tp_config,tp_config->spi_i2c_count,i2c_spi_tx_buf,tmp,1);
    if(tp_config->transfer_status != HAL_OK)
    {
        send_error_code(tp_config,tp_config->spi_i2c_count,tp_config->transfer_status);
        return false;
    }
    *data = tmp[0];

    return true;
}

bool fspi_flash_write_wait_for_enable_latch(tp_config_t *tp_config)
{
    uint8_t rdsr = 0x01;
    uint16_t retry_count = 0;
    do
    {
        i2c_spi_long_packet_tx_buffer[0] = 0xff;
        fspi_write(tp_config,i2c_spi_long_packet_tx_buffer,1);
        fspi_read(tp_config,&rdsr);
        if (retry_count < 100)
        {
            retry_count++;
            //delay_us(200);
        }
        else
        {
            return false;
        }
    }while((rdsr & 0x02) != 0x02);
    fspi_cs_enable(tp_config,false);

    return true;
}

bool fspi_flash_wait_for_write_end(tp_config_t *tp_config,uint32_t timeout)
{
    uint8_t rdsr = 0x01;
    uint16_t retry_count = 0;
    do
    {
        i2c_spi_long_packet_tx_buffer[0] = 0xff;
        fspi_write(tp_config,i2c_spi_long_packet_tx_buffer,1);
        fspi_read(tp_config,&rdsr);
        if (retry_count < timeout)
        {
            retry_count++;
            //delay_ms(1);
        }
        else
        {
            return false;
        }
    }while((rdsr & 0x01) != 0);
    fspi_cs_enable(tp_config,false);

    return true;
}
