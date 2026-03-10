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

# Clone Peanut-GB
if [ ! -d "lib/peanut-gb" ]; then
    echo "Cloning Peanut-GB..."
    git clone --depth 1 https://github.com/deltabeard/Peanut-GB.git lib/peanut-gb
fi

echo ""
echo "=== Setup complete ==="
echo ""
echo "Next steps:"
echo "  1. Convert a ROM:  python3 tools/convert_rom.py path/to/game.gb"
echo "  2. Build:          mkdir -p build && cd build && cmake .. && make -j"
echo "  3. Flash:          Copy build/pe_gbc.uf2 to the Explorer (hold BOOTSEL)"
