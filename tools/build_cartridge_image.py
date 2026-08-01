"""
tools/build_cartridge_image.py -- pack multiple .dsk game images into a
single flat binary matching cartridge_slots[]'s ReRAM layout
(src/cartridge_layout.h), for flashing the whole multi-game cartridge
partition in one shot.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from embed_disk import read_dsk_bytes, DOS33_DISK_IMAGE_SIZE

CARTRIDGE_SLOT_COUNT = 6
CARTRIDGE_SLOT_SIZE = DOS33_DISK_IMAGE_SIZE
ERASED_FILL_BYTE = 0xFF


class SlotIndexError(ValueError):
    """Raised when a requested slot index is outside [0, CARTRIDGE_SLOT_COUNT)."""


def build_cartridge_image(slot_paths: dict[int, Path]) -> bytes:
    """Build the flat multi-slot cartridge image."""
    for slot_index in slot_paths:
        if not (0 <= slot_index < CARTRIDGE_SLOT_COUNT):
            raise SlotIndexError(
                f"Slot index {slot_index} out of range [0, {CARTRIDGE_SLOT_COUNT})"
            )

    out = bytearray(bytes([ERASED_FILL_BYTE]) * (CARTRIDGE_SLOT_COUNT * CARTRIDGE_SLOT_SIZE))

    for slot_index, path in slot_paths.items():
        data = read_dsk_bytes(path)
        offset = slot_index * CARTRIDGE_SLOT_SIZE
        out[offset : offset + CARTRIDGE_SLOT_SIZE] = data

    return bytes(out)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="Pack multiple .dsk files into a flat ReRAM cartridge partition binary."
    )
    for i in range(CARTRIDGE_SLOT_COUNT):
        parser.add_argument(
            f"--slot{i}",
            type=Path,
            help=f"Path to .dsk image for slot {i}",
        )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        required=True,
        help="Path to write the combined cartridge binary",
    )

    args = parser.parse_args(argv)

    slot_paths = {}
    for i in range(CARTRIDGE_SLOT_COUNT):
        p = getattr(args, f"slot{i}")
        if p is not None:
            slot_paths[i] = p

    try:
        data = build_cartridge_image(slot_paths)
        args.output.write_bytes(data)
    except OSError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    except Exception as exc:
        print(f"Error building cartridge image: {exc}", file=sys.stderr)
        return 1
    print(
        f"Wrote {len(data)} bytes ({len(slot_paths)}/{CARTRIDGE_SLOT_COUNT} slots populated) to {args.output}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
