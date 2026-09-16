//
// Created by Bobby on 2023/10/27.
//

#include "bsp_in_flash.h"

#include <string.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_flash.h"

int mem_init(void){
    HAL_FLASH_Unlock();
    return (0);
}

int mem_deinit(void){

    HAL_FLASH_Lock();
    return (0);
}

int mem_erase(const uint32_t address,uint32_t size)
{
    uint32_t SectorError;
    FLASH_EraseInitTypeDef pEraseInit;

    mem_init();
    const uint32_t start_sector = mem_get_sector(address);
    const uint32_t bank = FLASH_BANK_1;

    const uint32_t sector_offset = 1;
    pEraseInit.TypeErase = TYPEERASE_SECTORS;
    pEraseInit.Sector = start_sector;
    pEraseInit.Banks  = bank;
    pEraseInit.NbSectors = sector_offset;
    pEraseInit.VoltageRange = VOLTAGE_RANGE_3;
    const int status = HAL_FLASHEx_Erase(&pEraseInit, &SectorError);
    if (status != HAL_OK)
    {
        printf("erase error %d\r\n",status);
    }
    mem_deinit();
    return status;
}

// len: length in bytes, a multiple of 32
int mem_write(uint32_t address, uint8_t* write_data, uint32_t len)
{
    uint32_t i = 0;
    if((address + len) > FLASH_END_ADDR)
        return 1;
    if(len == 0)
        return 0;
    __set_PRIMASK(1);
    mem_init();
    for(i = 0; i < len; i++){
        uint32_t temp_word = 0;
        memcpy(&temp_word,write_data,4);
        write_data += 4;
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address,temp_word) == HAL_OK){
            address += 4;
        }
        else{
            goto err;
        }
    }

    mem_deinit();
    __set_PRIMASK(0);
    return (0);

    err:
        printf("flash address 0x%x ,data address 0x%x 0x%x\r\n",address,*write_data,*write_data);
        mem_deinit();
        __set_PRIMASK(0);
        return (1);
}


int mem_read(uint32_t address,uint32_t *buf,uint32_t size)
{
    uint32_t i;
    for(i=0;i<size;i++)
    {

        buf[i]= *(__IO uint32_t*)address; // Read 4 bytes.
        __DSB();
        address+=4; // Advance 4 bytes.
    }
    return 0;
}

uint16_t mem_read_half_word(uint32_t address)
{
    return *(__IO uint16_t*)address;
}

uint32_t mem_get_sector(uint32_t address)
{
    uint32_t sector = 0;

    if(address < ADDR_FLASH_SECTOR_1_BANK1 && address >= ADDR_FLASH_SECTOR_0_BANK1)
    {
        sector = FLASH_SECTOR_0;
    }
    else if(address < ADDR_FLASH_SECTOR_2_BANK1 && address >= ADDR_FLASH_SECTOR_1_BANK1)
    {
        sector = FLASH_SECTOR_1;
    }
    else if(address < ADDR_FLASH_SECTOR_3_BANK1 && address >= ADDR_FLASH_SECTOR_2_BANK1)
    {
        sector = FLASH_SECTOR_2;
    }
    else if(address < ADDR_FLASH_SECTOR_4_BANK1 && address >= ADDR_FLASH_SECTOR_3_BANK1)
    {
        sector = FLASH_SECTOR_3;
    }
    else if(address < ADDR_FLASH_SECTOR_5_BANK1 && address >= ADDR_FLASH_SECTOR_4_BANK1)
    {
        sector = FLASH_SECTOR_4;
    }
    else if(address < ADDR_FLASH_SECTOR_6_BANK1 && address >= ADDR_FLASH_SECTOR_5_BANK1)
    {
        sector = FLASH_SECTOR_5;
    }
    else if(address < ADDR_FLASH_SECTOR_7_BANK1 && address >= ADDR_FLASH_SECTOR_6_BANK1)
    {
        sector = FLASH_SECTOR_6;
    }
    else if(address >= ADDR_FLASH_SECTOR_7_BANK1 && address < FLASH_END_ADDR)
    {
        sector = FLASH_SECTOR_7;
    }
    else
    {
        sector = FLASH_SECTOR_7;
    }

    return sector;
}
