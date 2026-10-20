#!/usr/bin/env python3
"""Generate a synthetic DMG ROM for firmware build checks (no game assets)."""

import argparse
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = bytearray(32768)
    data[0x100:0x103] = bytes([0xC3, 0x00, 0x01])  # JP 0x0100
    data[0x134:0x13C] = b"CI-SMOKE"
    data[0x14D] = (-sum(data[0x134:0x14D]) - 25) & 0xFF
    # ROM-only, no RAM. Peanut-GB does not require the boot logo.
    args.output.write_bytes(data)


if __name__ == "__main__":
    main()
