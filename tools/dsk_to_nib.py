"""
tools/dsk_to_nib.py -- convert a standard DOS-order 140 KiB .dsk image
(35 tracks x 16 sectors x 256 bytes, flat "DOS order" -- see
src/disk_sector_layout.h) into per-track raw nibble-encoded (6-and-2 GCR)
data suitable for disk2_controller.c's disk2_controller_load_nibble_disk().

Ported from whscullin/apple2js (https://github.com/whscullin/apple2js),
specifically js/formats/format_utils.ts's explodeSector16() and
js/formats/do.ts's createDiskFromDOS() (MIT license -- see LICENSE_APPLE2JS
below for the full notice, reproduced per MIT's attribution requirement).

NEXT_STEPS.md Step 7: real, unmodified Apple II disk images (Zork I,
Apple DOS 3.3 System Master) execute JMP ($003E) into the Disk II
peripheral card's own ROM, which drives disk2_controller.c's
$C0E0-$C0EF softswitches to read raw nibbles off a (simulated) magnetic
track -- NOT disk_trap.c's fast-sector-read shortcut, which only
understands flat sector data. This tool bridges the gap: our existing
.dsk assets (dos33_sample.dsk, checkerboard_demo.dsk, hires_demo.dsk,
and eventually real commercial images) are all flat DOS-order sector
data; disk2_controller.c needs nibble-encoded track data instead.

============================================================================
LICENSE_APPLE2JS -- MIT License notice, REQUIRED per apple2js's license
terms since this module's algorithm (explodeSector16/DO sector-order
table) is a direct port of its code:

The MIT License (MIT)

Copyright (c) 2010-2021 Will Scullin and contributors

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
============================================================================
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

TRACKS = 35
SECTORS_PER_TRACK = 16
SECTOR_SIZE = 256
DOS33_DISK_IMAGE_SIZE = TRACKS * SECTORS_PER_TRACK * SECTOR_SIZE  # 143360

# DOS 3.3 Physical->DOS sector order (index is physical sector, value is
# DOS sector) -- ported verbatim from format_utils.ts's DO[] table. This
# is real, documented Apple II hardware/DOS behavior, not something to
# hand-derive.
DO = [
    0x0, 0x7, 0xE, 0x6, 0xD, 0x5, 0xC, 0x4,
    0xB, 0x3, 0xA, 0x2, 0x9, 0x1, 0x8, 0xF,
]

# 6-and-2 GCR translate table -- ported verbatim from format_utils.ts's
# TRANS62[].
TRANS62 = [
    0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6,
    0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3,
    0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC,
    0xBD, 0xBE, 0xBF, 0xCB, 0xCD, 0xCE, 0xCF, 0xD3,
    0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE,
    0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC,
    0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
    0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
]

DEFAULT_VOLUME = 254  # matches apple2js's setBinary() default volume number


def four_x_four(val: int) -> tuple[int, int]:
    """4-and-4 encoding used for the address-field header bytes (volume,
    track, sector, checksum) -- ported verbatim from format_utils.ts's
    fourXfour()."""
    xx = val & 0xAA
    yy = val & 0x55
    xx >>= 1
    xx |= 0xAA
    yy |= 0xAA
    return xx, yy


def explode_sector_16(volume: int, track: int, sector: int, data: bytes) -> list[int]:
    """Nibblizes one 256-byte sector into its full on-disk representation
    (sync gap + address field + data field), 6-and-2 GCR encoding.
    Ported verbatim from format_utils.ts's explodeSector16() -- this is
    real, documented Apple II Disk II low-level track format, not
    something to hand-derive or simplify.
    """
    buf: list[int] = []

    # Gap 1/3
    if sector == 0:
        gap = 0x80
    else:
        gap = 0x28 if track == 0 else 0x26
    buf.extend([0xFF] * gap)

    # Address field
    checksum = volume ^ track ^ sector
    buf.extend([0xD5, 0xAA, 0x96])  # Address Prolog
    buf.extend(four_x_four(volume))
    buf.extend(four_x_four(track))
    buf.extend(four_x_four(sector))
    buf.extend(four_x_four(checksum))
    buf.extend([0xDE, 0xAA, 0xEB])  # Epilog

    # Gap 2
    buf.extend([0xFF] * 0x05)

    # Data field
    buf.extend([0xD5, 0xAA, 0xAD])  # Data Prolog

    nibbles = [0] * 0x158
    ptr2 = 0
    ptr6 = 0x56

    idx2 = 0x55
    for idx6 in range(0x101, -1, -1):
        val6 = data[idx6 % 0x100]
        val2 = nibbles[ptr2 + idx2]

        val2 = (val2 << 1) | (val6 & 1)
        val6 >>= 1
        val2 = (val2 << 1) | (val6 & 1)
        val6 >>= 1

        nibbles[ptr6 + idx6] = val6
        nibbles[ptr2 + idx2] = val2 & 0xFF

        idx2 -= 1
        if idx2 < 0:
            idx2 = 0x55

    last = 0
    for idx in range(0x156):
        val = nibbles[idx]
        buf.append(TRANS62[last ^ val])
        last = val
    buf.append(TRANS62[last])

    buf.extend([0xDE, 0xAA, 0xEB])  # Epilog

    # Gap 3
    buf.append(0xFF)

    return buf


def dsk_to_nibble_tracks(dsk_data: bytes, volume: int = DEFAULT_VOLUME) -> list[bytes]:
    """Converts a flat 143360-byte DOS-order .dsk image into a list of 35
    raw nibble-encoded track byte strings, ready for
    disk2_controller_load_nibble_disk(). Ported from do.ts's
    createDiskFromDOS()."""
    if len(dsk_data) != DOS33_DISK_IMAGE_SIZE:
        raise ValueError(
            f"expected {DOS33_DISK_IMAGE_SIZE} bytes (140 KiB DOS 3.3 "
            f"image), got {len(dsk_data)} bytes"
        )

    tracks: list[bytes] = []
    for t in range(TRACKS):
        track_nibbles: list[int] = []
        for physical_sector in range(SECTORS_PER_TRACK):
            dos_sector = DO[physical_sector]
            offset = (SECTORS_PER_TRACK * t + dos_sector) * SECTOR_SIZE
            sector_data = dsk_data[offset:offset + SECTOR_SIZE]
            track_nibbles.extend(
                explode_sector_16(volume, t, physical_sector, sector_data)
            )
        tracks.append(bytes(track_nibbles))
    return tracks


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Convert a DOS-order .dsk image into per-track raw "
                    "nibble-encoded (6-and-2 GCR) data."
    )
    parser.add_argument("dsk_path", type=Path, help="path to the .dsk file")
    parser.add_argument(
        "-o", "--output-dir", type=Path, default=None,
        help="directory to write track00.nib .. track34.nib into "
             "(default: print track lengths to stdout only, no files "
             "written)",
    )
    parser.add_argument(
        "--volume", type=int, default=DEFAULT_VOLUME,
        help=f"disk volume number for the address-field checksum "
             f"(default: {DEFAULT_VOLUME}, matches apple2js's default)",
    )
    args = parser.parse_args(argv)

    try:
        dsk_data = args.dsk_path.read_bytes()
    except OSError as exc:
        print(f"error: could not read {args.dsk_path}: {exc.strerror}", file=sys.stderr)
        return 1

    try:
        tracks = dsk_to_nibble_tracks(dsk_data, args.volume)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    if args.output_dir:
        try:
            args.output_dir.mkdir(parents=True, exist_ok=True)
            for i, track_data in enumerate(tracks):
                out_path = args.output_dir / f"track{i:02d}.nib"
                out_path.write_bytes(track_data)
        except OSError as exc:
            print(f"error: could not write to {args.output_dir}: {exc.strerror}", file=sys.stderr)
            return 1
        print(f"Wrote {len(tracks)} track files to {args.output_dir}")
    else:
        for i, track_data in enumerate(tracks):
            print(f"track {i:02d}: {len(track_data)} nibble bytes")

    return 0


if __name__ == "__main__":
    sys.exit(main())
