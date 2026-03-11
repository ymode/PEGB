#!/usr/bin/env python3
"""Convert .gb ROM files to a C header for embedding in flash.

Usage:
    convert_rom.py <rom.gb>              # Single ROM
    convert_rom.py <rom1.gb> <rom2.gb>   # Multiple ROMs
    convert_rom.py roms/                  # All .gb files in a directory
"""

import sys
import os
import glob

def get_rom_title(data):
    """Extract game title from ROM header (0x134-0x143)."""
    raw = data[0x134:0x144]
    # Null-terminate and strip non-printable chars
    title = ""
    for b in raw:
        if b == 0:
            break
        if 32 <= b < 127:
            title += chr(b)
    return title.strip() or "UNKNOWN"

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <rom.gb|directory> [rom2.gb ...]")
        sys.exit(1)

    rom_files = []
    for arg in sys.argv[1:]:
        if os.path.isdir(arg):
            rom_files.extend(sorted(glob.glob(os.path.join(arg, "*.gb"))))
            rom_files.extend(sorted(glob.glob(os.path.join(arg, "*.GB"))))
        else:
            rom_files.append(arg)

    if not rom_files:
        print("No .gb files found")
        sys.exit(1)

    out_path = os.path.join(os.path.dirname(__file__), "..", "src", "rom_data.h")

    roms = []
    for path in rom_files:
        with open(path, "rb") as f:
            data = f.read()
        title = get_rom_title(data)
        var_name = f"rom_{len(roms)}"
        roms.append((title, var_name, data, os.path.basename(path)))

    with open(out_path, "w") as f:
        f.write("#ifndef ROM_DATA_H\n")
        f.write("#define ROM_DATA_H\n\n")
        f.write("#include <stdint.h>\n")
        f.write("#include <stddef.h>\n\n")

        # Write each ROM as a separate array
        for title, var_name, data, filename in roms:
            f.write(f"// {filename} - \"{title}\" ({len(data)} bytes)\n")
            f.write(f"static const uint8_t {var_name}[] = {{\n")
            for i in range(0, len(data), 16):
                chunk = data[i:i+16]
                hex_str = ", ".join(f"0x{b:02X}" for b in chunk)
                f.write(f"    {hex_str},\n")
            f.write("};\n\n")

        # Write ROM directory
        f.write("typedef struct {\n")
        f.write("    const char *title;\n")
        f.write("    const uint8_t *data;\n")
        f.write("    size_t size;\n")
        f.write("} rom_entry_t;\n\n")

        f.write(f"#define ROM_COUNT {len(roms)}\n\n")
        f.write("static const rom_entry_t rom_list[ROM_COUNT] = {\n")
        for title, var_name, data, filename in roms:
            f.write(f"    {{ \"{title}\", {var_name}, {len(data)} }},\n")
        f.write("};\n\n")

        f.write("#endif // ROM_DATA_H\n")

    print(f"Wrote {out_path} with {len(roms)} ROM(s):")
    for title, _, data, filename in roms:
        print(f"  {filename}: \"{title}\" ({len(data) / 1024:.1f} KB)")

if __name__ == "__main__":
    main()
