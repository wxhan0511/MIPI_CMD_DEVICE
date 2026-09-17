/**
 * @file    boot_extflash.c
 * @brief   W25Q256 (SPI3) driver — faithful port of the application's
 *          Bsp/bsp_spi_flash.c so the backup/restore path behaves identically
 *          to the working application firmware.
 *
 * Differences from the app only: DWT delay is self-initialized, logging goes
 * through Boot_Log_Printf instead of printf, and the SPI3 handle is owned here.
 */

#include "boot_extflash.h"

#include <string.h>

#include "boot_log.h"

static SPI_HandleTypeDef hspi3;
static uint8_t s_spiBuf[4 * 1024];
static uint8_t g_spiTxBuf[4096];
static uint8_t g_spiRxBuf[4096];

#define SPI_BUFFER_SIZE 4096
#define Flash_SectorSize 4096
#define Flash_TotalSize (32 * 1024 * 1024)

#define EXT_CS_LOW() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET)
#define EXT_CS_HIGH() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET)

#define CMD_WREN 0x06U
#define CMD_READ 0x13U
#define CMD_RDSR 0x05U
#define CMD_SE 0x21U
#define CMD_PP 0x12U
#define CMD_RDID 0x9FU
#define WIP_FLAG 0x01U

/* ------------------------------------------------------------------ */
/* DWT cycle-counter delay (mirrors the app's Bsp/bsp_dwt.c)           */
/* ------------------------------------------------------------------ */
#define DEM_CR_TRCENA (1UL << 24)
#define DWT_CR_CYCCNTENA (1UL << 0)

static void ext_init_dwt(void)
{
  CoreDebug->DEMCR |= DEM_CR_TRCENA;
  DWT->CYCCNT = 0U;
  DWT->CTRL |= DWT_CR_CYCCNTENA;
}

static void ext_delay_us(uint32_t time)
{
  uint32_t tCnt = 0U;
  uint32_t tDelayCnt = time * (HAL_RCC_GetSysClockFreq() / 1000000U);
  uint32_t tStart = DWT->CYCCNT;

  while (tCnt < tDelayCnt)
  {
    tCnt = DWT->CYCCNT - tStart;
  }
}

/* ------------------------------------------------------------------ */
/* SPI transfer (mirrors bsp_spiTransfer)                              */
/* ------------------------------------------------------------------ */
static uint8_t ext_spi_transfer(uint8_t *tx, uint8_t *rx, uint16_t len)
{
  ext_delay_us(100);
  if (len > SPI_BUFFER_SIZE)
  {
    return 0U;
  }
  if (HAL_SPI_TransmitReceive(&hspi3, tx, rx, len, 1000000U) != HAL_OK)
  {
    Boot_Log_Printf("[spi flash] transfer error %u\r\n", (unsigned)len);
    return 0U;
  }
  return 1U;
}

static void ext_write_enable(void)
{
  EXT_CS_LOW();
  g_spiTxBuf[0] = CMD_WREN;
  ext_spi_transfer(g_spiTxBuf, g_spiRxBuf, 1);
  EXT_CS_HIGH();
}

static void ext_wait_busy(void)
{
  while (1)
  {
    EXT_CS_LOW();
    g_spiTxBuf[0] = CMD_RDSR;
    g_spiTxBuf[1] = 0U;
    ext_spi_transfer(g_spiTxBuf, g_spiRxBuf, 2);
    EXT_CS_HIGH();
    if ((g_spiRxBuf[1] & WIP_FLAG) == 0U)
    {
      break;
    }
  }
}

static uint8_t ext_read_status(void)
{
  EXT_CS_LOW();
  g_spiTxBuf[0] = CMD_RDSR;
  g_spiTxBuf[1] = 0U;
  ext_spi_transfer(g_spiTxBuf, g_spiRxBuf, 2);
  EXT_CS_HIGH();
  return g_spiRxBuf[1];
}

/* ------------------------------------------------------------------ */
/* Erase / read / page-program / write (mirrors bsp_spi_flash.c)       */
/* ------------------------------------------------------------------ */
static void ext_erase_sector(uint32_t addr)
{
  uint8_t n;
  ext_write_enable();

  EXT_CS_LOW();
  n = 0;
  g_spiTxBuf[n++] = CMD_SE;
  g_spiTxBuf[n++] = (uint8_t)(addr >> 24);
  g_spiTxBuf[n++] = (uint8_t)(addr >> 16);
  g_spiTxBuf[n++] = (uint8_t)(addr >> 8);
  g_spiTxBuf[n++] = (uint8_t)(addr);
  ext_spi_transfer(g_spiTxBuf, g_spiRxBuf, n);
  EXT_CS_HIGH();

  ext_wait_busy();
}

