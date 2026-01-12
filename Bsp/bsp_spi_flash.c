#include "bsp.h"
#include "bsp_spi_flash.h"
#include "spi.h"



// static SPI_HandleTypeDef hspi_flash = {0};
// SPI Flash 句柄和缓冲区
static uint8_t s_spiBuf[4*1024];
static uint8_t g_spiTxBuf[SPI_BUFFER_SIZE];
static uint8_t g_spiRxBuf[SPI_BUFFER_SIZE];

/**
 * @brief 通过SPI发送和接收数据
 * @param g_spiTxBuf 发送缓冲区
 * @param g_spiRxBuf 接收缓冲区
 * @param g_spiLen   发送/接收长度
 * @retval 1 成功，0 失败
 */
uint8_t bsp_spiTransfer(uint8_t *g_spiTxBuf, uint8_t *g_spiRxBuf, uint16_t g_spiLen)
{
    uint8_t status;
    bsp_delay_us(100);
    if (g_spiLen > SPI_BUFFER_SIZE)
    {
        return 0;
    }
    status = HAL_SPI_TransmitReceive(&hspi3, g_spiTxBuf, g_spiRxBuf, g_spiLen, 1000000);
    if(status != HAL_OK)
    {
        //Error_Handler(__FILE__, __LINE__);
        printf("[spi flash] write error %d\r\n",status);
        return 0;
    }
    return 1;
}
/**
 * @brief 读取SPI Flash的JEDEC ID
 * @retval 24位ID
 */
uint32_t bsp_flash_read_id(void)
{
    uint32_t uiID;
    uint8_t id1, id2, id3;
    //uint8_t g_spiTxBuf[4],g_spiRxBuf[4];

    SF_CS_L();									/* ʹ��Ƭѡ */
    g_spiTxBuf[0] = (CMD_RDID);								/* ���Ͷ�ID���� */

    bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, 4);

    id1 = g_spiRxBuf[1];					/* ��ID�ĵ�1���ֽ� */
    id2 = g_spiRxBuf[2];					/* ��ID�ĵ�2���ֽ� */
    id3 = g_spiRxBuf[3];					/* ��ID�ĵ�3���ֽ� */
    SF_CS_H();									  /* ����Ƭѡ */

    uiID = ((uint32_t)id1 << 16) | ((uint32_t)id2 << 8) | id3;
    //printf("[spi flash] read id %x\r\n",uiID);
    return uiID;
}

/**
 * @brief 使能SPI Flash写操作
 */
static void sf_WriteEnable(void)
{
    //uint8_t g_spiTxBuf[4],g_spiRxBuf[4];
    uint8_t status;
    SF_CS_L();									/* ʹ��Ƭѡ */
    g_spiTxBuf[0] = (CMD_WREN);								/* �������� */
    status = bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, 1);
    SF_CS_H();									/* ����Ƭѡ */
    //printf("[spi flash] write enable %d \r\n",status);
}
/**
 * @brief 等待SPI Flash写入/擦除操作完成
 */
static void sf_WaitForWriteEnd(void)
{
    //uint8_t g_spiTxBuf[4],g_spiRxBuf[4];
    uint8_t status;
    while(1)
    {
        SF_CS_L();									/* ʹ��Ƭѡ */
        g_spiTxBuf[0] = (CMD_RDSR);						/* ������� ��״̬�Ĵ��� */
        g_spiTxBuf[1] = 0;		/* �޹����� */
        status = bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, 2);
        SF_CS_H();									/* ����Ƭѡ */
        
        //q:status,g_spiRxBuf[1]分别为1,3,解释为什么
        //a:status为1表示传输成功，g_spiRxBuf[1]为3表示写入进行中
        if ((g_spiRxBuf[1] & WIP_FLAG) != SET)	/* �ж�״̬�Ĵ�����æ��־λ */
        {
            break;
        }
    }
    
}
/**
 * @brief 擦除指定地址所在的扇区
 * @param addr 扇区起始地址
 */
