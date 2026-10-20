#!/usr/bin/env python3
"""Host tests for pacing, the audio queue, input and menu transitions."""
import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--core", type=Path, help="also test APU and emulator lifecycle")
    args = parser.parse_args()
    compiler = shlex.split(os.environ.get("CC", "cc"))
    flags = ["-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                  "-fno-omit-frame-pointer"]
    cases = {
        "runtime": ["tests/runtime_test.c", "src/timing.c", "src/audio_queue.c"],
        "input_menu": ["tests/input_menu_test.c", "src/input.c"],
    }
    if args.core:
        core = args.core.resolve()
        flags += ["-I", str(core), "-I", str(core / "examples/sdl2/minigb_apu"),
                  "-DMINIGB_APU_AUDIO_FORMAT_S16SYS=1", "-DENABLE_SOUND=1",
                  "-DENABLE_LCD=1", "-ffunction-sections", "-fdata-sections",
                  "-Wl,--gc-sections"]
        cases["audio_lifecycle"] = ["tests/audio_lifecycle_test.c", "src/timing.c",
                                    "src/audio_queue.c", "src/cart_ram.c",
                                    str(core / "examples/sdl2/minigb_apu/minigb_apu.c")]
    with tempfile.TemporaryDirectory(prefix="pegb-runtime-") as temp:
        # Source-local includes must not pick up the user's generated game ROMs.
        isolated = Path(temp) / "source"
        shutil.copytree(ROOT / "src", isolated / "src",
                        ignore=shutil.ignore_patterns("rom_data.h"))
        shutil.copytree(ROOT / "tests", isolated / "tests",
                        ignore=shutil.ignore_patterns("__pycache__"))
        flags += ["-I", str(isolated / "tests/stubs"), "-I", str(isolated / "src")]
        for name, sources in cases.items():
            binary = Path(temp) / name
            command = compiler + flags + [str(isolated / p) for p in sources] + ["-o", str(binary)]
            print("Compile:", shlex.join(command), flush=True)
            subprocess.run(command, check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
