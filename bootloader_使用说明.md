# Bootloader（IAP 固件升级引导程序）说明与使用手册

适用硬件：**STM32F407VGT6**（1 MB Flash / 128 KB RAM / 64 KB CCMRAM）
适用工程：`MIPI_Cmd_Device`（应用程序，本仓库）+ `bootloader/`（引导程序，独立工程）

---

## 1. 总体架构

```text
                    STM32F407VGT6
                          │ 复位
                          ▼
              ┌───────────────────────┐
              │      Bootloader       │  0x08000000，64 KB（Sector 0~3）
              └───────────┬───────────┘
                          │ 读 RTC 备份寄存器 BKP0R
              ┌───────────┴───────────┐
          有升级标志(0x5AA55AA5)    无标志 且 App 合法
              │                       │
              ▼                       ▼
         升级模式(等 SPI 帧)      跳转到 App 执行
              │                       │
              ▼                       ▼
     主机通过 SPI2 发送升级帧   ┌───────────────────────┐
     ERASE / WRITE / READ      │     Application       │ 0x08010000，832 KB
              │                │   （本仓库固件）        │
              ▼                └───────────┬───────────┘
        JUMP_APP 校验后跳转                │ SPI2 收到 CMD_ENTER_BOOT(0x27)
              │                            ▼
              └──────────── 写升级标志 → 软件复位 → 回到 Bootloader
```

## 2. Flash 分区

| 区域 | 地址范围 | 大小 | 扇区 | 说明 |
| ---- | -------- | ---- | ---- | ---- |
| Bootloader | 0x08000000 ~ 0x0800FFFF | 64 KB | Sector 0~3 | 引导程序，升级时**永不擦除** |
| Application | 0x08010000 ~ 0x080DFFFF | 832 KB | Sector 4~10 | 应用程序，升级时整体擦除重写 |
| Version data | 0x080E0000 ~ 0x080E2FFF | 12 KB | Sector 11 | 固件/硬件版本信息，升级时**永不擦除** |

> ⚠️ 版本区原位于 0x0807D000（在 Sector 7 内部）。因 F407 只能按整扇区擦除，
> 已将版本区迁移到独立的 Sector 11，否则升级擦除 App 时会连带把版本信息擦掉。
> App 区大小为 832 KB 是因为当前 LVGL 界面使能后固件实测约 488 KB。

**升级标志不占 Flash**：存放在 RTC 备份寄存器 `BKP0R`（VBAT 域），断电、
复位均不丢失；Bootloader 消费该标志后立即清零。

## 3. 上电启动决策流程

`bootloader/Core/Src/main.c`：

1. 读 `BKP0R` == `0x5AA55AA5`？→ 清零标志，进入**升级模式**；
2. 否则检查 App 合法性（0x08010000 处的栈指针落在 SRAM 范围、复位向量落在
   App 区且 Thumb 位为 1）→ 合法则**跳转 App**；
3. 否则进入**升级模式**（等主机来升级）。

跳转前已做完整清理：停 SysTick、关全部中断并清 pending、重设 `SCB->VTOR`
与 MSP。升级模式下 bootloader 拉高 `M_INT`（PC4）通知主机"已就绪"。

## 4. 应用程序侧（App）改动一览

| 文件 | 改动 |
| ---- | ---- |
| `STM32F407VGTX_FLASH.ld` | FLASH 起始地址 0x08000000 → **0x08010000**（长度 0xD0000）；版本区迁至 0x080E0000 |
| `CMakeLists.txt` | 增加 `-DUSER_VECT_TAB_ADDRESS -DVECT_TAB_OFFSET=0x10000U`，启动时自动把 `SCB->VTOR` 指到 0x08010000 |
| `Bsp/boot_flag.c/.h`（新增） | 升级标志读写：`Boot_RequestUpgrade()` / `Boot_IsUpgradeRequested()` / `Boot_ClearUpgradeRequest()` |
| `Task/task_sample.h` | 命令枚举新增 `CMD_ENTER_BOOT = 0x27` |
| `Task/task_sample.c` | 新增 0x27 命令处理 + 任务循环安全点执行关机与复位 |

**升级命令的完整执行时序**（App 内部）：

```text
主机发 0xA0 0x27 ...（64字节）
        │
        ▼
task_sample: 回 ACK（meter_tx_buf[2] = 状态字节），task_com_resume() 发送
        │ 置 upgrade_pending = 1
        ▼
任务循环顶部安全点：
    ① disableTim1PWMOutput() / disableTim2PWMOutput()   ← 关 PWM 输出
    ② bsp_power_single_disable(0..19)                    ← 关全部 20 路电源
    ③ osDelay(100)                                       ← 等 ACK 帧从 SPI 发完
    ④ Boot_RequestUpgrade()                              ← 写 BKP0R + 软件复位（不返回）
        │
        ▼
复位后 Bootloader 读到标志 → 进入升级模式 → 拉高 M_INT → 等主机升级帧
```