void bsp_flash_erase_sector(uint32_t addr)
{
    //uint8_t g_spiTxBuf[5],g_spiRxBuf[5];
    uint8_t g_spiLen;
    uint8_t status;
    sf_WriteEnable();

    /* ������������ */
    SF_CS_L();
    g_spiLen = 0;
    g_spiTxBuf[g_spiLen++] = CMD_SE;
    g_spiTxBuf[g_spiLen++] = ((addr & 0xFF000000) >> 24);
    g_spiTxBuf[g_spiLen++] = ((addr & 0xFF0000) >> 16);
    g_spiTxBuf[g_spiLen++] = ((addr & 0xFF00) >> 8);
    g_spiTxBuf[g_spiLen++] = (addr & 0xFF);
    status = bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, g_spiLen);
    SF_CS_H();

    sf_WaitForWriteEnd();
}
/**
 * @brief 整片擦除SPI Flash
 */
void bsp_flash_erase_chip(void)
{
    //uint8_t g_spiTxBuf[5],g_spiRxBuf[5];
    uint8_t g_spiLen;

    sf_WriteEnable();
    //选中SPI Flash
    SF_CS_L();
    g_spiLen = 0;
    g_spiTxBuf[g_spiLen++] = CMD_BE;
    bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, g_spiLen);
    printf("[spi flash] chip erase %d\r\n",g_spiRxBuf[0]);
    //g_spiRxBuf[0]为255表示擦除命令发送成功，等待擦除完成
    //等待擦除完成
    SF_CS_H();
    sf_WaitForWriteEnd();
}
/**
 * @brief 页写入（最多256字节），支持多页连续写
 * @param _pBuf      写入数据指针
 * @param _uiWriteAddr 写入起始地址
 * @param _usSize    写入字节数（必须为256的倍数）
 */
void sf_PageWrite(uint8_t * _pBuf, uint32_t _uiWriteAddr, uint16_t _usSize)
{
    //uint8_t g_spiTxBuf[SPI_BUFFER_SIZE];
    //uint8_t g_spiRxBuf[SPI_BUFFER_SIZE];
    uint32_t g_spiLen;
    uint32_t i, j;
    uint8_t status;
    for (j = 0; j < _usSize / 256; j++)
    {
        sf_WriteEnable();

        SF_CS_L();
        g_spiLen = 0;
        g_spiTxBuf[g_spiLen++] = (0x12);
        g_spiTxBuf[g_spiLen++] = ((_uiWriteAddr & 0xFF000000) >> 24);
        g_spiTxBuf[g_spiLen++] = ((_uiWriteAddr & 0xFF0000) >> 16);
        g_spiTxBuf[g_spiLen++] = ((_uiWriteAddr & 0xFF00) >> 8);
        g_spiTxBuf[g_spiLen++] = (_uiWriteAddr & 0xFF);
        for (i = 0; i < 256; i++)
        {
            g_spiTxBuf[g_spiLen++] = (*_pBuf++);
        }
        status = bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, g_spiLen);
        SF_CS_H();

        sf_WaitForWriteEnd();

        _uiWriteAddr += 256;
        //printf("[spi flash] page write %x %d %d\r\n",_uiWriteAddr,_usSize,status);
    }

    SF_CS_L();
    g_spiLen = 0;
    g_spiTxBuf[g_spiLen++] = (CMD_DISWR);
    status = bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, g_spiLen);
    SF_CS_H();
    //printf("[spi flash] write protect %d\r\n",status);
    sf_WaitForWriteEnd();
}
//读取SPI Flash的状态寄存器
uint8_t bsp_flash_read_status(void)
{
    uint8_t status;
    //uint8_t g_spiTxBuf[4],g_spiRxBuf[4];
    SF_CS_L();									
    g_spiTxBuf[0] = (CMD_RDSR);								
    bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, 2);
    status = g_spiRxBuf[1];					
    SF_CS_H();									 
    //printf("[spi flash] read status %d\r\n",status);
    return status;
}
void QSPI_FLASH_Wait_Busy(void)
{
	volatile uint32_t _reg;
	while(1){
		_reg = bsp_flash_read_status();
		if((_reg & 0x0101)==0) break;
	}
}
/**
 * @brief 从SPI Flash读取数据
 * @param p_buf     读取数据缓冲区
 * @param read_addr 读取起始地址
 * @param read_size 读取字节数
 */
