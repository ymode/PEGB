#!/usr/bin/env python3
"""Test cartridge RAM storage; optionally exercise the actual Peanut-GB mapper."""

import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--core", type=Path,
                        help="Peanut-GB checkout directory for mapper integration tests")
    args = parser.parse_args()
    flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-O1", "-g"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                  "-fno-omit-frame-pointer"]
    if args.core:
        if not (args.core / "peanut_gb.h").is_file():
            parser.error("--core must contain peanut_gb.h; run ./setup.sh first")
        flags += ["-DTEST_PEANUT_GB=1", "-DENABLE_SOUND=0", "-DENABLE_LCD=0",
                  "-I", str(args.core.resolve())]
    compiler = shlex.split(os.environ.get("CC", "cc"))
    with tempfile.TemporaryDirectory(prefix="pegb-cart-ram-") as temp:
        binary = Path(temp) / "cart_ram_test"
        command = compiler + flags + ["-I", str(ROOT / "src"),
                                      str(ROOT / "src/cart_ram.c"),
                                      str(ROOT / "tests/cart_ram_test.c"),
                                      "-o", str(binary)]
        print("Compile:", shlex.join(command), flush=True)
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
