/**
 ******************************************************************************
 * @file    main.c
 * @brief   Bootloader for the STM32F407VGT6 MIPI CMD board.
 *
 * Boot decision tree:
 *   1. RTC backup flag (BKP0R == BOOT_MAGIC)?  -> clear it, upgrade mode.
 *   2. Valid application at 0x08010000?        -> jump to it.
 *   3. Otherwise                                -> upgrade mode (wait for host).
 *
 * Upgrade mode listens on SPI2 as a slave (same wiring as the application)
 * and speaks the frame protocol defined in boot_protocol.h. M_INT (PC4) is
 * raised so the host knows the bootloader is ready.
 ******************************************************************************
 */

#include "main.h"
#include "boot_flag.h"
#include "boot_flash.h"
#include "boot_jump.h"
#include "boot_log.h"
#include "boot_protocol.h"
#include "boot_spi.h"

static void SystemClock_Config(void);
static void Upgrade_Loop(void) __attribute__((noreturn));

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  Boot_Log_Init();
  Boot_Log_Printf("MIPI CMD bootloader v%u (%s %s)",
                  BOOT_PROTOCOL_VERSION, __DATE__, __TIME__);

  /* Decide where to go: upgrade request beats a valid application */
  if (Boot_IsUpgradeRequested())
  {
    Boot_ClearUpgradeRequest(); /* Consumed; a failed upgrade just re-enters here */
    Boot_Log_Printf("upgrade flag detected -> upgrade mode");
  }
  else if (Boot_AppIsValid())
  {
    Boot_Log_Printf("valid app @0x%08lX, reset=0x%08lX -> booting",
                    (uint32_t)BOOT_APP_ADDRESS,
                    *(__IO uint32_t *)(BOOT_APP_ADDRESS + 4U));
    Boot_JumpToApp(); /* Never returns */
  }
  else
  {
    Boot_Log_Printf("no valid app (MSP=0x%08lX reset=0x%08lX) -> upgrade mode",
                    *(__IO uint32_t *)BOOT_APP_ADDRESS,
                    *(__IO uint32_t *)(BOOT_APP_ADDRESS + 4U));
  }

  Upgrade_Loop(); /* Never returns */

  while (1)
  {
    /* Never reached */
  }
}

/**
 * @brief  Upgrade mode: SPI2 slave + command loop.
 */
static void Upgrade_Loop(void)
{
  uint8_t rx_frame[BOOT_FRAME_LEN];
  uint8_t tx_frame[BOOT_FRAME_LEN];

  Boot_Spi_Init();
  Boot_Log_Printf("upgrade mode ready: SPI2 slave");
  Boot_Log_Printf("------------------------------------------------------------------------------------03\r\n");

  for (;;)
  {
    /* If the host never engages (no frame within ~10s) and a valid app is
       present, boot it instead of waiting forever in upgrade mode. */
    if (Boot_Spi_ReceiveFrame(rx_frame, 10000U) != HAL_OK)
    {
      if (Boot_AppIsValid())
      {
        Boot_Log_Printf("no host activity, booting valid app");
        Boot_Spi_DeInit();
        Boot_JumpToApp(); /* Never returns */
      }
      continue; /* No valid app: keep waiting for the host */
    }

    /* M_INT low while processing: the host waits for the LOW->HIGH transition
       before clocking out the dummy read. Enforce a minimum 2 ms busy window
       so fast commands (SYNC) are still reliably observed as low->high. */
    Boot_Log_Printf("Boot_Spi_SignalBusy\r\n");
    Boot_Spi_SignalBusy();
    uint32_t busy_start = HAL_GetTick();
    uint8_t jump = Boot_Protocol_Handle(rx_frame, tx_frame);
    Boot_Log_Printf("cmd 0x%02X -> status 0x%02X", rx_frame[1], tx_frame[2]);
    while (HAL_GetTick() - busy_start < 2U)
    {
    }

    /* Arm the response non-blocking and raise M_INT: the host sees the high
       level, then its dummy read transaction clocks the response out. */
    Boot_Spi_ArmResponse(tx_frame);
    Boot_Log_Printf("Boot_Spi_SignalReady\r\n");
    Boot_Spi_SignalReady();
    Boot_Spi_WaitResponseDone(5000U);

    if (jump)
    {
      Boot_Log_Printf("jumping to application @0x%08lX",
                      (uint32_t)BOOT_APP_ADDRESS);
      Boot_Spi_DeInit();
      Boot_JumpToApp(); /* Never returns */
    }
  }
}

/**
 * @brief  System clock configuration: 168 MHz, identical to the application
 *         so the peripherals keep the same timing in both images.
 */
static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON; /* 8 MHz crystal */
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  Boot_Log_Printf("ERROR: Error_Handler entered, halting");
  __disable_irq();
  while (1)
  {
  }
}