void bsp_flash_read(uint8_t * p_buf, uint32_t read_addr, uint32_t read_size)
{
    //uint8_t g_spiTxBuf[5];
    //uint8_t g_spiTxBuf_read[SPI_BUFFER_SIZE];
    //uint8_t g_spiRxBuf[SPI_BUFFER_SIZE];
    uint32_t g_spiLen;
    uint16_t rem;
    uint16_t i;

    if ((read_size == 0) ||(read_addr + read_size) > Flash_TotalSize)
    {
        return;
    }
    QSPI_FLASH_Wait_Busy();
    SF_CS_L();
    g_spiLen = 0;
    g_spiTxBuf[g_spiLen++] = (CMD_READ);
    g_spiTxBuf[g_spiLen++] = ((read_addr & 0xFF000000) >> 24);
    g_spiTxBuf[g_spiLen++] = ((read_addr & 0xFF0000) >> 16);
    g_spiTxBuf[g_spiLen++] = ((read_addr & 0xFF00) >> 8);
    g_spiTxBuf[g_spiLen++] = (read_addr & 0xFF);
    bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, g_spiLen);

    /* ��ʼ�����ݣ��ְ��� */
    for (i = 0; i < read_size / SPI_BUFFER_SIZE; i++)
    {
        g_spiLen = SPI_BUFFER_SIZE;
        bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, g_spiLen);

        memcpy(p_buf, g_spiRxBuf, SPI_BUFFER_SIZE);
        p_buf += SPI_BUFFER_SIZE;
    }

    rem = read_size % SPI_BUFFER_SIZE;
    if (rem > 0)
    {
        g_spiLen = rem;
        bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, g_spiLen);

        memcpy(p_buf, g_spiRxBuf, rem);
    }
    SF_CS_H();
}
/**
 * @brief 比较SPI Flash指定地址的数据与目标数据是否一致
 * @param _uiSrcAddr Flash起始地址
 * @param _ucpTar    目标数据指针
 * @param _uiSize    比较字节数
 * @retval 0 相等，1 不相等
 */
static uint8_t sf_CmpData(uint32_t _uiSrcAddr, uint8_t *_ucpTar, uint32_t _uiSize)
{
    //uint8_t g_spiTxBuf[5];
    //uint8_t g_spiTxBuf_read[SPI_BUFFER_SIZE];
    //uint8_t g_spiRxBuf[SPI_BUFFER_SIZE];
    uint32_t g_spiLen;

    uint16_t i, j;
    uint16_t rem;

    if ((_uiSrcAddr + _uiSize) > Flash_TotalSize)
    {
        return 1;
    }

    if (_uiSize == 0)
    {
        return 0;
    }

    SF_CS_L();
    g_spiLen = 0;
    g_spiTxBuf[g_spiLen++] = (CMD_READ);
    g_spiTxBuf[g_spiLen++] = ((_uiSrcAddr & 0xFF000000) >> 24);
    g_spiTxBuf[g_spiLen++] = ((_uiSrcAddr & 0xFF0000) >> 16);
    g_spiTxBuf[g_spiLen++] = ((_uiSrcAddr & 0xFF00) >> 8);
    g_spiTxBuf[g_spiLen++] = (_uiSrcAddr & 0xFF);
    bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, g_spiLen);

    /* ��ʼ�����ݣ��ְ��� */
    for (i = 0; i < _uiSize / SPI_BUFFER_SIZE; i++)
    {
        g_spiLen = SPI_BUFFER_SIZE;
        bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, g_spiLen);

        for (j = 0; j < SPI_BUFFER_SIZE; j++)
        {
            if (g_spiRxBuf[j] != *_ucpTar++)
            {
                goto NOTEQ;
            }
        }
    }

    rem = _uiSize % SPI_BUFFER_SIZE;
    if (rem > 0)
    {
        g_spiLen = rem;
        bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, g_spiLen);

        for (j = 0; j < rem; j++)
        {
            if (g_spiRxBuf[j] != *_ucpTar++)
            {
                goto NOTEQ;
            }
        }
    }
    SF_CS_H();
    return 0;

    NOTEQ:
    SF_CS_H();
    return 1;
}
/**
 * @brief 判断新旧数据是否需要擦除（有0变1则需要擦除）
 * @param _ucpOldBuf 旧数据
 * @param _ucpNewBuf 新数据
 * @param _usLen     长度
 * @retval 1 需要擦除，0 不需要
 */
