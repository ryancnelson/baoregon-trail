#!/usr/bin/env python3
"""
tools/gen_nib_disk_header.py -- generates a C header embedding a
disk2_controller.h-compatible disk2_nibble_track_t[DISK2_MAX_TRACKS]
array from a directory of track00.nib .. track34.nib files (see
tools/dsk_to_nib.py), for embedding into the QEMU-target build
(src/main_qemu_disk2boot.c) as static const data -- the QEMU bare-metal
target has no filesystem, so disk images must be compiled in as byte
arrays, same pattern as tools/gen_bitmap_header.py.

Usage: python3 tools/gen_nib_disk_header.py <nib_track_dir> -o <out.h> --name <c_identifier>
"""
from __future__ import annotations
import argparse
from pathlib import Path

DISK2_MAX_TRACKS = 35
DISK2_MAX_TRACK_BYTES = 6656


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("nib_dir", type=Path)
    parser.add_argument("-o", "--output", type=Path, required=True)
    parser.add_argument("--name", default="embedded_nib_disk",
                         help="C identifier prefix for the generated array/length table")
    args = parser.parse_args(argv)

    tracks: list[bytes] = []
    for t in range(DISK2_MAX_TRACKS):
        path = args.nib_dir / f"track{t:02d}.nib"
        data = path.read_bytes()
        if len(data) > DISK2_MAX_TRACK_BYTES:
            raise ValueError(f"{path}: {len(data)} bytes exceeds DISK2_MAX_TRACK_BYTES ({DISK2_MAX_TRACK_BYTES})")
        tracks.append(data)

    lines = [
        f"/* Auto-generated from {args.nib_dir} by tools/gen_nib_disk_header.py -- do not hand-edit.",
        " * Matches disk2_controller.h's disk2_nibble_track_t layout exactly:",
        " *   uint8_t data[DISK2_MAX_TRACK_BYTES]; int length;",
        " * so this can be memcpy'd/cast directly into a disk2_nibble_track_t[DISK2_MAX_TRACKS]",
        " * array at the call site (see src/main_qemu_disk2boot.c). */",
        f"#ifndef {args.name.upper()}_H",
        f"#define {args.name.upper()}_H",
        "#include <stdint.h>",
        "",
        f"#define {args.name.upper()}_NUM_TRACKS {DISK2_MAX_TRACKS}",
        f"#define {args.name.upper()}_MAX_TRACK_BYTES {DISK2_MAX_TRACK_BYTES}",
        "",
        f"static const int {args.name}_track_lengths[{DISK2_MAX_TRACKS}] = {{",
        "    " + ",".join(str(len(t)) for t in tracks) + ",",
        "};",
        "",
        f"static const uint8_t {args.name}_track_data[{DISK2_MAX_TRACKS}][{DISK2_MAX_TRACK_BYTES}] = {{",
    ]
    for t in tracks:
        lines.append("{")
        padded = t + bytes(DISK2_MAX_TRACK_BYTES - len(t))
        for i in range(0, len(padded), 20):
            lines.append("    " + ",".join(str(b) for b in padded[i:i + 20]) + ",")
        lines.append("},")
    lines += ["};", "", "#endif"]

    args.output.write_text("\n".join(lines) + "\n")
    total = sum(len(t) for t in tracks)
    print(f"Wrote {args.output}: {len(tracks)} tracks, {total} real nibble bytes "
          f"({len(tracks) * DISK2_MAX_TRACK_BYTES} bytes allocated/padded)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
