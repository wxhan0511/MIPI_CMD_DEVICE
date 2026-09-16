# MIPI_Cmd_Device 代码实施细节说明

## 2. Bootloader（IAP）方案实施细节

### 2.1 Flash 分区（STM32F407VGT6，1MB）

```text
0x08000000 ┬ Bootloader   64KB  Sector 0~3   （独立工程 bootloader/）
0x08010000 ┼ Application  832KB Sector 4~10  （本仓库固件，LVGL 使能后实测 489KB）
0x080E0000 ┼ Version data 12KB  Sector 11    （fw/hw 名称与版本，88B）
```

**两个关键设计决策（与最初方案不同处）**：

1. **版本区从 0x0807D000 迁到 Sector 11**：F407 只能整扇区擦除，原位置落在 Sector 7 内部——Bootloader 擦 App（必然覆盖 Sector 7）会把版本数据一起擦掉。迁到独立扇区后，升级擦除永远不会碰它。
2. **App 区扩到 832KB（Sector 4~10）**：本仓库固件使能 LVGL 界面后实测 489KB，超出最初方案的 448KB。App 实际占用 0x08010000~0x08087788，其余空间留作增长。

链接脚本（`STM32F407VGTX_FLASH.ld`）：

```ld
MEMORY
{
  CCMRAM        (xrw) : ORIGIN = 0x10000000, LENGTH = 64K
  RAM           (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
  FLASH         (rx)  : ORIGIN = 0x08010000, LENGTH = 0xD0000
  FLASH_VERSION (rx)  : ORIGIN = 0x080E0000, LENGTH = 12K
}
/* .const_section 定位同步改为 0x080E0000，KEEP(.fw_version/.fw_name/.hw_version/.hw_name) */
```

### 2.2 升级标志：RTC 备份寄存器（Bsp/boot_flag.c/.h）

- 载体：`RTC->BKP0R`（VBAT 域，复位/断电不丢，不占 Flash）；魔数 `BOOT_MAGIC 0x5AA55AA5`；
- **寄存器级实现，不依赖 HAL RTC 模块/CubeMX RTC 配置**（本工程原本没有 RTC）：

```c
void Boot_BkUpInit(void)
{
  __HAL_RCC_PWR_CLK_ENABLE();
  SET_BIT(PWR->CR, PWR_CR_DBP);              /* 解锁备份域 */
  if (READ_BIT(RCC->BDCR, RCC_BDCR_RTCEN) == 0U) {
    SET_BIT(RCC->CSR, RCC_CSR_LSION);        /* LSI 作 RTC 时钟（仅驱动备份寄存器，无日历） */
    while (READ_BIT(RCC->CSR, RCC_CSR_LSIRDY) == 0U) { }
    MODIFY_REG(RCC->BDCR, RCC_BDCR_RTCSEL, RCC_BDCR_RTCSEL_1);
    SET_BIT(RCC->BDCR, RCC_BDCR_RTCEN);
  }
}
```

- 三个 API：`Boot_RequestUpgrade()`（写标志+`NVIC_SystemReset()`，不返回）、`Boot_IsUpgradeRequested()`、`Boot_ClearUpgradeRequest()`；
- Bootloader 侧 `bootloader/Core/Src/boot_flag.c` 逻辑完全一致（常量需两侧同步，见 §4）。

### 2.3 App 侧改动（逐文件）

| 文件 | 改动内容 |
| ---- | -------- |
| `STM32F407VGTX_FLASH.ld` | FLASH 起点 0x08000000→0x08010000、长度 0xD0000；FLASH_VERSION→0x080E0000 |
| `CMakeLists.txt` | `add_definitions(-DUSER_VECT_TAB_ADDRESS -DVECT_TAB_OFFSET=0x10000U)` —— CubeMX 的 `system_stm32f4xx.c` 据此在 `SystemInit()` 里执行 `SCB->VTOR = FLASH_BASE \| VECT_TAB_OFFSET` = 0x08010000 |
| `Bsp/boot_flag.c/.h`（新增） | §2.2 的升级标志实现 |
| `Task/task_sample.h` | 枚举新增 `CMD_ENTER_BOOT = 0x27`（0x10~0x26 已占用，与 Bootloader 的 SYNC 共用同值） |
| `Task/task_sample.c` | ① 新增 `static volatile uint8_t upgrade_pending`；② 新增 0x27 case：回 ACK → `task_com_resume()` → 置 pending；③ 主循环顶部安全点执行关机+复位（见下） |
| `Bsp/bsp.c` | 版本打印改有界 `%.*s`（§1.5） |
| `Core/Src/stm32f4xx_it.c` | 故障处理器增强（§1.4） |

**升级命令的完整执行时序**（`task_sample_run` 内部）：