static uint8_t sf_NeedErase(uint8_t * _ucpOldBuf, uint8_t *_ucpNewBuf, uint16_t _usLen)
{
    uint16_t i;
    uint8_t ucOld;
    for (i = 0; i < _usLen; i++)
    {
        ucOld = *_ucpOldBuf++;
        ucOld = ~ucOld;
        if ((ucOld & (*_ucpNewBuf++)) != 0)
        {
            return 1;
        }
    }
    return 0;
}
/**
 * @brief 自动写入一页数据（必要时自动擦除），并校验写入正确性
 * @param _ucpSrc   源数据
 * @param _uiWrAddr 写入地址
 * @param _usWrLen  写入长度
 * @retval 1 成功，0 失败
 */
static uint8_t sf_AutoWritePage(uint8_t *_ucpSrc, uint32_t _uiWrAddr, uint16_t _usWrLen)
{
    uint16_t i;
    uint16_t j;					/* ������ʱ */
    uint32_t uiFirstAddr;		/* ������ַ */
    uint8_t ucNeedErase;		/* 1��ʾ��Ҫ���� */
    uint8_t cRet;

    if (_usWrLen == 0)
    {
        return 1;
    }

    if (_uiWrAddr + _usWrLen >= Flash_TotalSize)
    {
        printf("[spi flash] write addr out of range: addr=0x%08lX, size=%u\r\n", _uiWrAddr, _usWrLen);
        return 0;
    }

    if (_usWrLen > Flash_SectorSize)
    {
        printf("[spi flash] write size too large: addr=0x%08lX, size=%u\r\n", _uiWrAddr, _usWrLen);
        return 0;
    }

    bsp_flash_read(s_spiBuf, _uiWrAddr, _usWrLen);
    if (memcmp(s_spiBuf, _ucpSrc, _usWrLen) == 0)
    {
        return 1;
    }

    ucNeedErase = 0;
    if (sf_NeedErase(s_spiBuf, _ucpSrc, _usWrLen))
    {
        ucNeedErase = 1;
    }

    uiFirstAddr = _uiWrAddr & 0xfffff000;

    if (_usWrLen == Flash_SectorSize)
    {
        for	(i = 0; i < Flash_SectorSize; i++)
        {
            s_spiBuf[i] = _ucpSrc[i];
        }
    }
    else
    {
        bsp_flash_read(s_spiBuf, uiFirstAddr, Flash_SectorSize);

        i = _uiWrAddr & 0xfff;
        memcpy(&s_spiBuf[i], _ucpSrc, _usWrLen);
    }

    cRet = 0;
    for (i = 0; i < 3; i++)
    {
        if (ucNeedErase == 1)
        {
            bsp_flash_erase_sector(uiFirstAddr);		/* ����1������ */
        }

        sf_PageWrite(s_spiBuf, uiFirstAddr, Flash_SectorSize);

        if (sf_CmpData(_uiWrAddr, _ucpSrc, _usWrLen) == 0)
        {
            cRet = 1;
            break;
        }
        else
        {
            if (sf_CmpData(_uiWrAddr, _ucpSrc, _usWrLen) == 0)
            {
                cRet = 1;
                break;
            }

            /* ʧ�ܺ��ӳ�һ��ʱ�������� */
            for (j = 0; j < 10000; j++);
        }
    }

    return cRet;
}
/**
 * @brief 向SPI Flash写入数据（自动跨页、自动擦除、自动校验）
 * @param p_buf      写入数据指针
 * @param write_addr 写入起始地址
 * @param write_size 写入字节数
 * @retval 1 成功，0 失败
 */

 