> 注意：不在 SPI 收到命令时立即复位，避免 DMA/采样/电源输出处于中间态。

## 5. SPI 升级协议（主机 ↔ Bootloader）

**电气接口**与 App 完全一致：主机为 SPI 主机，设备为从机
PB12=NSS（硬件输入）、PB13=SCK、PC2=MISO、PC3=MOSI，
模式 0（CPOL=0，CPHA=1Edge），8 bit，MSB 先行。

**帧格式**：每帧固定 **64 字节**，两个方向一致：

| 字节 | 含义 |
| ---- | ---- |
| [0] | 帧头 `0xA0` |
| [1] | 命令字 |
| [2] | 应答状态码 |
| [3..] | 命令相关负载（小端），未用字节填 `0x00` |

**收发方式**：每个命令两阶段、单工：

1. 主机敲时钟发出 64 字节命令帧（从机接收）；
2. Bootloader 处理完毕后，主机用一次"哑读"事务把 64 字节应答帧读回来。

**命令列表**：

| 命令 | 名称 | 负载（小端） | 应答负载 |
| ---- | ---- | ------------ | -------- |
| 0x27 | SYNC 握手 | 无 | [3]=协议版本(1)，[4]=App 是否有效，[5..8]=App 区最大字节数 |
| 0x28 | ERASE 擦除 | [3..6]=要擦除的 App 字节数（0=整区） | 状态码 |
| 0x29 | WRITE 写入 | [3..6]=App 区内偏移（**必须 4 字节对齐**），[7]=数据长度(≤56)，[8..]=数据 | 状态码 |
| 0x2A | READ 回读校验 | [3..6]=偏移，[7..8]=长度(≤61) | [3..]=回读数据 |
| 0x30 | JUMP_APP 跳转 | 无 | 状态码（主机读到 OK 后设备即跳转 App） |
| 0x31 | GET_INFO | 无 | 同 SYNC |

**状态码**（应答帧 [2]）：

| 值 | 含义 |
| -- | ---- |
| 0x00 | 成功 |
| 0x01 | 一般错误 |
| 0x02 | 地址错误（未对齐/越界） |
| 0x03 | Flash 擦写失败 |
| 0x04 | 长度非法 |
| 0x05 | 帧头错误 |

> SYNC 沿用 `0x27`（与 App 的 `CMD_ENTER_BOOT` 同值），主机侧全程只需一个命令常量。
> WRITE 的最后一块允许不足 56 字节，Bootloader 会用 0xFF 补齐到整字再编程；
> 除最后一块外，建议每块长度取 4 的倍数并保持偏移 4 字节对齐。

**主机侧标准升级流程**：

```text
① SYNC(0x27)                 → 确认 bootloader 在位（M_INT 已拉高也可作为就绪信号）
② ERASE(0x28, size=固件长度)  → 擦除 App 区（整区约需数秒，主机请加大该帧超时）
③ 循环 WRITE(0x29)：offset = 0, 56, 112, ...（4 对齐步进，最后一块为余数）
④ （可选）READ(0x2A) 逐块回读比对
⑤ JUMP_APP(0x30)             → 设备校验后跳入新固件
```

## 6. 编译方法

**Bootloader**（独立工程，复用本仓库的 HAL 驱动）：

```bat
cd D:\Project\MIPI_Cmd_Device
cmake -S bootloader -B bootloader/build -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build bootloader/build --clean-first
```

产物：`bootloader/build/boot.elf / boot.hex / boot.bin`
（当前体积约 33 KB / 64 KB 预算，含 USART3 日志；RAM 约 7.5 KB）

