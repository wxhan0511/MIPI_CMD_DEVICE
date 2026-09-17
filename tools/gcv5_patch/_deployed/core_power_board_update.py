"""电源板 (STM32) 固件在线升级核心逻辑.

继承 HalBase，和 HalPowerControl 共用 /dev/spidev0.0、GPIO 片选、M_INT
就绪线及同一把硬件锁，升级与普通电源命令不会在总线上交叉。

协议 (与 MIPI_Cmd_Device 仓库 bootloader/Core/Src/boot_protocol.c 完全一致):

控制帧长 64 字节; 升级数据帧协商后为 1024 字节: [0]=0xA0 帧头,
[1]=命令, [2]=状态, [3..]=负载(小端), 余量补 0
    两阶段单工: 主机发命令帧 -> 等 M_INT(处理完成) -> 主机按当前帧长读应答帧

正常运行态 (App 固件):
    0x11 GET_SW_VERSION  应答 [3..6] = sw_version 4 字节 (major.minor.patch.build)
    0x27 CMD_ENTER_BOOT  App 回 ACK 后约 100ms 软复位进入 Bootloader

Bootloader 态:
    0x27/0x31 SYNC/GET_INFO  [3]=协议版本 [4]=app有效 [5..8]=u32 最大可刷字节数
    0x28 ERASE               [3..6]=u32 长度 (按扇区对齐覆盖)
    0x32 SET_FRAME           [3..4]=u16 frame length (64 or 1024)
    0x29 WRITE               [3..6]=u32 offset [7..8]=u16 len(<=1015) [9..]=数据
    0x2A READ                [3..6]=u32 offset [7..8]=u16 len(<=1021) -> [3..]=数据
    0x30 JUMP_APP            应答后设备自行跳转新固件
"""

from __future__ import annotations

import os
import struct
import threading
import time
import zlib

from core.logger import logger
from hal.hal_base import HalBase, StatusCode

# 与 bootloader/Core/Inc/main.h / boot_version.h 同步
PB_APP_ADDRESS = 0x08010000
PB_FLASH_MAX = 0x80000           # App 槽 512 KB
PB_META_OFFSET = 0x7FFF0         # 16 字节元数据块在 app 槽内的偏移 (512 KB - 16 B)
PB_CONTROL_FRAME = 64
PB_FAST_FRAME = 4096
PB_FAST_WRITE = 4084  # 4096 - 4 偏移 - 2 长度 - 2 帧头 - 4 空余，按 4 对齐取 4084
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
CMD_SET_FRAME = 0x32
CMD_BACKUP = 0x33
CMD_RESTORE = 0x34
CMD_GET_BACKUP_INFO = 0x35
CMD_VERIFY = 0x36

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