```c
/* 1) 命令阶段：收到 0xA0 0x27 */
case CMD_ENTER_BOOT:
    meter_tx_buf[2] = g_sample_task.cmd_status;   /* ACK */
    task_com_resume();                            /* 把 ACK 帧交 SPI 层发送 */
    upgrade_pending = 1;                          /* 不立即复位！ */
    g_sample_task.cmd_type = NORMAL_LOOP_EVENT;
    break;

/* 2) 任务循环顶部安全点（for(;;) 开头、switch 之前） */
if (upgrade_pending) {
    upgrade_pending = 0;
    disableTim1PWMOutput();
    disableTim2PWMOutput();
    for (uint8_t i = 0; i < 20; i++)
        bsp_power_single_disable(i);   /* 关全部 20 路电源 */
    osDelay(100);                      /* 等 64B ACK 帧从 SPI2 发完 */
    Boot_RequestUpgrade();             /* 写 BKP0R + 复位（不返回） */
}
```

> 不在收到命令时立即复位的原因：设备带 20 路电源、多路 PWM、DMA 采样，直接复位会让
> Bootloader 接管时硬件处于中间态。先安全断电、ACK 发完，再走。

### 2.4 Bootloader 工程（bootloader/，独立 CMake 工程）

```text
bootloader/
├── CMakeLists.txt                  # 复用父目录 HAL 驱动（../Drivers），-Os 裁剪
├── STM32F407VGTX_BOOT.ld           # FLASH 0x08000000 / 64K
├── README.md                       # 英文协议手册
└── Core/
    ├── Inc/  main.h stm32f4xx_hal_conf.h stm32f4xx_it.h
    │         boot_flag.h boot_flash.h boot_jump.h boot_spi.h boot_protocol.h boot_log.h
    └── Src/  main.c system_stm32f4xx.c stm32f4xx_it.c syscalls.c sysmem.c
              boot_flag.c boot_flash.c boot_jump.c boot_spi.c boot_protocol.c boot_log.c
```

各模块职责与要点：

- **main.c**：启动决策树（升级标志 > App 合法性 > 停在升级模式）；`SystemClock_Config` 与 App 相同（HSE 8MHz→PLL→168MHz）；升级模式初始化 SPI2 后拉高 `M_INT`(PC4) 通知主机；
- **boot_jump.c**：`Boot_AppIsValid()` 校验（初始 MSP 在 SRAM 内且**允许 == RAM 顶**——App 的 `_estack = 0x20020000` 恰好等于 RAM 顶，这里曾是 `strictly less than` 导致永远判无效、无法跳转，已修复为 `<=`）；`Boot_JumpToApp()`：停 SysTick → 关中断清 pending → `SCB->VTOR = 0x08010000` → `__set_MSP` → `__enable_irq()`（CubeMX App 不会自己开中断）→ 跳转；
- **boot_flash.c**：F407 扇区表（16K×4 / 64K / 128K×7）；`Boot_FlashEraseRange()` 按地址区间覆盖的整扇区擦除（只允许落在 App 区内）；`Boot_FlashWrite()` 字编程、尾部 0xFF 补齐；
- **boot_spi.c**：SPI2 从机（PB12 NSS 硬件输入 / PB13 SCK / PC2 MISO / PC3 MOSI，模式 0，与 App 完全同接线），轮询收发、无 DMA；
- **boot_protocol.c**：64 字节帧协议，命令 SYNC(0x27)/ERASE(0x28)/WRITE(0x29)/READ(0x2A)/JUMP_APP(0x30)/GET_INFO(0x31)；WRITE 负载 `[3..6]=偏移(4对齐) [7]=长度(≤56) [8..]=数据`；
- **boot_log.c**：USART3@115200（PD8/PD9，与 App 同口）轮询日志：启动横幅、决策结果、每帧 `cmd -> status`、擦写详情、跳转提示——Bootloader 阶段全程可见。

**体积**：Bootloader 33KB/64KB（vsnprintf 约 18KB；日志只许整数/字符串格式，禁 %f）；App 489KB/832KB，RAM 127.6KB/128KB。

### 2.5 端到端升级流程

```text
App 运行中
   │ 主机发 0xA0 0x27（64B）
   ▼
ACK → 关 PWM → 关 20 路电源 → 等 ACK 发完 → BKP0R=0x5AA55AA5 → 复位
   ▼
Bootloader：读标志命中 → 清标志 → 升级模式（M_INT 拉高）
   ▼
主机：SYNC → ERASE(固件长度) → WRITE 循环(56B/帧, 4对齐) → READ 回读比对 → JUMP_APP
   ▼
校验 App 有效 → 跳转 → 新固件运行
```

### 2.6 上位机集成（gcv5_service, LubanCat RK3588）

上位机与电源板仅通过 SPI 通信（LubanCat 为 SPI 主机：`/dev/spidev0.0` + GPIO7.4 片选 + gpiochip1.14 就绪线，与 `hal_power.py` 完全同一条总线）。新增文件（部署于 `/home/cat/gcv5_service/`）：

