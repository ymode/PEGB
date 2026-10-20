# PEGB 📌🐝

A Game Boy emulator running on the [Pimoroni Explorer](https://shop.pimoroni.com/products/explorer) (RP2350B).

Built on top of [Peanut-GB](https://github.com/deltabeard/Peanut-GB) by deltabeard — a single-header Game Boy (DMG) emulator designed for embedded systems.

<img src="https://github.com/user-attachments/assets/d951a080-ec59-4070-a4e4-1db5a977e8b9" width="300">

## Hardware

- **Board:** Pimoroni Explorer (RP2350B, 48 GPIO, 520 KB SRAM, 16 MB flash)
- **Display:** 320x240 ST7789 IPS LCD via 8-bit parallel bus (PIO-driven with DMA)
- **Audio:** PWM output on the onboard piezo speaker
- **Input:** 6 onboard buttons

## Button Mapping

| Explorer Button | Game Boy |
|----------------|----------|
| A | D-pad Up |
| B | D-pad Down |
| C | D-pad Left |
| Z | D-pad Right |
| X | A |
| Y | B |
| C + Z | Start |
| A + B | Select |

Start and Select use "impossible" d-pad combos (opposite directions) that can't occur during normal gameplay.

## Building

### Prerequisites

- [ARM GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) (arm-none-eabi-gcc with newlib)
- CMake 3.13+
- A `.gb` ROM file (you must supply your own)

### Setup

```bash
./setup.sh
```

This clones the [Pico SDK](https://github.com/raspberrypi/pico-sdk) and a tested
revision of [Peanut-GB](https://github.com/deltabeard/Peanut-GB). The build uses
minigb_apu directly from Peanut-GB's `examples/sdl2/minigb_apu` directory,
including its upstream license. No separate audio-library checkout is needed.
If an existing Peanut-GB checkout has a different revision, setup asks you to
move it aside rather than overwrite it.

### Convert a ROM

```bash
python3 tools/convert_rom.py path/to/game.gb
```

This generates `src/rom_data.h` which embeds the ROM into flash.

### Build

```bash
mkdir -p build && cd build
PICO_SDK_PATH=../lib/pico-sdk cmake ..
make -j$(nproc)
```

### Flash

1. Hold **BOOTSEL** on the Explorer while plugging in USB
2. Copy `build/pe_gbc.uf2` to the mounted `RP2350` drive
3. The board reboots and starts the emulator

## Host regression tests

The `Host regression tests` GitHub Actions workflow runs on every push and pull
request using Ubuntu, Python 3, and GCC. It executes the converter and ROM-read
suites below, plus ROM-read and cartridge-RAM checks with ASan/UBSan. CI disables leak detection to avoid restricted
runner `/proc` access; address and undefined-behavior failures still fail the job.
This workflow does not install the Pico SDK, build firmware, or test hardware.

Run the host tests with Python 3 and a host C11 compiler (GCC or Clang):

```bash
python3 tests/test_convert_rom.py
python3 tests/test_rom_read.py
python3 tests/test_rom_read.py --sanitize
python3 tests/test_cart_ram.py --sanitize
```

Set `CC` to select the compiler. The sanitizer run requires AddressSanitizer and
UndefinedBehaviorSanitizer support. No Pico SDK, emulator dependencies, or game
ROMs are needed; temporary build files are removed automatically.

If a restricted runner prevents LeakSanitizer from inspecting `/proc`, run
`ASAN_OPTIONS=detect_leaks=0 python3 tests/test_rom_read.py --sanitize`.
Address and undefined-behavior checks remain enabled.

The runner extracts `gb_rom_read()` verbatim from `src/main.c` and compiles it
against simulated ROM data. It checks every valid byte (including the last byte),
bank boundaries, ROM switching, exact and beyond-end addresses, maximum address
values, and zero-length ROMs with null and non-null data. These are host callback
tests, not a firmware build or validation on RP2350 hardware.

The converter tests create synthetic ROM headers and unusual filenames in a
temporary directory, compile the generated headers as strict C11 with trigraphs
enabled, and execute a host probe to compare every title and ROM byte. They cover
quotes, backslashes, attempted C injection, filtered header controls, newline and
control characters in filenames, directory discovery, de-duplication, and helper
control escapes followed by hex digits. Filename cases are skipped explicitly if
the filesystem cannot represent them. No repository ROM header is overwritten.
Use `python3 tests/test_convert_rom.py --source path/to/convert_rom.py` to test
another converter implementation.

To compare with an older implementation, pass `--source path/to/main.c`.
`--valid-only` checks unchanged in-range behavior without making invalid reads.

Cartridge-RAM tests compile the same storage and callbacks as the firmware. They
check every byte for capacities from zero through 128 KiB, independent banks,
invalid addresses, rejected oversized allocations, and clearing when switching
games. After setup, also exercise the actual MBC5 mapper with 32/64/128 KiB RAM:

```bash
python3 tests/test_cart_ram.py --sanitize --core lib/peanut-gb
```

The separate `Firmware build` workflow starts with a fresh checkout, runs setup
twice to check repeatability, runs the mapper integration tests, and compiles an
Explorer UF2 using the ARM toolchain and a generated synthetic ROM. It does not
run the firmware on physical hardware or use commercial game ROMs. To generate
the same build fixture locally, run `python3 tests/make_test_rom.py roms/ci-smoke.gb`
and convert it with the normal ROM converter.

## Technical Details

- RP2350B overclocked to 250 MHz
- Display driven via PIO state machine with DMA transfers for the framebuffer
- GB 160x144 framebuffer scaled to 320x240 (2x horizontal, nearest-neighbor vertical)
- Audio generated by [minigb_apu](https://github.com/deltabeard/Peanut-GB/tree/master/examples/sdl2/minigb_apu) at 32768 Hz, output via timer-driven PWM
- Cartridge RAM uses a fixed 128 KiB allocation, with access limited to the selected cartridge's declared size. Out-of-range reads return `0xFF`; writes are ignored. RAM is cleared on game changes and is not persisted across power cycles.
- Emulation is DMG (original Game Boy) only — Game Boy Color is not supported

## ROM Notice

This project does not include any ROM files. You must provide your own legally obtained `.gb` ROM files. ROM files are excluded from version control via `.gitignore`.

## Credits

- [Peanut-GB](https://github.com/deltabeard/Peanut-GB) by deltabeard — the Game Boy emulator core
- [minigb_apu](https://github.com/deltabeard/Peanut-GB/tree/master/examples/sdl2/minigb_apu) — Game Boy APU emulation, based on [MiniGBS](https://github.com/baines/MiniGBS) by Alex Baines
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [Pimoroni](https://pimoroni.com/) for the Explorer board

## License

MIT License. See [LICENSE](LICENSE).

Peanut-GB and minigb_apu are used under their respective licenses — see the [Peanut-GB repository](https://github.com/deltabeard/Peanut-GB) for details.
