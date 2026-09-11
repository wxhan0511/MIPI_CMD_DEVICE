#include "bsp.h"
#include "bsp_spi_flash.h"
#include "spi.h"

// static SPI_HandleTypeDef hspi_flash = {0};
// SPI Flash handle and buffers
static uint8_t s_spiBuf[4 * 1024];
static uint8_t g_spiTxBuf[SPI_BUFFER_SIZE];
static uint8_t g_spiRxBuf[SPI_BUFFER_SIZE];

/**
 * @brief Send and receive data over SPI
 * @param g_spiTxBuf Transmit buffer
 * @param g_spiRxBuf Receive buffer
 * @param g_spiLen   Transfer length
 * @retval 1 Success, 0 Failure
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
    if (status != HAL_OK)
    {
        // Error_Handler(__FILE__, __LINE__);
        printf("[spi flash] write error %d\r\n", status);
        return 0;
    }
    return 1;
}
/**
 * @brief Read the JEDEC ID of the SPI Flash
 * @retval 24-bit ID
 */
uint32_t bsp_flash_read_id(void)
{
    uint32_t uiID;
    uint8_t id1, id2, id3;
    // uint8_t g_spiTxBuf[4],g_spiRxBuf[4];

    SF_CS_L();                  /* Enable chip select */
    g_spiTxBuf[0] = (CMD_RDID); /* Send the read-ID command */

    bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, 4);

    id1 = g_spiRxBuf[1]; /* Byte 1 of the ID */
    id2 = g_spiRxBuf[2]; /* Byte 2 of the ID */
    id3 = g_spiRxBuf[3]; /* Byte 3 of the ID */
    SF_CS_H();           /* Release chip select */

    uiID = ((uint32_t)id1 << 16) | ((uint32_t)id2 << 8) | id3;
    // printf("[spi flash] read id %x\r\n",uiID);
    return uiID;
}

/**
 * @brief Enable SPI Flash write operations
 */
static void sf_WriteEnable(void)
{
    // uint8_t g_spiTxBuf[4],g_spiRxBuf[4];
    uint8_t status;
    SF_CS_L();                  /* Enable chip select */
    g_spiTxBuf[0] = (CMD_WREN); /* Send the write-enable command */
    status = bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, 1);
    SF_CS_H(); /* Release chip select */
    // printf("[spi flash] write enable %d \r\n",status);
}
/**
 * @brief Wait for the SPI Flash write/erase operation to finish
 */
static void sf_WaitForWriteEnd(void)
{
    // uint8_t g_spiTxBuf[4],g_spiRxBuf[4];
    uint8_t status;
    while (1)
    {
        SF_CS_L();                  /* Enable chip select */
        g_spiTxBuf[0] = (CMD_RDSR); /* Send command: read the status register */
        g_spiTxBuf[1] = 0;          /* Don't care */
        status = bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, 2);
        SF_CS_H(); /* Release chip select */

        // Q: status and g_spiRxBuf[1] equal 1 and 3 respectively -- what does that mean?
        // A: status == 1 means the transfer succeeded; g_spiRxBuf[1] holds the status register value
        if ((g_spiRxBuf[1] & WIP_FLAG) != SET) /* Check the busy flag in the status register */
        {
            break;
        }
    }
}
/**
 * @brief Erase the sector containing the given address
 * @param addr Sector start address
 */
void bsp_flash_erase_sector(uint32_t addr)
{
    // uint8_t g_spiTxBuf[5],g_spiRxBuf[5];
    uint8_t g_spiLen;
    uint8_t status;
    sf_WriteEnable();

    /* Send the sector erase command */
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
 * @brief Erase the entire SPI Flash chip
 */
void bsp_flash_erase_chip(void)
{
    // uint8_t g_spiTxBuf[5],g_spiRxBuf[5];
    uint8_t g_spiLen;

    sf_WriteEnable();
    // Select the SPI Flash
    SF_CS_L();
    g_spiLen = 0;
    g_spiTxBuf[g_spiLen++] = CMD_BE;
    bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, g_spiLen);
    printf("[spi flash] chip erase %d\r\n", g_spiRxBuf[0]);
    // g_spiRxBuf[0] == 255 means the erase command was sent; wait for the erase to finish
    // Wait for the erase to complete
    SF_CS_H();
    sf_WaitForWriteEnd();
}
/**
 * @brief Page write (up to 256 bytes per page), supports continuous multi-page writes
 * @param _pBuf        Pointer to the data to write
 * @param _uiWriteAddr Start address for the write
 * @param _usSize      Number of bytes to write (must be a multiple of 256)
 */
