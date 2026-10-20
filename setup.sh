#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== PE-GBC Setup ==="

# Clone Pico SDK if not already present
if [ -z "$PICO_SDK_PATH" ]; then
    if [ ! -d "lib/pico-sdk" ]; then
        echo "Cloning Pico SDK..."
        git clone --depth 1 -b 2.1.1 https://github.com/raspberrypi/pico-sdk.git lib/pico-sdk
        cd lib/pico-sdk && git submodule update --init && cd "$SCRIPT_DIR"
    fi
    export PICO_SDK_PATH="$SCRIPT_DIR/lib/pico-sdk"
    echo "PICO_SDK_PATH=$PICO_SDK_PATH"
fi

# Copy pico_sdk_import.cmake
if [ ! -f "pico_sdk_import.cmake" ]; then
    cp "$PICO_SDK_PATH/external/pico_sdk_import.cmake" .
fi

# Keep the core and its bundled minigb_apu at the revision tested by CI.
PEANUT_GB_REVISION=d0bcca771c83a2638c93a9ae61f3d51f226dc905
if [ ! -d "lib/peanut-gb" ]; then
    echo "Cloning Peanut-GB..."
    git init -q lib/peanut-gb
    git -C lib/peanut-gb remote add origin https://github.com/deltabeard/Peanut-GB.git
    git -C lib/peanut-gb fetch --depth 1 origin "$PEANUT_GB_REVISION"
    git -C lib/peanut-gb checkout --detach FETCH_HEAD
fi
if [ "$(git -C lib/peanut-gb rev-parse HEAD)" != "$PEANUT_GB_REVISION" ]; then
    echo "Expected Peanut-GB revision $PEANUT_GB_REVISION." >&2
    echo "Move your existing lib/peanut-gb checkout aside and rerun setup.sh." >&2
    exit 1
fi

echo ""
echo "=== Setup complete ==="
echo ""
echo "Next steps:"
echo "  1. Convert a ROM:  python3 tools/convert_rom.py path/to/game.gb"
echo "  2. Build:          PICO_SDK_PATH=\"$PICO_SDK_PATH\" cmake -S . -B build && cmake --build build -j"
echo "  3. Flash:          Copy build/pe_gbc.uf2 to the Explorer (hold BOOTSEL)"
