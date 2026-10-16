#!/usr/bin/env python3
"""Finalize the MIPI CMD application image and build a factory image.

The bootloader validates a fixed 512 KiB application slot at 0x08010000.  The
last 16 bytes contain firmware metadata; the CRC field at metadata + 8 covers
all bytes before that metadata block.  A plain objcopy image leaves this field
at zero, so it must never be used as the final programming image.
"""

from __future__ import annotations

import argparse
import struct
import sys
import zlib
from pathlib import Path


APP_ADDRESS = 0x08010000
APP_SLOT_SIZE = 0x80000
APP_META_OFFSET = 0x7FFF0
APP_CRC_OFFSET = APP_META_OFFSET + 8
BOOT_SLOT_SIZE = 0x10000


def fail(message: str) -> None:
    raise SystemExit(f"firmware finalize error: {message}")


def read_file(path: Path, label: str) -> bytes:
    if not path.is_file():
        fail(f"{label} not found: {path}")
    return path.read_bytes()


def finalize_app(raw: bytes) -> tuple[bytes, int]:
    if len(raw) > APP_SLOT_SIZE:
        fail(f"application is {len(raw)} bytes, slot is only {APP_SLOT_SIZE}")

    image = bytearray(b"\xFF" * APP_SLOT_SIZE)
    image[: len(raw)] = raw

    msp, reset = struct.unpack_from("<II", image, 0)
    if not (0x20000000 < msp <= 0x20020000):
        fail(f"invalid application MSP 0x{msp:08X}")
    if not (APP_ADDRESS <= (reset & ~1) < APP_ADDRESS + APP_SLOT_SIZE):
        fail(f"invalid application reset vector 0x{reset:08X}")
    if (reset & 1) == 0:
        fail(f"application reset vector is not Thumb code: 0x{reset:08X}")

    crc = zlib.crc32(image[:APP_META_OFFSET]) & 0xFFFFFFFF
    struct.pack_into("<I", image, APP_CRC_OFFSET, crc)
    return bytes(image), crc


def verify_app(image: bytes, expected_crc: int) -> None:
    if len(image) != APP_SLOT_SIZE:
        fail(f"final application size is {len(image)}, expected {APP_SLOT_SIZE}")
    stored_crc = struct.unpack_from("<I", image, APP_CRC_OFFSET)[0]
    computed_crc = zlib.crc32(image[:APP_META_OFFSET]) & 0xFFFFFFFF
    if stored_crc != expected_crc or computed_crc != expected_crc:
        fail(
            f"CRC verification failed: stored=0x{stored_crc:08X}, "
            f"computed=0x{computed_crc:08X}, expected=0x{expected_crc:08X}"
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--app-raw", required=True, type=Path)
    parser.add_argument("--boot-bin", required=True, type=Path)
    parser.add_argument("--app-out", required=True, type=Path)
    parser.add_argument("--factory-out", required=True, type=Path)
    args = parser.parse_args()

    raw_app = read_file(args.app_raw, "raw application")
    boot = read_file(args.boot_bin, "bootloader")
    if len(boot) > BOOT_SLOT_SIZE:
        fail(f"bootloader is {len(boot)} bytes, slot is only {BOOT_SLOT_SIZE}")

    app, crc = finalize_app(raw_app)
    verify_app(app, crc)

    args.app_out.parent.mkdir(parents=True, exist_ok=True)
    args.factory_out.parent.mkdir(parents=True, exist_ok=True)
    args.app_out.write_bytes(app)

    factory = bytearray(b"\xFF" * BOOT_SLOT_SIZE)
    factory[: len(boot)] = boot
    factory.extend(app)
    args.factory_out.write_bytes(factory)

    fw = ".".join(str(value) for value in app[APP_META_OFFSET:APP_META_OFFSET + 4])
    hw = ".".join(str(value) for value in app[APP_META_OFFSET + 4:APP_META_OFFSET + 8])
    print(
        f"Firmware finalized: app={len(app)} bytes, factory={len(factory)} bytes, "
        f"fw={fw}, hw={hw}, CRC32=0x{crc:08X}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
