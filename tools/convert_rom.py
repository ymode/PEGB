#!/usr/bin/env python3
"""Convert a .gb ROM file to a C header for embedding in flash."""

import sys
import os

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <rom.gb>")
        sys.exit(1)

    rom_path = sys.argv[1]
    out_path = os.path.join(os.path.dirname(__file__), "..", "src", "rom_data.h")

    with open(rom_path, "rb") as f:
        rom = f.read()

    with open(out_path, "w") as f:
        f.write("#ifndef ROM_DATA_H\n")
        f.write("#define ROM_DATA_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"// Auto-generated from {os.path.basename(rom_path)}\n")
        f.write(f"// Size: {len(rom)} bytes ({len(rom) / 1024:.1f} KB)\n\n")
        f.write("static const uint8_t rom_data[] = {\n")

        for i in range(0, len(rom), 16):
            chunk = rom[i:i+16]
            hex_str = ", ".join(f"0x{b:02X}" for b in chunk)
            f.write(f"    {hex_str},\n")

        f.write("};\n\n")
        f.write("#endif // ROM_DATA_H\n")

    print(f"Wrote {out_path} ({len(rom)} bytes)")

if __name__ == "__main__":
    main()
