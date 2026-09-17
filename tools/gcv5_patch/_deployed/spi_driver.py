import sys
sys.path.insert(0, sys.path[0]+"/../")
import os
import time
import threading
from hal.gpio_driver import GpioDriver
from core.logger import logger

try:
    from periphery import SPI
    HAS_HW_LIBS = True
except ImportError:
    HAS_HW_LIBS = False
    logger.warning("periphery library not found. SPI operations will be mocked.")

class SpiDriver:
    """
    底层 SPI 驱动类 (Singleton)
    负责统一管理所有的 SPI 设备文件，防止重复打开导致的冲突。
    """
    _instances = {} # 存储不同设备文件的实例，如 {"/dev/spidev0.0": obj}
    _lock = threading.Lock()

    # def __new__(cls, device, mode=0, speed=1000000,cs_port=None,cs_line_num=None):
    #     with cls._lock:
    #         if device not in cls._instances:
    #             instance = super(SpiDriver, cls).__new__(cls)
    #             instance._initialized = False
    #             cls._instances[device] = instance
    #         return cls._instances[device]

    def __init__(self, device, mode=0, speed=1000000,cs_port=None,cs_line_num=None):
        if getattr(self, "_initialized", False):
            # 如果已经初始化过，且参数不同，则更新配置
            self.update_config(mode, speed)
            return
        
        self.device = device
        uid_is_root = (os.getuid() == 0)
        has_hw_libs = HAS_HW_LIBS
        self.is_real = uid_is_root and has_hw_libs
        if not self.is_real:
            logger.info(
                "SpiDriver REAL mode disabled: uid_is_root=%s(uid=%s), HAS_HW_LIBS=%s, device=%r",
                uid_is_root, os.getuid(), has_hw_libs, device
            )
        self.spi = None
        self.cs_port = None
        self.cs_line_num = None
        
        if self.is_real:
            if not os.path.exists(device):
                logger.warning(f"SpiDriver: {device} not found in /dev. Falling back to MOCK mode.")
                self.is_real = False
            else:
                try:
                    self.spi = SPI(device, mode, speed)
                    self.cs_port = cs_port
                    self.cs_line_num = cs_line_num
                    
                    if self.cs_port is not None:
                        self.cs_bus = GpioDriver(self.cs_port)
                        if self.cs_line_num is not None:
                            self.cs_bus.request_output(self.cs_line_num, consumer="cs0")
                            self.cs_bus.set_value(self.cs_line_num, 1)
                        else:
                            logger.warning(f"SpiDriver: cs_port provided but cs_line_num is None for {device}")
                    else:
                        logger.warning(f"SpiDriver: No CS GPIO control for {device}")
                    
                    time.sleep(0.1)
                    logger.info(f"SpiDriver: Real device {device} initialized.{cs_port}")
                except Exception as e:
                    if "busy" in str(e).lower() or (isinstance(e, OSError) and e.errno == 16):
                        logger.error(f"SpiDriver: 设备 {device} 被占用 (Device or resource busy)")
                    else:
                        logger.error(f"SpiDriver: Failed to open {device}: {e}")
                    self.is_real = False
        else:
            logger.info(f"SpiDriver: Device {device} initialized in MOCK mode.")
            
        self._initialized = True

    def update_config(self, mode=0, speed=1000000):
        """
        动态修改现有 SPI 实例的属性，避免重新打开设备文件,立即生效。
        """
        if not self.is_real or not self.spi:
            return

        try:
            changed = False
            if mode is not None and self.spi.mode != mode:
                self.spi.mode = mode
                changed = True
            if speed is not None and self.spi.max_speed != speed:
                self.spi.max_speed = speed
                changed = True
            
            if changed:
                logger.info(f"SpiDriver: Config updated for {self.device}: mode={mode}, speed={speed}")
        except Exception as e:
            logger.error(f"SpiDriver: Failed to update config for {self.device}: {e}")

    def transfer(self, data_out):
        """统一的传输接口"""
        if not self.is_real or not self.spi:
            # Mock 模式：原样返回或返回全 0
            return [0] * len(data_out)
            
        try:
            # 只有在提供了 CS GPIO 时才手动拉低
            if hasattr(self, "cs_bus") and self.cs_bus and self.cs_line_num is not None:
                self.cs_bus.set_value(self.cs_line_num, 0)
            
            ret = self.spi.transfer(data_out)
            
            if hasattr(self, "cs_bus") and self.cs_bus and self.cs_line_num is not None:
                self.cs_bus.set_value(self.cs_line_num, 1)
                
            return ret
        except Exception as e:
            logger.error(f"SpiDriver: Transfer error on {self.device}: {e}")
            return [0] * len(data_out)

    def close(self):
        if self.spi:
            self.spi.close()
if __name__ == "__main__":
    spi = SpiDriver("/dev/spidev0.0",cs_port="7",cs_line_num=4)
    spi.transfer([0xAA, 0xBB, 0xCC, 0xDD])
    spi.transfer([0xAA, 0xBB, 0xCC, 0xDD])
    spi.close()