| 文件 | 职责 |
| ---- | ---- |
| `core/power_board_update.py` | 升级核心：复用 `HalPowerControl` 单例的 SPI 总线与硬件锁（升级帧与正常电源命令永不交错）；实现 Bootloader 协议（SYNC/ERASE/WRITE/READ/JUMP_APP，56B 写块、61B 读块、全量回读校验）；正常运行态版本查询（0x11）；后台线程状态机（prepare→enter_boot→erase→write→verify→jump→version→done），进度/阶段/错误信息供前端轮询 |
| `web/routes/power_board.py` | `/power_board/version`（点击查询运行版本）、`/power_board/upload`（.bin 上传，≤960KB）、`/power_board/start`（启动后台升级，要求 Lua 任务空闲）、`/power_board/status`（进度轮询）；上传/启动仅 admin |
| `web/routes/__init__.py` | 注册 `power_board` 路由模块 |
| `web/templates/update.html` | 系统更新页新增"电源板更新"卡片：当前运行版本（点击查询）+ .bin 上传区 + 升级进度条 + 完成后自动显示新版本号 |

**Bootloader 配套修改（需重烧 boot.hex）**：
- `M_INT` 改为**每命令翻转**（收到命令拉低=处理中，应答发出后拉高=就绪）——上位机"等就绪再读应答"的时序才能覆盖 ERASE 这类长操作；旧版 M_INT 常高，上位机会在读 ERASE 应答时读到垃圾（协议不兼容，**必须重烧**）；
- 擦写边界从 0xD0000 扩到 **0xF0000（Flash 末尾）**：App bin 的尾部 88 字节是版本数据块（位于 Sector 11），整 bin 连同版本一起烧写，升级后版本号才会更新。

### 2.7 常量同步表（改一侧必须同步另一侧）

| 常量 | 值 | App 侧 | Bootloader 侧 |
| ---- | -- | ------ | ------------- |
| BOOT_MAGIC | 0x5AA55AA5 | `Bsp/boot_flag.h` | `Core/Inc/boot_flag.h` |
| 升级/SYNC 命令 | 0x27 | `Task/task_sample.h` | `Core/Inc/boot_protocol.h` |
| App 起始/可刷大小 | 0x08010000 / 0xF0000 | `STM32F407VGTX_FLASH.ld` | `bootloader/Core/Inc/main.h` |
| 版本区 | 0x080E0000 / 12K（随 bin 尾部整体烧写） | `STM32F407VGTX_FLASH.ld` | `bootloader/Core/Inc/main.h` |

---

## 3. 调试实录（三个连环坑与定位方法）

1. **跳转后无任何输出**：`Boot_AppIsValid()` 用 `app_stack < RAM_END` 判断，而 App 初始 MSP 恰好等于 RAM 顶（0x20020000），永远判无效 → Bootloader 静默停在升级模式。修复为 `<=`。**教训：栈顶指针 == RAM 顶是合法初始 MSP。**
2. **跳转成功但 App 横幅后 HardFault（PC/LR=0x08002AF1，BFAR=0x08100000）**：BFAR 恰为 Flash 末尾 → `%s` 扫描未终止字符串 → 芯片上 **0x080E0000 版本扇区未烧录**（擦除态全 0xFF），`printf("%s", fw_name)` 一路扫穿。烧录工具日志同时报 `Programming error @ 0x080E0000`。**教训：换链接布局后必须整 hex 重烧并开启 Verify；打印外部段字符串一律用有界 `%.*s`。**
3. **`Fault Address` 不打印**：BFSR 的 BFARVALID 在 bit7，原代码判断 bit6。已修。

> 通用方法：增强版 HardFault 处理器的 `EXC_RETURN / CFSR / BFAR / Fault PC / Fault LR`
> 五元组足以在无调试器的情况下定位绝大多数总线类故障。

---

## 4. 构建与烧录速查

```bat
:: Bootloader（首次或改动后）
cmake -S bootloader -B bootloader/build -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build bootloader/build          :: → bootloader/build/boot.hex（0x08000000）

:: 应用程序
cmake --build build                     :: → build/build.hex（0x08010000 + 0x080E0000）
```

烧录：**用 hex，不要用 bin**（hex 自带两段地址）。首次：全片擦除 → 烧 boot.hex → 烧 build.hex → Verify 开启。
日常升级：走 SPI 协议（§2.5），无需 SWD。

---

## 5. 已知注意事项

- **RAM 余量 0.6KB**（127.6/128KB，LVGL 使能所致）：再加功能前先看 map；
- **芯片容量**：本方案按 1MB（VGT6）设计。ST-Link 工具若识别为 512KB（VET6 或兼容片，`Revision ID: Unknown` 是常见特征），0x080E0000 无法编程——先读 `0x1FFF7A22`（16 位，KB）确认真实容量，512KB 需另行重划分区（版本区并入 App 镜像 + 固件裁剪）；
- **Bootloader 无 CRC 命令**：主机用 READ 回读比对代替；如需 CRC 可用 F407 CRC 外设追加命令；
- **升级中途断电**：下次上电 App 无效 → Bootloader 留在升级模式重走流程，无变砖风险；
- Bootloader 日志禁用 `%f`（浮点 printf 会撑爆 64KB 预算）。
