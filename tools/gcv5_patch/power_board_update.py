"""电源板 (STM32) 固件在线升级核心逻辑.

SPI 传输完全复用 hal_power 的生产实现 (HalPowerControl._spi_exchange64):
同一 /dev/spidev0.0 总线、同一 GPIO 片选与 M_INT 就绪线、同一硬件锁串行化、
等待期间同样的 64B 总线 poke 节拍 —— 与读电压/读电流等命令同源, 保证重复
调用不破坏从机 HAL 状态机.

协议 (与 MIPI_Cmd_Device 仓库 bootloader/Core/Src/boot_protocol.c 完全一致):

    帧长 64 字节: [0]=0xA0 帧头, [1]=命令, [2]=状态, [3..]=负载(小端), 余量补 0
    两阶段单工: 主机发命令帧 -> 等 M_INT(处理完成) -> 主机敲时钟读 64B 应答帧

正常运行态 (App 固件):
    0x11 GET_SW_VERSION  应答 [3..6] = sw_version 4 字节 (major.minor.patch.build)
    0x27 CMD_ENTER_BOOT  App 回 ACK 后约 100ms 软复位进入 Bootloader

Bootloader 态:
    0x27/0x31 SYNC/GET_INFO  [3]=协议版本 [4]=app有效 [5..8]=u32 最大可刷字节数
    0x28 ERASE               [3..6]=u32 长度 (按扇区对齐覆盖)
    0x29 WRITE               [3..6]=u32 offset [7]=len(<=56) [8..]=数据
    0x2A READ                [3..6]=u32 offset [7..8]=u16 len(<=61) -> [3..]=数据
    0x30 JUMP_APP            应答后设备自行跳转新固件
"""

from __future__ import annotations

import os
import struct
import threading
import time

from core.logger import logger
from hal.hal_base import StatusCode

# 与 bootloader/Core/Inc/main.h 的 BOOT_APP_ADDRESS / BOOT_APP_MAX_SIZE 同步
PB_APP_ADDRESS = 0x08010000
PB_FLASH_MAX = 0xF0000           # App 起点到 Flash 末尾 (含尾部 88B 版本块)
PB_BLOCK_WRITE = 56              # WRITE 单帧最大数据长度
PB_BLOCK_READ = 61               # READ 单帧最大数据长度
PB_BIN_PATH = "/tmp/power_board_fw.bin"

# 命令字 (App 态与 Bootloader 态共用 0x27, 见 boot_protocol.h 注释)
CMD_GET_SW_VERSION = 0x11
CMD_ENTER_BOOT = 0x27
CMD_SYNC = 0x27
CMD_ERASE = 0x28
CMD_WRITE = 0x29
CMD_READ = 0x2A
CMD_JUMP_APP = 0x30
CMD_GET_INFO = 0x31

BOOT_STATUS_OK = 0x00


class PowerBoardUpdateError(Exception):
    """升级过程中的业务错误, 信息可直接展示给用户."""


_init_lock = threading.Lock()  # Module-level singleton lock (shared by all threads)


def get_power_board_updater() -> "PowerBoardUpdater":
    """进程内单例入口 (延迟初始化, 不触碰 Flask 启动流程)."""
    global _updater
    try:
        return _updater
    except NameError:
        pass
    with _init_lock:
        try:
            return _updater
        except NameError:
            _updater = PowerBoardUpdater()
            return _updater


