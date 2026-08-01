#!/usr/bin/env python3
"""
tools/convert_image_to_hires.py -- quick, low-fidelity PNG -> Apple II
Hi-Res bitmap converter. Thresholds to 1-bit black/white (no real Hi-Res
color-artifact dithering yet -- see README note below) and packs into the
real 8KB Hi-Res buffer layout via the same hires_line_offsets[] table
used by src/video_apple2.c (extracted programmatically, not retyped).

Usage:
    python3 tools/convert_image_to_hires.py tools/assets/oregon_trail_title.png tools/oregon_trail_hires.bin
"""
import re
import sys
from pathlib import Path

from PIL import Image

HIRES_WIDTH = 280
HIRES_HEIGHT = 192
HIRES_BUFFER_SIZE = 8192


def load_offsets():
    src = Path("src/video_apple2.c").read_text()
    m = re.search(r"hires_line_offsets\[HIRES_ROWS\] = \{(.*?)\};", src, re.S)
    return [int(x, 16) for x in re.findall(r"0x[0-9a-fA-F]+", m.group(1))]


def main():
    if len(sys.argv) < 3:
        print(f"usage: {sys.argv[0]} <input.png> <output.bin>", file=sys.stderr)
        sys.exit(1)
    in_path, out_path = sys.argv[1], sys.argv[2]

    img = Image.open(in_path).convert("L").resize((HIRES_WIDTH, HIRES_HEIGHT))
    offsets = load_offsets()
    assert len(offsets) == HIRES_HEIGHT

    buf = bytearray(HIRES_BUFFER_SIZE)
    pixels = img.load()

    for row in range(HIRES_HEIGHT):
        row_base = offsets[row]
        for byte_idx in range(40):
            byte_val = 0
            for bit in range(7):
                x = byte_idx * 7 + bit
                if x >= HIRES_WIDTH:
                    continue
                lum = pixels[x, row]
                if lum > 96:  # quick threshold, no real dithering (v1)
                    byte_val |= (1 << bit)
            buf[row_base + byte_idx] = byte_val

    Path(out_path).write_bytes(bytes(buf))
    lit = sum(bin(b).count("1") for b in buf)
    print(f"Wrote {out_path}: {len(buf)} bytes, {lit} lit sub-pixels "
          f"({100*lit/(HIRES_WIDTH*HIRES_HEIGHT):.1f}% of frame)")


if __name__ == "__main__":
    main()
