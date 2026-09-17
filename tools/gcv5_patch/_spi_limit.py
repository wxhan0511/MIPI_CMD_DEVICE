import ctypes, fcntl, os

class spi_ioc_transfer(ctypes.Structure):
    _fields_ = [
        ("tx_buf", ctypes.c_uint64), ("rx_buf", ctypes.c_uint64),
        ("len", ctypes.c_uint32), ("speed_hz", ctypes.c_uint32),
        ("delay_usecs", ctypes.c_uint16), ("bits_per_word", ctypes.c_uint8),
        ("cs_change", ctypes.c_uint8), ("tx_nbits", ctypes.c_uint8),
        ("rx_nbits", ctypes.c_uint8), ("word_delay_usecs", ctypes.c_uint8),
        ("pad", ctypes.c_uint8),
    ]

SPI_IOC_MESSAGE_1 = (1 << 30) | (ord('k') << 8) | (32 << 16)

def try_len(fd, n, full_duplex):
    tx = ctypes.create_string_buffer(n)
    rx = ctypes.create_string_buffer(n) if full_duplex else None
    tr = spi_ioc_transfer()
    tr.tx_buf = ctypes.addressof(tx)
    tr.rx_buf = ctypes.addressof(rx) if rx else 0
    tr.len = n
    tr.speed_hz = 1000000
    tr.bits_per_word = 8
    try:
        fcntl.ioctl(fd, SPI_IOC_MESSAGE_1, tr)
        return "OK"
    except OSError as e:
        return "ERR errno=%d" % e.errno

fd = os.open("/dev/spidev0.0", os.O_RDWR)
print("full-duplex (tx+rx):")
for n in [4096, 4097, 5120]:
    print("  len=%-6d -> %s" % (n, try_len(fd, n, True)))
os.close(fd)
