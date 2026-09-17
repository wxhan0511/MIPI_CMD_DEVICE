import os
import struct
import threading
import time
from enum import IntEnum

from core.logger import logger
from hal.spi_driver import SpiDriver
from hal.gpio_driver import GpioDriver
class StatusCode(IntEnum):
    OK = 0
    FAIL = 1
    TIMEOUT = 2

class HalBase:
    FRAME_SIZE = 64

    # 同一进程内所有 HAL 实例共享物理总线资源。
    _hw_lock = threading.RLock()
    _spi_lock = threading.Lock()
    _shared_spi_buses = {}

    # 类级共享：避免 HalPowerControl/HalMeterControl 重复 request 同一 ready 引脚
    _ready_lock = threading.Lock()

    _shared_io_bus = None
    _shared_ready_line = 14
    _shared_ready_requested = False

    def __init__(self, spi_dev="/dev/spidev0.0", cs_port=None, cs_line=None):
        # 1) 同一实例重复初始化，直接跳过
        if getattr(self, "_initialized", False):
            logger.info(
                "HalBase __init__ skipped (same instance): cls=%s id=%s pid=%s",
                self.__class__.__name__, hex(id(self)), os.getpid()
            )
            return

        logger.info(
            "HalBase __init__ enter: cls=%s id=%s pid=%s spi_dev=%s cs_port=%r cs_line=%r",
            self.__class__.__name__, hex(id(self)), os.getpid(), spi_dev, cs_port, cs_line
        )

        self._debug = True

        # 2) ready GPIO 只申请一次
        with HalBase._ready_lock:
            if HalBase._shared_io_bus is None:
                HalBase._shared_io_bus = GpioDriver("1")
                logger.info("HalBase shared io_bus created: chip=1")

            if not HalBase._shared_ready_requested:
                HalBase._shared_io_bus.request_input(HalBase._shared_ready_line, consumer="hal_ready")
                HalBase._shared_ready_requested = True
                logger.info("HalBase ready line requested FIRST time: chip=0 line=%s", HalBase._shared_ready_line)
            else:
                logger.info("HalBase ready line already requested, reuse it: chip=0 line=%s", HalBase._shared_ready_line)

        self.io_bus = HalBase._shared_io_bus
        self.ready_line_num = HalBase._shared_ready_line

        spi_key = (spi_dev, str(cs_port) if cs_port is not None else None, cs_line)
        with HalBase._spi_lock:
            if spi_key not in HalBase._shared_spi_buses:
                if cs_port and cs_line is not None:
                    bus = SpiDriver(spi_dev, cs_port=cs_port, cs_line_num=cs_line)
                else:
                    bus = SpiDriver(spi_dev)
                HalBase._shared_spi_buses[spi_key] = bus
            self.spi_bus = HalBase._shared_spi_buses[spi_key]

        self._initialized = True
        logger.info("HalBase __init__ done: cls=%s id=%s", self.__class__.__name__, hex(id(self)))

    def _log(self, msg):
        if self._debug:
            logger.debug(msg)

    def set_debug(self, enabled: bool):
        self._debug = bool(enabled)

    def _pad64(self, data):
        return self._pad_frame(data, self.FRAME_SIZE)

    @staticmethod
    def _pad_frame(data, frame_size):
        if frame_size <= 0 or frame_size > 0xFFFF:
            raise ValueError(f"invalid SPI frame size: {frame_size}")
        buf = list(data)
        if len(buf) > frame_size:
            raise ValueError(
                f"SPI frame payload too large: {len(buf)} > {frame_size}")
        return buf + [0x00] * (frame_size - len(buf))

    def _resp_f32(self, resp, default=0.0):
        # 约定: resp[2] 为状态码, resp[3:7] 为 float32 数据
        if not isinstance(resp, (list, tuple)) or len(resp) < 7:
            self._log(f"_resp_f32 invalid resp format: {resp}")
            return default
        try:
            return struct.unpack("<f", bytes(resp[3:7]))[0]
        except Exception as e:
            self._log(f"_resp_f32 unpack error: {e}")
            return default

    def _wait_ready(self, expected, timeout):
        if not self.spi_bus.is_real:
            return True
        start_time = time.monotonic()
        while self.io_bus.get_value(self.ready_line_num) != expected:
            if time.monotonic() - start_time >= timeout:
                return False
            time.sleep(0.001)
        return True

    def _spi_exchange_frame(self, header, payload=None, *, frame_size=FRAME_SIZE,
                            read_len=None, timeout=10.0,
                            require_busy_edge=False):
        """完成一次命令发送和应答读取，返回 ``(StatusCode, rx_data)``。

        ``require_busy_edge`` 用于 Bootloader：发送后必须观察到 M_INT
        先低后高，避免把命令发送前残留的高电平误认为新应答。
        """
        if payload is None:
            payload = []
        if read_len is None:
            read_len = frame_size
        if read_len < 0 or read_len > frame_size:
            raise ValueError(f"invalid SPI read length: {read_len}")

        tx = self._pad_frame(list(header) + list(payload), frame_size)
        with self._hw_lock:
            if not self._wait_ready(1, timeout):
                return StatusCode.TIMEOUT, []

            self.spi_bus.transfer(tx)
            if read_len == 0:
                return StatusCode.OK, []

            if require_busy_edge and not self._wait_ready(0, timeout):
                return StatusCode.TIMEOUT, []
            if not self._wait_ready(1, timeout):
                return StatusCode.TIMEOUT, []

            rx = list(self.spi_bus.transfer([0x00] * frame_size))
            status = StatusCode.FAIL
            if len(rx) > 2:
                try:
                    status = StatusCode(rx[2])
                except ValueError:
                    pass
            return status, rx[:read_len]

    def _spi_exchange64(self, header, payload=None, read_len=FRAME_SIZE):
        """兼容现有设备命令的固定 64 字节交换。"""
        return self._spi_exchange_frame(
            header,
            payload,
            frame_size=self.FRAME_SIZE,
            read_len=read_len,
        )
