# Bootloader — MIPI CMD Board (STM32F407VGT6)

Standalone IAP bootloader that receives application firmware over the same
SPI2 slave interface the application uses, with the host PC acting as SPI
master.

## Flash partition

| Region       | Address range            | Size  | Notes                          |
| ------------ | ------------------------ | ----- | ------------------------------ |
| Bootloader   | 0x08000000 - 0x0800FFFF  | 64 KB | Sectors 0-3, this project      |
| Application  | 0x08010000 - 0x080DFFFF  | 832 KB| Sectors 4-10, erased on update |
| Version data | 0x080E0000 - 0x080E2FFF  | 12 KB | Sector 11, never erased        |

The upgrade request is carried in the RTC backup register `BKP0R`
(magic `0x5AA55AA5`), which survives reset and power loss without consuming
any flash sector.

## Boot decision tree

1. `BKP0R == BOOT_MAGIC`?  -> clear the flag, enter upgrade mode.
2. Application valid (stack pointer inside SRAM, reset vector inside the
   application slot)?  -> jump to it.
3. Otherwise -> enter upgrade mode.

In upgrade mode the bootloader raises `M_INT` (PC4) to tell the host it is
ready, then listens on SPI2 (PB12 NSS / PB13 SCK / PC2 MISO / PC3 MOSI,
mode 0, hardware NSS, 8-bit).

## Console log

The bootloader prints to the **same console as the application**:
USART3 @115200 8N1, PD8=TX / PD9=RX (blocking polling output, no IRQ/DMA).
Boot-time lines include the banner, the boot decision (`upgrade flag
detected`, `valid app ... -> booting`, `no valid app ... -> upgrade mode`),
one `cmd 0xNN -> status 0xNN` line per received frame, erase/write details
and the final `jumping to application` line.

## Host protocol

Two-phase simplex per command. Control commands use a 64-byte record. The
bootloader can negotiate a 1024-byte data record with `SET_FRAME` (0x32):

1. Host clocks out a command frame (slave receives).
2. Bootloader processes the command, then the host clocks in a response of
   the negotiated frame length with a dummy read transaction.

Frame layout (both directions): `[0]=0xA0` head, `[1]=command`,
`[2]=status`, `[3..]`=payload, unused bytes `0x00`.

| Cmd   | Name     | Payload (LE)                                   | Response payload            |
| ----- | -------- | ---------------------------------------------- | --------------------------- |
| 0x27  | SYNC     | none                                           | [3]=proto ver, [4]=app valid, [5..8]=max app size |
| 0x28  | ERASE    | [3..6]=app size to erase (0 = whole slot)      | status                      |
| 0x29  | WRITE    | [3..6]=offset (4-byte aligned), [7..8]=data length (<= 1015), [9..]=data | status |
| 0x2A  | READ     | [3..6]=offset, [7..8]=length (<= 1021)         | data at [3..]               |
| 0x30  | JUMP_APP | none                                           | status, then device boots app |
| 0x31  | GET_INFO | none                                           | same as SYNC                |
| 0x32  | SET_FRAME | [3..4]=frame length (64 or 1024)              | status                      |

Status codes: `0x00` OK, `0x01` generic error, `0x02` bad address/alignment,
`0x03` flash operation failed, `0x04` bad length, `0x05` bad frame head.

`SYNC` reports the maximum frame length in response bytes `[9..10]`. Hosts
should send `SET_FRAME` while still using the 64-byte control frame, then use
the negotiated size for `WRITE` and `READ`. If `SET_FRAME` is rejected, keep
using 64-byte frames for compatibility.

Note: `SYNC` uses the same byte (`0x27`) as the application's
`CMD_ENTER_BOOT`, so the host can keep one constant for the whole flow.

## Application-side flow (MIPI_Cmd_Device project)

1. Host sends `CMD_ENTER_BOOT` (0x27) over SPI2.
2. The application ACKs, then at a safe point in the sampling task:
   disables both PWM outputs, turns off all 20 power rails, drains the ACK
   frame, writes the boot flag and resets (`Boot_RequestUpgrade()`).
3. After reset the bootloader consumes the flag and waits for the upgrade
   frames above.

## Build

```bat
cmake -S bootloader -B bootloader/build -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build bootloader/build
```

Output: `bootloader/build/boot.elf` / `boot.hex` / `boot.bin`
(FLASH 64 KB budget, current usage ~14.7 KB).

## First-time flashing

1. Erase the chip, flash `boot.hex` (goes to 0x08000000 by address).
2. Flash the application `build.hex` from the main project — its vector
   table now links at 0x08010000 and the version data at 0x080E0000, so a
   plain address-based flash works.
3. The next reset boots through the bootloader into the application.
