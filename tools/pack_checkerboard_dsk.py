#!/usr/bin/env python3
"""
tools/pack_checkerboard_dsk.py -- packages the ca65-assembled
checkerboard.bin (see checkerboard.s) into a 140 KiB flat DOS-order .dsk
image, for use with hires_demo_runner.

Build + pack:
    ca65 tools/checkerboard.s -o tools/checkerboard.o
    ld65 -C tools/checkerboard.cfg -o tools/checkerboard.bin tools/checkerboard.o
    python3 tools/pack_checkerboard_dsk.py disks/checkerboard_demo.dsk
"""
import sys
from pathlib import Path

DOS33_DISK_SIZE = 143360


def main():
    bin_path = Path("tools/checkerboard.bin")
    out_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("disks/checkerboard_demo.dsk")

    program = bin_path.read_bytes()
    if len(program) > DOS33_DISK_SIZE:
        raise SystemExit(f"program too large: {len(program)} bytes")

    disk = bytearray(DOS33_DISK_SIZE)
    disk[:len(program)] = program

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(disk)
    print(f"Packed {len(program)}-byte program into {out_path} ({len(disk)} bytes)")


if __name__ == "__main__":
    main()
