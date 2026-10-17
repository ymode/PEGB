#!/usr/bin/env python3
"""Compile and test the actual gb_rom_read callback without the Pico SDK."""

import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=ROOT / "src/main.c",
                        help="main.c to test (default: repository source)")
    parser.add_argument("--sanitize", action="store_true",
                        help="enable AddressSanitizer and UndefinedBehaviorSanitizer")
    parser.add_argument("--valid-only", action="store_true",
                        help="only read valid bytes, for comparison with old source")
    args = parser.parse_args()

    source = args.source.read_text(encoding="utf-8")
    # The callback is top-level, with its closing brace at column zero.
    # Fail explicitly if a source refactor makes this extraction ambiguous.
    callbacks = list(re.finditer(
        r"^uint8_t gb_rom_read\(struct gb_s \*gb,\s*"
        r"const uint_fast32_t addr\)\s*\{.*?^\}",
        source, re.MULTILINE | re.DOTALL))
    if len(callbacks) != 1:
        parser.error("expected exactly one gb_rom_read definition in source")

    flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic", "-O1", "-g"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                  "-fno-omit-frame-pointer"]
    compiler = shlex.split(os.environ.get("CC", "cc"))
    with tempfile.TemporaryDirectory(prefix="pegb-rom-read-") as temp:
        build = Path(temp)
        (build / "gb_rom_read.inc").write_text(
            callbacks[0].group(0) + "\n", encoding="utf-8")
        binary = build / "rom_read_test"
        command = compiler + flags + ["-I", str(build),
                                     str(ROOT / "tests/rom_read_test.c"),
                                     "-o", str(binary)]
        print("Compile:", shlex.join(command), flush=True)
        subprocess.run(command, check=True)
        command = [str(binary)]
        if args.valid_only:
            command.append("--valid-only")
        subprocess.run(command, check=True)


if __name__ == "__main__":
    main()