class PowerBoardUpdater:
    """电源板固件升级器: SPI 传输 + 升级状态机 + 后台线程."""

    def __init__(self):
        # HalPowerControl 是单例: 直接借用其生产级 SPI 传输 (含硬件锁/poke 节拍),
        # 升级期间 Lua 任务发起的电源命令经由同一把硬件锁自然串行化.
        from hal.hal_power import HalPowerControl

        self._power = HalPowerControl()
        self._frame = 64

        self._state_lock = threading.Lock()
        self._thread = None
        self._bin_path = None
        self._state = {
            "running": False,
            "stage": "idle",
            "percent": 0,
            "message": "待机",
            "ok": None,
            "version": None,
            "error": None,
            "bin_size": 0,
        }

    # ------------------------------------------------------------------ #
    # 状态
    # ------------------------------------------------------------------ #
    def get_state(self) -> dict:
        with self._state_lock:
            return dict(self._state)

    def _set_state(self, **kw):
        with self._state_lock:
            self._state.update(kw)

    # ------------------------------------------------------------------ #
    # SPI 传输 —— 完全复用 hal_power 的生产实现
    # ------------------------------------------------------------------ #
    def _exchange(self, cmd: int, payload: bytes = b""):
        """两阶段单工交换: 直接走 HalPowerControl._spi_exchange64 (生产验证过的传输).

        返回 (status, 64B 应答帧 list), 帧格式 [0]=0xA0 [1]=cmd [2]=status [3..]=负载.
        """
        status, rx = self._power._spi_exchange64(
            [0xA0, cmd], list(payload), read_len=self._frame)
        return status, list(rx)

    def _boot_cmd(self, cmd: int, payload: bytes = b"") -> list:
        """Bootloader 命令: 专用两阶段交换 (必须观察到 M_INT 低->高跳变才读应答,
        防止轮询到上一次残留的空闲高电平而抢读).

        时序: 等 M_INT 高(空闲, 带 poke) -> 发命令帧 -> 等 M_INT 低(处理中) ->
        等 M_INT 高(应答已挂载) -> 敲时钟读 64B 应答帧.
        Bootloader 侧保证处理中低电平窗口 >= 2ms, 1ms 轮询必然能观察到跳变."""
        frame = bytes([0xA0, cmd, 0x00]) + bytes(payload)
        if len(frame) > self._frame:
            raise PowerBoardUpdateError(f"命令帧超长: {len(frame)}")
        frame = frame + b"\x00" * (self._frame - len(frame))

        io, spi = self._power.io_bus, self._power.spi_bus
        ready = self._power.ready_line_num

        with self._power._hw_lock:
            # 1. 等空闲高电平 (带 poke, 与 hal_power 相同的节拍)
            t0 = time.time()
            while io.get_value(ready) != 1:
                if time.time() - t0 > 10.0:
                    raise PowerBoardUpdateError(
                        "电源板未就绪 (M_INT 等待超时), 请检查电源板连接")
                spi.transfer([0xBB] * self._frame)
                time.sleep(0.001)

            # 2. 发送命令帧
            spi.transfer(list(frame))

            # 3. 等待 低->高 跳变 (低=处理中, 高=应答已挂载)
            t0 = time.time()
            seen_low = False
            while time.time() - t0 < 15.0:
                v = io.get_value(ready)
                if v == 0:
                    seen_low = True
                elif seen_low and v == 1:
                    break
                time.sleep(0.001)
            else:
                raise PowerBoardUpdateError(
                    f"Bootloader 命令 0x{cmd:02X} 处理超时 (15s)")

            # 4. 读应答帧
            rx = list(spi.transfer([0x00] * self._frame))

        if len(rx) < 3 or rx[2] != BOOT_STATUS_OK:
            raise PowerBoardUpdateError(
                f"Bootloader 命令 0x{cmd:02X} 失败 (status=0x{rx[2]:02X})")
        if len(rx) < 9 or rx[0] != 0xA0 or rx[1] != cmd:
            raise PowerBoardUpdateError(
                f"Bootloader 应答帧头不符: got 0x{rx[0]:02X} 0x{rx[1]:02X}")
        return rx

    # ------------------------------------------------------------------ #
    # 版本查询 (正常运行态)
    # ------------------------------------------------------------------ #
    def read_running_version(self):
        """查询电源板当前运行固件版本; 无应答/超时抛错, 未上报返回 None."""
        status, rx = self._exchange(CMD_GET_SW_VERSION)
        if status == StatusCode.TIMEOUT or len(rx) < 7:
            raise PowerBoardUpdateError("电源板无应答 (SPI 超时), 请检查电源板状态")
        raw = bytes(rx[3:7])
        if raw in (b"\x00\x00\x00\x00", b"\xff\xff\xff\xff"):
            return None
        return ".".join(str(b) for b in raw)

    # ------------------------------------------------------------------ #
    # Bootloader 协议
    # ------------------------------------------------------------------ #
    def _enter_boot(self):
        """正常运行态 -> Bootloader 态: 发 0x27, 等 App 复位并重试 SYNC 握手."""
        logger.info("power_board_update: sending CMD_ENTER_BOOT")
        self._exchange(CMD_ENTER_BOOT)
        time.sleep(2.0)  # App 延时 100ms 后复位, Bootloader 数百 ms 内就绪

        last_err = None
        for _ in range(10):
            try:
                rx = self._boot_cmd(CMD_SYNC)
                proto_ver = rx[3]
                app_valid = rx[4]
                max_size = struct.unpack("<I", bytes(rx[5:9]))[0]
                logger.info(
                    "power_board_update: bootloader ready, proto=v%s app_valid=%s max=%#x",
                    proto_ver, bool(app_valid), max_size)
                return
            except PowerBoardUpdateError as e:
                last_err = e
                time.sleep(0.5)
        raise PowerBoardUpdateError(
            f"未能进入 Bootloader (请确认电源板已烧录 bootloader 固件): {last_err}")

    # ------------------------------------------------------------------ #
    # 升级状态机 (后台线程)
    # ------------------------------------------------------------------ #
    def save_bin(self, stream, filename: str) -> dict:
        """保存上传的 .bin 固件, 返回 {path, size}."""
        if self.get_state().get("running"):
            raise PowerBoardUpdateError("升级进行中, 禁止上传新固件")
        if not filename.lower().endswith(".bin"):
            raise PowerBoardUpdateError("请上传 .bin 固件文件")
        data = stream.read()
        if not data:
            raise PowerBoardUpdateError("固件文件为空")
        if len(data) > PB_FLASH_MAX:
            raise PowerBoardUpdateError(
                f"固件过大: {len(data)} 字节, 上限 {PB_FLASH_MAX} 字节 (960KB)")
        with open(PB_BIN_PATH, "wb") as f:
            f.write(data)
        self._bin_path = PB_BIN_PATH
        self._set_state(bin_size=len(data))
        logger.info("power_board_update: saved firmware %s (%d bytes)",
                    filename, len(data))
        return {"path": PB_BIN_PATH, "size": len(data)}

    def start_update(self, path: str = None):
        with self._state_lock:
            if self._state.get("running"):
                raise PowerBoardUpdateError("已有升级任务在进行中")
            bin_path = path or self._bin_path
            if not bin_path or not os.path.exists(bin_path):
                raise PowerBoardUpdateError("尚未上传固件文件")
            self._state.update({
                "running": True, "stage": "prepare", "percent": 0,
                "message": "准备升级...", "ok": None, "version": None,
                "error": None, "bin_size": os.path.getsize(bin_path),
            })
            self._thread = threading.Thread(
                target=self._run_update, args=(bin_path,), daemon=True)
            self._thread.start()

    def _run_update(self, bin_path: str):
        try:
            version = self._update_flow(bin_path)
            self._set_state(running=False, stage="done", percent=100, ok=True,
                            version=version,
                            message=(f"电源板升级成功, 当前版本 v{version}"
                                     if version else
                                     "电源板升级成功 (新固件未上报版本号, 需要单片机端实现 0x11 命令)"))
            logger.info("power_board_update: success, version=%s", version)
        except Exception as e:
            logger.exception("power_board_update: failed")
            self._set_state(running=False, stage="error", ok=False,
                            error=str(e), message=f"升级失败: {e}")

    def _update_flow(self, bin_path: str):
        data = open(bin_path, "rb").read()
        if not data:
            raise PowerBoardUpdateError("固件文件为空")
        if len(data) > PB_FLASH_MAX:
            raise PowerBoardUpdateError(
                f"固件过大: {len(data)} 字节, 上限 {PB_FLASH_MAX} 字节")

        # 1. 进入 Bootloader
        self._set_state(stage="enter_boot", percent=2,
                        message="通知电源板进入 Bootloader...")
        self._enter_boot()

        # 2. 整区擦除 (按扇区覆盖 [App起点, App起点+固件长度))
        self._set_state(stage="erase", percent=8,
                        message=f"擦除 Flash ({len(data)} 字节, 约 1~5 秒)...")
        self._boot_cmd(CMD_ERASE, struct.pack("<I", len(data)))

        # 3. 逐块写入
        self._set_state(stage="write", percent=15, message="写入固件...")
        offset = 0
        while offset < len(data):
            chunk = data[offset:offset + PB_BLOCK_WRITE]
            self._boot_cmd(CMD_WRITE,
                           struct.pack("<I", offset) +
                           bytes([len(chunk)]) + chunk)
            offset += len(chunk)
            self._set_state(percent=15 + int(55 * offset / len(data)),
                            message=f"写入固件 {offset}/{len(data)} 字节")

        # 4. 回读校验
        self._set_state(stage="verify", percent=72, message="回读校验...")
        offset = 0
        while offset < len(data):
            n = min(PB_BLOCK_READ, len(data) - offset)
            rx = self._boot_cmd(CMD_READ,
                                struct.pack("<I", offset) + struct.pack("<H", n))
            if bytes(rx[3:3 + n]) != data[offset:offset + n]:
                raise PowerBoardUpdateError(f"回读校验失败 @ 偏移 0x{offset:X}")
            offset += n
            self._set_state(percent=72 + int(20 * offset / len(data)),
                            message=f"回读校验 {offset}/{len(data)} 字节")

        # 5. 跳转新固件
        self._set_state(stage="jump", percent=94, message="跳转新固件...")
        self._boot_cmd(CMD_JUMP_APP)

        # 6. 等新固件起来后查询版本
        self._set_state(stage="version", percent=97, message="等待电源板重启...")
        time.sleep(3.0)
        version = None
        for _ in range(10):
            try:
                version = self.read_running_version()
                if version:
                    break
            except PowerBoardUpdateError:
                pass
            time.sleep(1.0)

        self._set_state(percent=99)
        return version
