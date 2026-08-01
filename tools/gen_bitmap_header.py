#!/usr/bin/env python3
"""
tools/gen_bitmap_header.py -- generates src/oregon_trail_bitmap_data.h
from tools/oregon_trail_hires.bin, for embedding into the QEMU-target
build (src/main_qemu.c) as a C byte array. Re-run after any change to
the source bitmap.

Usage: python3 tools/gen_bitmap_header.py
"""
from pathlib import Path

SRC = Path("tools/oregon_trail_hires.bin")
OUT = Path("src/oregon_trail_bitmap_data.h")


def main():
    data = SRC.read_bytes()
    lines = [
        "/* Auto-generated from tools/oregon_trail_hires.bin by "
        "tools/gen_bitmap_header.py -- do not hand-edit. */",
        "#ifndef OREGON_TRAIL_BITMAP_DATA_H",
        "#define OREGON_TRAIL_BITMAP_DATA_H",
        "#include <stdint.h>",
        f"static const uint8_t oregon_trail_bitmap[{len(data)}] = {{",
    ]
    for i in range(0, len(data), 16):
        lines.append("    " + ",".join(str(b) for b in data[i:i + 16]) + ",")
    lines += ["};", "#endif"]
    OUT.write_text("\n".join(lines) + "\n")
    print(f"Wrote {OUT} ({len(data)} bytes embedded)")


if __name__ == "__main__":
    main()