**应用程序**（本仓库，即 App）：

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --clean-first
```

产物：`build/build.hex`（含 0x08010000 的 App 与 0x080E0000 的版本数据两段地址记录）

## 7. 烧录说明

**首次烧录（整板新片 / 全片擦除后）**：

1. 用 ST-Link / J-Link 全片擦除；
2. 烧录 `boot.hex` —— hex 自带地址，直接落在 0x08000000；
3. 烧录 App 的 `build.hex` —— 按地址自动落在 0x08010000 与 0x080E0000；
4. 复位：Bootloader 校验 App 合法后自动跳入应用。

**日常升级（已装 Bootloader 的设备）**：只需走第 5 节的主机升级流程，
无需拆机接线到 SWD；App `build.hex` 可直接作为升级固件源
（解析 hex/ bin 后按 WRITE 协议下发即可）。

**注意**：App 现在从 0x08010000 启动，**调试器直刷 App 时**需确认烧录器
按 hex 地址下载（默认行为）；若用纯 bin 裸烧到 0x08000000 会覆盖 Bootloader。

## 8. 关键常量与同步关系

| 常量 | 值 | App 侧位置 | Bootloader 侧位置 |
| ---- | -- | ---------- | ----------------- |
| BOOT_MAGIC | 0x5AA55AA5 | `Bsp/boot_flag.h` | `bootloader/Core/Inc/boot_flag.h` |
| CMD_ENTER_BOOT / SYNC | 0x27 | `Task/task_sample.h` | `boot_protocol.h` |
| App 起始 / 大小 | 0x08010000 / 0xD0000 | `STM32F407VGTX_FLASH.ld` | `bootloader/Core/Inc/main.h` |
| 版本区 | 0x080E0000 / 12K | `STM32F407VGTX_FLASH.ld`（仅 App 引用） | —（不擦即安全） |

> 修改任一侧常量时必须两侧同步，否则会出现"进不了升级模式"或
> "擦错区域"。

## 9. Bootloader 串口日志与排障

Bootloader 与 App 共用同一个调试串口：**USART3 @115200 8N1（PD8=TX / PD9=RX）**，
上位机一个终端即可看完全程。日志为阻塞式轮询输出，不依赖中断/DMA。

上电后正常日志序列：

```text
MIPI CMD bootloader v1 (Sep 14 2026 10:00:00)          ← 复位后第一条，log 输出即代表 Bootloader 在跑
valid app @0x08010000, reset=0x080192CD -> booting     ← 正常路径：跳入 App，随后应是 App 自己的启动 log
```

若没有跳转而是停在升级模式，会看到（并伴随 M_INT 拉高）：

```text
upgrade flag detected -> upgrade mode                  ← 收到过 CMD_ENTER_BOOT（或标志残留）
no valid app (MSP=0x20020000 reset=0x080192CD) -> upgrade mode   ← App 区无效：没烧/擦了/烧错地址
upgrade mode ready: SPI2 slave, M_INT raised
```

升级过程中的每帧日志：

```text
erasing app area: 0x08010000 + 851968 bytes...         ← ERASE 开始（耗时数秒）
cmd 0x28 -> status 0x00                                ← 每帧命令的执行结果
cmd 0x29 -> status 0x00                                ← WRITE 逐帧（每 56 字节一条，即写进度）
cmd 0x2A -> status 0x00                                ← READ 回读
write failed: offset=... len=...                       ← 编程失败详情
jumping to application @0x08010000                     ← JUMP_APP 成功
ERROR: Error_Handler entered, halting                  ← Bootloader 内部致命错误
```

**快速排障对照**：

| 日志表现 | 结论 / 处理 |
| -------- | ----------- |
| 复位后一条 log 都没有 | Bootloader 没在跑：确认 boot.hex 已烧（0x08000000），检查串口线/波特率 115200 |
| `no valid app ... -> upgrade mode` | App 区无效：烧 App `build.hex`，或 App 烧错地址（应在 0x08010000） |
| `upgrade flag detected`（非预期出现） | 上次升级流程没走完/标志残留，属正常恢复路径；走完一次升级即清 |
| `cmd 0x29 -> status 0x02` | 主机 WRITE 偏移未 4 字节对齐或越界 |
| `cmd 0x28 -> status 0x03` | 擦除失败（供电/时序），重试 |
| `jumping to application` 后 App 无 log | 查 App 本身：确认烧的是新 ld 构建的 `build.hex`，用调试器看是否 HardFault |

## 10. 已知状态与注意事项

- App 当前 RAM 占用 127.6 KB / 128 KB（97.4%，LVGL 界面使能后），已无多少余量；
- Bootloader 串口日志占用 Flash 约 33 KB / 64 KB（vsnprintf 带来约 18 KB），
  仍留有约 31 KB 余量；日志只用整数/字符串格式，勿加 %f；
- Bootloader 不含 CRC 校验命令，主机可使用 READ 回读自行比对；
  如需 CRC，可利用 F407 CRC 外设在协议中追加命令；
- 升级中途断电：Bootloader 会在下次上电时因 App 无效而留在升级模式，
  重新走一遍升级流程即可，无变砖风险（Bootloader 区不会被升级流程擦除）；
- Bootloader 升级模式下会拉高 `M_INT`(PC4)，主机可据此判断设备已进入
  bootloader（App 运行时该引脚按通信协议翻转，语义不同）；
- `boot_flag.c` 使用 LSI 作为 RTC 时钟源（仅用于驱动备份寄存器，无日历功能）；
  若板上后续启用 RTC 日历并改用 LSE，此模块的时钟选择逻辑需同步调整。