class PowerBoardUpdater(HalBase):
    """电源板固件升级器: SPI 传输 + 升级状态机 + 后台线程."""

    def __init__(self):
        super().__init__(
            spi_dev="/dev/spidev0.0",
            cs_port="7",
            cs_line=4,
        )
        self._frame = PB_CONTROL_FRAME
        self._write_block = PB_CONTROL_FRAME - 8
        self._read_block = PB_CONTROL_FRAME - 3

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
            "crc": None,
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
    # SPI 传输 —— 复用 HalBase 的总线、锁和握手实现
    # ------------------------------------------------------------------ #
    def _exchange(self, cmd: int, payload: bytes = b""):
        """正常 App 命令沿用 HalBase 的固定 64 字节交换。

        返回 (status, 应答帧 list), 帧格式 [0]=0xA0 [1]=cmd [2]=status [3..]=负载.
        """
        status, rx = self._spi_exchange64(
            [0xA0, cmd], list(payload), read_len=PB_CONTROL_FRAME)
        return status, list(rx)

    def _boot_cmd(self, cmd: int, payload: bytes = b"") -> list:
        """Bootloader 命令: 专用两阶段交换 (必须观察到 M_INT 低->高跳变才读应答,
        防止轮询到上一次残留的空闲高电平而抢读).

        时序: 等 M_INT 高(空闲, 带 poke) -> 发命令帧 -> 等 M_INT 低(处理中) ->
        等 M_INT 高(应答已挂载) -> 按当前帧长敲时钟读应答帧.
        Bootloader 侧保证处理中低电平窗口 >= 2ms, 1ms 轮询必然能观察到跳变."""
        status, rx = self._spi_exchange_frame(
            [0xA0, cmd, 0x00],
            list(payload),
            frame_size=self._frame,
            read_len=self._frame,
            timeout=60.0 if cmd in (CMD_ERASE, CMD_BACKUP, CMD_RESTORE) else 15.0,
            require_busy_edge=True,
        )
        if status == StatusCode.TIMEOUT:
            raise PowerBoardUpdateError(
                f"Bootloader 命令 0x{cmd:02X} 处理超时")
        if len(rx) < 3 or rx[2] != BOOT_STATUS_OK:
            response_status = rx[2] if len(rx) > 2 else 0xFF
            raise PowerBoardUpdateError(
                f"Bootloader 命令 0x{cmd:02X} 失败 "
                f"(status=0x{response_status:02X})")
        if len(rx) < 9 or rx[0] != 0xA0 or rx[1] != cmd:
            raise PowerBoardUpdateError(
                f"Bootloader 应答帧头不符: got 0x{rx[0]:02X} 0x{rx[1]:02X}")
        return rx

    def _verify_crc(self):
        """校验 Flash CRC, 返回 (ok, computed, stored) 用于诊断。"""
        status, rx = self._spi_exchange_frame(
            [0xA0, CMD_VERIFY, 0x00], [],
            frame_size=self._frame, read_len=self._frame,
            timeout=15.0, require_busy_edge=True)
        if len(rx) < 11:
            return False, None, None
        computed = struct.unpack("<I", bytes(rx[3:7]))[0]
        stored = struct.unpack("<I", bytes(rx[7:11]))[0]
        logger.info(
            "power_board_update: VERIFY raw=%s status=0x%02X computed=0x%08X stored=0x%08X",
            bytes(rx[0:12]).hex(), rx[2], computed, stored)
        return rx[2] == BOOT_STATUS_OK, computed, stored

    def _restore_cmd(self):
        """从备份区还原, 返回 (ok, reason)。"""
        status, rx = self._spi_exchange_frame(
            [0xA0, CMD_RESTORE, 0x00], [],
            frame_size=self._frame, read_len=self._frame,
            timeout=60.0, require_busy_edge=True)
        if len(rx) < 4:
            return False, 0xFF
        return rx[2] == BOOT_STATUS_OK, rx[3]

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
        self._frame = PB_CONTROL_FRAME
        self._write_block = PB_CONTROL_FRAME - 8
        self._read_block = PB_CONTROL_FRAME - 3
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
                try:
                    self._boot_cmd(CMD_SET_FRAME,
                                   struct.pack("<H", PB_FAST_FRAME))
                except PowerBoardUpdateError:
                    logger.info("power_board_update: using 64-byte compatibility frames")
                else:
                    self._frame = PB_FAST_FRAME
                    self._write_block = PB_FAST_WRITE
                    self._read_block = PB_FAST_FRAME - 3
                    logger.info("power_board_update: negotiated %d-byte frames", self._frame)
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
        """保存上传的 .bin 固件, 计算并回填 CRC, 返回 {path, size, crc}."""
        if self.get_state().get("running"):
            raise PowerBoardUpdateError("升级进行中, 禁止上传新固件")
        if not filename.lower().endswith(".bin"):
            raise PowerBoardUpdateError("请上传 .bin 固件文件")
        data = stream.read()
        if not data:
            raise PowerBoardUpdateError("固件文件为空")
        if len(data) != PB_FLASH_MAX:
            raise PowerBoardUpdateError(
                f"固件大小应为 {PB_FLASH_MAX} 字节 (512KB), 实际 {len(data)} 字节")
        # CRC 覆盖 app 数据区 (去掉尾部 16 字节元数据块), 回填到元数据块 [8..11]
        crc = zlib.crc32(data[:PB_META_OFFSET]) & 0xFFFFFFFF
        data = (data[:PB_META_OFFSET + 8] + struct.pack("<I", crc) +
                data[PB_META_OFFSET + 12:])
        with open(PB_BIN_PATH, "wb") as f:
            f.write(data)
        self._bin_path = PB_BIN_PATH
        crc_hex = f"{crc:08X}"
        self._set_state(bin_size=len(data), crc=crc_hex)
        logger.info("power_board_update: saved firmware %s (%d bytes, crc=0x%s)",
                    filename, len(data), crc_hex)
        return {"path": PB_BIN_PATH, "size": len(data), "crc": crc_hex}

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
            crc = self._state.get("crc")  # 文件 CRC (上传时算出, 确定正确)
            crc_msg = f" (文件 CRC 0x{crc}, FLASH CRC 校验一致)" if crc else ""
            self._set_state(running=False, stage="done", percent=100, ok=True,
                            version=version,
                            message=((f"电源板升级成功, 当前版本 v{version}{crc_msg}"
                                      if version else
                                      f"电源板升级成功{crc_msg} "
                                      "(新固件未上报版本号, 需要单片机端实现 0x11 命令)")))
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
            chunk = data[offset:offset + self._write_block]
            if self._frame > PB_CONTROL_FRAME:
                write_payload = (struct.pack("<I", offset) +
                                 struct.pack("<H", len(chunk)) + chunk)
            else:
                write_payload = (struct.pack("<I", offset) +
                                 bytes([len(chunk)]) + chunk)
            self._boot_cmd(CMD_WRITE,
                           write_payload)
            offset += len(chunk)
            self._set_state(percent=15 + int(60 * offset / len(data)),
                            message=f"写入固件 {offset}/{len(data)} 字节")

        # 4. 校验 Flash CRC (与上传文件 CRC 比对)
        self._set_state(stage="verify", percent=72, message="校验 Flash CRC...")
        ok, computed, stored = self._verify_crc()
        if not ok:
            logger.error(
                "power_board_update: CRC mismatch flash=0x%08X file=0x%08X",
                computed or 0, stored or 0)
            raise PowerBoardUpdateError(
                f"Flash CRC 校验失败 (flash=0x{(computed or 0):08X}, "
                f"文件=0x{(stored or 0):08X})")
        self._set_state(flash_crc=f"{(stored or computed or 0):08X}")
        logger.info("power_board_update: CRC match 0x%08X",
                    stored or computed or 0)

        # 5. 备份到外部 flash
        self._set_state(stage="backup", percent=80, message="备份到外部 flash...")
        self._boot_cmd(CMD_BACKUP)

        # 6. 跳转新固件
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

    # ------------------------------------------------------------------ #
    # 从备份区还原 (app 损坏时的网页回退入口)
    # ------------------------------------------------------------------ #
    def start_restore(self):
        with self._state_lock:
            if self._state.get("running"):
                raise PowerBoardUpdateError("已有升级任务在进行中")
            self._state.update({
                "running": True, "stage": "restore", "percent": 0,
                "message": "准备从备份区还原...", "ok": None,
                "version": None, "error": None,
            })
            self._thread = threading.Thread(
                target=self._run_restore, daemon=True)
            self._thread.start()

    def _run_restore(self):
        try:
            version = self._restore_flow()
            self._set_state(running=False, stage="done", percent=100, ok=True,
                            version=version,
                            message=(f"已从备份区还原, 当前版本 v{version}"
                                     if version else
                                     "已从备份区还原 (新固件未上报版本号)"))
            logger.info("power_board_update: restore success, version=%s", version)
        except Exception as e:
            logger.exception("power_board_update: restore failed")
            self._set_state(running=False, stage="error", ok=False,
                            error=str(e), message=f"还原失败: {e}")

    def _restore_flow(self):
        # 1. 进入 Bootloader
        self._set_state(stage="enter_boot", percent=5,
                        message="通知电源板进入 Bootloader...")
        self._enter_boot()

        # 2. 从备份区还原 (bootloader 校验 CRC 后自动跳转)
        self._set_state(stage="restore", percent=30,
                        message="从外部 flash 还原固件 (约 10~30 秒)...")
        ok, reason = self._restore_cmd()
        if not ok:
            reason_text = {
                1: "无可用备份 (请先成功升级一次生成备份)",
                2: "内部 flash 擦除失败",
                3: "外部 flash 读取失败",
                4: "内部 flash 写入失败",
                5: "还原后 CRC 校验失败",
            }.get(reason, f"未知原因 ({reason})")
            raise PowerBoardUpdateError(f"从备份区还原失败: {reason_text}")

        # 3. 等新固件起来后查询版本
        self._set_state(stage="version", percent=95, message="等待电源板重启...")
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
