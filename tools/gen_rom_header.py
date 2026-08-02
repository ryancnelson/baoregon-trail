#!/usr/bin/env python3
import sys

def main():
    if len(sys.argv) < 3:
        print("usage: gen_rom_header.py <rom_bin> <out_h>")
        sys.exit(1)
    
    rom_path = sys.argv[1]
    out_path = sys.argv[2]
    
    with open(rom_path, "rb") as f:
        data = f.read()
    
    with open(out_path, "w") as out:
        out.write("#ifndef APPLE2E_SYSTEM_ROM_H\n#define APPLE2E_SYSTEM_ROM_H\n\n#include <stdint.h>\n\n")
        out.write(f"/* Real Apple IIe System ROM (16384 bytes, $C000-$FFFF) */\n")
        out.write(f"static const uint8_t g_apple2e_system_rom[{len(data)}] = {{\n")
        for i, b in enumerate(data):
            out.write(f"0x{b:02X},")
            if (i + 1) % 16 == 0:
                out.write("\n")
        out.write("};\n\n#endif\n")

if __name__ == "__main__":
    main()