static void ext_read(uint8_t *p_buf, uint32_t read_addr, uint32_t read_size)
{
  uint8_t n;
  uint16_t rem, i;

  if ((read_size == 0U) || (read_addr + read_size) > Flash_TotalSize)
  {
    return;
  }

  ext_wait_busy();
  EXT_CS_LOW();
  n = 0;
  g_spiTxBuf[n++] = CMD_READ;
  g_spiTxBuf[n++] = (uint8_t)(read_addr >> 24);
  g_spiTxBuf[n++] = (uint8_t)(read_addr >> 16);
  g_spiTxBuf[n++] = (uint8_t)(read_addr >> 8);
  g_spiTxBuf[n++] = (uint8_t)(read_addr);
  ext_spi_transfer(g_spiTxBuf, g_spiRxBuf, n);

  for (i = 0; i < read_size / SPI_BUFFER_SIZE; i++)
  {
    ext_spi_transfer(g_spiTxBuf, g_spiRxBuf, SPI_BUFFER_SIZE);
    memcpy(p_buf, g_spiRxBuf, SPI_BUFFER_SIZE);
    p_buf += SPI_BUFFER_SIZE;
  }

  rem = (uint16_t)(read_size % SPI_BUFFER_SIZE);
  if (rem > 0U)
  {
    ext_spi_transfer(g_spiTxBuf, g_spiRxBuf, rem);
    memcpy(p_buf, g_spiRxBuf, rem);
  }
  EXT_CS_HIGH();
}

static void ext_page_write(uint8_t *p_buf, uint32_t write_addr, uint16_t size)
{
  uint32_t n;
  uint32_t i, j;

  for (j = 0; j < size / 256U; j++)
  {
    ext_write_enable();

    EXT_CS_LOW();
    n = 0;
    g_spiTxBuf[n++] = CMD_PP;
    g_spiTxBuf[n++] = (uint8_t)(write_addr >> 24);
    g_spiTxBuf[n++] = (uint8_t)(write_addr >> 16);
    g_spiTxBuf[n++] = (uint8_t)(write_addr >> 8);
    g_spiTxBuf[n++] = (uint8_t)(write_addr);
    for (i = 0; i < 256U; i++)
    {
      g_spiTxBuf[n++] = (*p_buf++);
    }
    ext_spi_transfer(g_spiTxBuf, g_spiRxBuf, (uint16_t)n);
    EXT_CS_HIGH();

    ext_wait_busy();
    write_addr += 256U;
  }
}

static uint8_t ext_cmp_data(uint32_t src_addr, uint8_t *tar, uint32_t size)
{
  uint8_t n;
  uint16_t i, j, rem;

  if ((src_addr + size) > Flash_TotalSize || size == 0U)
  {
    return 1U;
  }

  EXT_CS_LOW();
  n = 0;
  g_spiTxBuf[n++] = CMD_READ;
  g_spiTxBuf[n++] = (uint8_t)(src_addr >> 24);
  g_spiTxBuf[n++] = (uint8_t)(src_addr >> 16);
  g_spiTxBuf[n++] = (uint8_t)(src_addr >> 8);
  g_spiTxBuf[n++] = (uint8_t)(src_addr);
  ext_spi_transfer(g_spiTxBuf, g_spiRxBuf, n);

  for (i = 0; i < size / SPI_BUFFER_SIZE; i++)
  {
    ext_spi_transfer(g_spiTxBuf, g_spiRxBuf, SPI_BUFFER_SIZE);
    for (j = 0; j < SPI_BUFFER_SIZE; j++)
    {
      if (g_spiRxBuf[j] != *tar++)
      {
        goto NOTEQ;
      }
    }
  }

  rem = (uint16_t)(size % SPI_BUFFER_SIZE);
  if (rem > 0U)
  {
    ext_spi_transfer(g_spiTxBuf, g_spiRxBuf, rem);
    for (j = 0; j < rem; j++)
    {
      if (g_spiRxBuf[j] != *tar++)
      {
        goto NOTEQ;
      }
    }
  }
  EXT_CS_HIGH();
  return 0U;

NOTEQ:
  EXT_CS_HIGH();
  return 1U;
}

static uint8_t ext_need_erase(uint8_t *old_buf, uint8_t *new_buf, uint16_t len)
{
  uint16_t i;
  for (i = 0; i < len; i++)
  {
    if ((~(*old_buf++) & (*new_buf++)) != 0U)
    {
      return 1U;
    }
  }
  return 0U;
}

static uint8_t ext_auto_write_page(uint8_t *src, uint32_t wr_addr, uint16_t wr_len)
{
  uint16_t i, j;
  uint32_t first_addr;
  uint8_t need_erase, ret;

  if (wr_len == 0U)
  {
    return 1U;
  }
  if ((wr_addr + wr_len) >= Flash_TotalSize || wr_len > Flash_SectorSize)
  {
    return 0U;
  }

  ext_read(s_spiBuf, wr_addr, wr_len);
  if (memcmp(s_spiBuf, src, wr_len) == 0)
  {
    return 1U;
  }

  need_erase = ext_need_erase(s_spiBuf, src, wr_len);
  first_addr = wr_addr & 0xFFFFF000U;

  if (wr_len == Flash_SectorSize)
  {
    for (i = 0; i < Flash_SectorSize; i++)
    {
      s_spiBuf[i] = src[i];
    }
  }
  else
  {
    ext_read(s_spiBuf, first_addr, Flash_SectorSize);
    i = (uint16_t)(wr_addr & 0xFFFU);
    memcpy(&s_spiBuf[i], src, wr_len);
  }

  ret = 0U;
  for (i = 0; i < 3U; i++)
  {
    if (need_erase == 1U)
    {
      ext_erase_sector(first_addr);
    }
    ext_page_write(s_spiBuf, first_addr, Flash_SectorSize);
    if (ext_cmp_data(wr_addr, src, wr_len) == 0U)
    {
      ret = 1U;
      break;
    }
    for (j = 0; j < 10000U; j++)
    {
    }
  }
  return ret;
}