void sf_PageWrite(uint8_t *_pBuf, uint32_t _uiWriteAddr, uint16_t _usSize)
{
    // uint8_t g_spiTxBuf[SPI_BUFFER_SIZE];
    // uint8_t g_spiRxBuf[SPI_BUFFER_SIZE];
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
        // printf("[spi flash] page write %x %d %d\r\n",_uiWriteAddr,_usSize,status);
    }

    SF_CS_L();
    g_spiLen = 0;
    g_spiTxBuf[g_spiLen++] = (CMD_DISWR);
    status = bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, g_spiLen);
    SF_CS_H();
    // printf("[spi flash] write protect %d\r\n",status);
    sf_WaitForWriteEnd();
}
// Read the SPI Flash status register
uint8_t bsp_flash_read_status(void)
{
    uint8_t status;
    // uint8_t g_spiTxBuf[4],g_spiRxBuf[4];
    SF_CS_L();
    g_spiTxBuf[0] = (CMD_RDSR);
    bsp_spiTransfer(g_spiTxBuf, g_spiRxBuf, 2);
    status = g_spiRxBuf[1];
    SF_CS_H();
    return status;
}
void QSPI_FLASH_Wait_Busy(void)
{
    volatile uint32_t _reg;
    while (1)
    {
        _reg = bsp_flash_read_status();
        if ((_reg & 0x0101) == 0)
            break;
    }
}
/**
 * @brief Read data from the SPI Flash
 * @param p_buf     Buffer for the read data
 * @param read_addr Start address for the read
 * @param read_size Number of bytes to read
 */
void bsp_flash_read(uint8_t *p_buf, uint32_t read_addr, uint32_t read_size)
{
    // uint8_t g_spiTxBuf[5];
    // uint8_t g_spiTxBuf_read[SPI_BUFFER_SIZE];
    // uint8_t g_spiRxBuf[SPI_BUFFER_SIZE];
    uint32_t g_spiLen;
    uint16_t rem;
    uint16_t i;

    if ((read_size == 0) || (read_addr + read_size) > Flash_TotalSize)
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

    /* Start reading data, in chunks */
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
 * @brief Compare the data at the given Flash address with the target data
 * @param _uiSrcAddr Flash start address
 * @param _ucpTar    Pointer to the target data
 * @param _uiSize    Number of bytes to compare
 * @retval 0 Equal, 1 Not equal
 */
static uint8_t sf_CmpData(uint32_t _uiSrcAddr, uint8_t *_ucpTar, uint32_t _uiSize)
{
    // uint8_t g_spiTxBuf[5];
    // uint8_t g_spiTxBuf_read[SPI_BUFFER_SIZE];
    // uint8_t g_spiRxBuf[SPI_BUFFER_SIZE];
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

    /* Start reading data, in chunks */
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
 * @brief Check whether the old and new data require an erase (erase is needed
 *        when new data would require programming 0 bits back to 1)
 * @param _ucpOldBuf Old data
 * @param _ucpNewBuf New data
 * @param _usLen     Length
 * @retval 1 Erase needed, 0 Not needed
 */
static uint8_t sf_NeedErase(uint8_t *_ucpOldBuf, uint8_t *_ucpNewBuf, uint16_t _usLen)
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
 * @brief Write one page of data automatically (erase first if needed) and verify the write
 * @param _ucpSrc   Source data
 * @param _uiWrAddr Write address
 * @param _usWrLen  Write length
 * @retval 1 Success, 0 Failure
 */
static uint8_t sf_AutoWritePage(uint8_t *_ucpSrc, uint32_t _uiWrAddr, uint16_t _usWrLen)
{
    uint16_t i;
    uint16_t j;           /* Loop counter */
    uint32_t uiFirstAddr; /* Sector start address */
    uint8_t ucNeedErase;  /* 1 means erase is needed */
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
        for (i = 0; i < Flash_SectorSize; i++)
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
            bsp_flash_erase_sector(uiFirstAddr); /* Erase one sector */
        }

        sf_PageWrite(s_spiBuf, uiFirstAddr, Flash_SectorSize);

        if (sf_CmpData(_uiWrAddr, _ucpSrc, _usWrLen) == 0)
        {
            cRet = 1;
            break;
        }

        /* Delay a while after failure, then retry */
        for (j = 0; j < 10000; j++)
            ;
    }

    return cRet;
}
/**
 * @brief Write data to the SPI Flash (automatic page crossing, automatic erase, automatic verify)
 * @param p_buf      Pointer to the data to write
 * @param write_addr Start address for the write
 * @param write_size Number of bytes to write
 * @retval 1 Success, 0 Failure
 */

uint8_t bsp_flash_write(uint8_t *p_buf, uint32_t write_addr, uint16_t write_size)
{
    uint16_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;

    Addr = write_addr % Flash_SectorSize;
    count = Flash_SectorSize - Addr;
    NumOfPage = write_size / Flash_SectorSize;
    NumOfSingle = write_size % Flash_SectorSize;
    QSPI_FLASH_Wait_Busy();
    // printf("[spi flash] write: addr=0x%08lX, size=%u, Addr=%u, count=%u, NumOfPage=%u, NumOfSingle=%u\r\n",
    //     write_addr, write_size, Addr, count, NumOfPage, NumOfSingle);
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
                write_addr += Flash_SectorSize;
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

                write_addr += count;
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
            NumOfPage = write_size / Flash_SectorSize;
            NumOfSingle = write_size % Flash_SectorSize;

            if (sf_AutoWritePage(p_buf, write_addr, count) == 0)
            {
                printf("[spi flash] sf_AutoWritePage failed: addr=0x%08lX, size=%u\r\n", write_addr, count);
                return 0;
            }

            write_addr += count;
            p_buf += count;

            while (NumOfPage--)
            {
                if (sf_AutoWritePage(p_buf, write_addr, Flash_SectorSize) == 0)
                {
                    printf("[spi flash] sf_AutoWritePage failed: addr=0x%08lX, size=%u\r\n", write_addr, Flash_SectorSize);
                    return 0;
                }
                write_addr += Flash_SectorSize;
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