uint8_t bsp_flash_write(uint8_t* p_buf, uint32_t write_addr, uint16_t write_size)
{
    uint16_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;

    Addr = write_addr % Flash_SectorSize;
    count = Flash_SectorSize - Addr;
    NumOfPage =  write_size / Flash_SectorSize;
    NumOfSingle = write_size % Flash_SectorSize;
    QSPI_FLASH_Wait_Busy();
    //printf("[spi flash] write: addr=0x%08lX, size=%u, Addr=%u, count=%u, NumOfPage=%u, NumOfSingle=%u\r\n",
    //    write_addr, write_size, Addr, count, NumOfPage, NumOfSingle);
    if (Addr == 0)
    {
        if (NumOfPage == 0)
        {
            if (sf_AutoWritePage(p_buf, write_addr, write_size) == 0)
            {
                printf("[spi flash] sf_AutoWritePage failed: addr=0x%08lX, size=%u\r\n", write_addr, write_size);
                return 0;
            }
        }
        else
        {
            while (NumOfPage--)
            {
                if (sf_AutoWritePage(p_buf, write_addr, Flash_SectorSize) == 0)
                {
                    printf("[spi flash] sf_AutoWritePage failed: addr=0x%08lX, size=%u\r\n", write_addr, Flash_SectorSize);
                    return 0;
                }
                write_addr +=  Flash_SectorSize;
                p_buf += Flash_SectorSize;
            }
            if (sf_AutoWritePage(p_buf, write_addr, NumOfSingle) == 0)
            {
                printf("[spi flash] sf_AutoWritePage failed: addr=0x%08lX, size=%u\r\n", write_addr, NumOfSingle);
                return 0;
            }
        }
    }
    else
    {
        if (NumOfPage == 0)
        {
            if (NumOfSingle > count) /* (_usWriteSize + _uiWriteAddr) > SPI_FLASH_PAGESIZE */
            {
                temp = NumOfSingle - count;

                if (sf_AutoWritePage(p_buf, write_addr, count) == 0)
                {
                    printf("[spi flash] sf_AutoWritePage failed: addr=0x%08lX, size=%u\r\n", write_addr, count);
                    return 0;
                }

                write_addr +=  count;
                p_buf += count;

                if (sf_AutoWritePage(p_buf, write_addr, temp) == 0)
                {
                    printf("[spi flash] sf_AutoWritePage failed: addr=0x%08lX, size=%u\r\n", write_addr, temp);
                    return 0;
                }
            }
            else
            {
                if (sf_AutoWritePage(p_buf, write_addr, write_size) == 0)
                {
                    printf("[spi flash] sf_AutoWritePage failed: addr=0x%08lX, size=%u\r\n", write_addr, write_size);
                    return 0;
                }
            }
        }
        else
        {
            write_size -= count;
            NumOfPage =  write_size / Flash_SectorSize;
            NumOfSingle = write_size % Flash_SectorSize;

            if (sf_AutoWritePage(p_buf, write_addr, count) == 0)
            {
                printf("[spi flash] sf_AutoWritePage failed: addr=0x%08lX, size=%u\r\n", write_addr, count);
                return 0;
            }

            write_addr +=  count;
            p_buf += count;

            while (NumOfPage--)
            {
                if (sf_AutoWritePage(p_buf, write_addr, Flash_SectorSize) == 0)
                {
                    printf("[spi flash] sf_AutoWritePage failed: addr=0x%08lX, size=%u\r\n", write_addr, Flash_SectorSize);
                    return 0;
                }
                write_addr +=  Flash_SectorSize;
                p_buf += Flash_SectorSize;
            }

            if (NumOfSingle != 0)
            {
                if (sf_AutoWritePage(p_buf, write_addr, NumOfSingle) == 0)
                {
                    printf("[spi flash] sf_AutoWritePage failed: addr=0x%08lX, size=%u\r\n", write_addr, NumOfSingle);
                    return 0;
                }
            }
        }
    }
    return 1;
}