static uint8_t ext_write(uint8_t *p_buf, uint32_t write_addr, uint16_t write_size)
{
  uint16_t NumOfPage, NumOfSingle, Addr, count, temp;
  uint32_t addr = write_addr;

  Addr = (uint16_t)(addr % Flash_SectorSize);
  count = Flash_SectorSize - Addr;
  NumOfPage = write_size / Flash_SectorSize;
  NumOfSingle = write_size % Flash_SectorSize;
  ext_wait_busy();

  if (Addr == 0U)
  {
    if (NumOfPage == 0U)
    {
      if (ext_auto_write_page(p_buf, addr, write_size) == 0U)
      {
        return 0U;
      }
    }
    else
    {
      while (NumOfPage--)
      {
        if (ext_auto_write_page(p_buf, addr, Flash_SectorSize) == 0U)
        {
          return 0U;
        }
        addr += Flash_SectorSize;
        p_buf += Flash_SectorSize;
      }
      if (NumOfSingle != 0U)
      {
        if (ext_auto_write_page(p_buf, addr, NumOfSingle) == 0U)
        {
          return 0U;
        }
      }
    }
  }
  else
  {
    if (NumOfPage == 0U)
    {
      if (NumOfSingle > count)
      {
        temp = NumOfSingle - count;
        if (ext_auto_write_page(p_buf, addr, count) == 0U)
        {
          return 0U;
        }
        addr += count;
        p_buf += count;
        if (ext_auto_write_page(p_buf, addr, temp) == 0U)
        {
          return 0U;
        }
      }
      else
      {
        if (ext_auto_write_page(p_buf, addr, write_size) == 0U)
        {
          return 0U;
        }
      }
    }
    else
    {
      write_size -= count;
      NumOfPage = write_size / Flash_SectorSize;
      NumOfSingle = write_size % Flash_SectorSize;

      if (ext_auto_write_page(p_buf, addr, count) == 0U)
      {
        return 0U;
      }
      addr += count;
      p_buf += count;

      while (NumOfPage--)
      {
        if (ext_auto_write_page(p_buf, addr, Flash_SectorSize) == 0U)
        {
          return 0U;
        }
        addr += Flash_SectorSize;
        p_buf += Flash_SectorSize;
      }

      if (NumOfSingle != 0U)
      {
        if (ext_auto_write_page(p_buf, addr, NumOfSingle) == 0U)
        {
          return 0U;
        }
      }
    }
  }
  return 1U;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */
void Boot_ExtFlash_Init(void)
{
  __HAL_RCC_SPI3_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = GPIO_PIN_13; /* CS */
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &gpio);

  gpio.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12; /* SCK/MISO/MOSI */
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(GPIOC, &gpio);

  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  HAL_SPI_Init(&hspi3);

  ext_init_dwt();
  EXT_CS_HIGH();

  /* Diagnostic: read JEDEC ID and status register 1. */
  uint32_t id;
  EXT_CS_LOW();
  g_spiTxBuf[0] = CMD_RDID;
  ext_spi_transfer(g_spiTxBuf, g_spiRxBuf, 4);
  EXT_CS_HIGH();
  id = ((uint32_t)g_spiRxBuf[1] << 16) | ((uint32_t)g_spiRxBuf[2] << 8) | g_spiRxBuf[3];
  Boot_Log_Printf("extflash: JEDEC ID = 0x%06lX", (uint32_t)id);

  uint8_t s1 = ext_read_status();
  Boot_Log_Printf("extflash: S1=0x%02X (BP=%d SRP=%d)",
                  s1, (s1 >> 2) & 0x0F, (s1 >> 7) & 0x01);
}

void Boot_ExtFlash_DeInit(void)
{
  HAL_SPI_DeInit(&hspi3);
}

uint8_t Boot_ExtFlash_EraseRange(uint32_t addr, uint32_t len)
{
  uint32_t end = addr + len;
  for (uint32_t a = addr; a < end; a += BOOT_EXT_SECTOR_SIZE)
  {
    ext_erase_sector(a);
  }
  return 1U;
}

uint8_t Boot_ExtFlash_Read(uint8_t *buf, uint32_t addr, uint32_t len)
{
  ext_read(buf, addr, len);
  return 1U;
}

uint8_t Boot_ExtFlash_Write(uint8_t *buf, uint32_t addr, uint32_t len)
{
  return ext_write(buf, addr, (uint16_t)len);
}
