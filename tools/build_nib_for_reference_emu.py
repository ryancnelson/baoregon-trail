#!/usr/bin/env python3
"""
tools/build_nib_for_reference_emu.py -- SCRATCH conversion helper, only
for producing standard fixed-size .nib files (35 tracks x 6656 bytes
each, 232960 bytes total) for reinette-II-plus's native SDL2 build
(which requires exactly this format, see reinetteII+.c's insertFloppy():
fread(disk[drv].data, 1, 232960, f)).

Wraps tools/dsk_to_nib.py's dsk_to_nibble_tracks() (which produces
correct but variably-sized per-track nibble data, 6602-6632 bytes,
matching disk2_controller.c's own variable-length track model) and pads
each track up to the fixed 6656-byte slot with 0xFF sync-gap filler
bytes (matching the existing gap-filler convention already used between
sectors in explode_sector_16()) so the result is a valid, standard-format
.nib file for an external reference tool.

This does NOT touch tools/dsk_to_nib.py, disk2_controller.c, or any
project format used by our own emulator -- purely external reference
tooling per Ryan's task (get a REAL, WORKING second reference to diff
against, using reinette-II-plus's own native SDL2 build).
"""
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO_ROOT / "tools"))
from dsk_to_nib import dsk_to_nibble_tracks, DEFAULT_VOLUME  # noqa: E402

NIB_TRACK_SIZE = 6656
NIB_TRACKS = 35
NIB_TOTAL_SIZE = NIB_TRACK_SIZE * NIB_TRACKS


def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    if len(argv) < 2:
        print("usage: build_nib_for_reference_emu.py <input.dsk> <output.nib> [volume]", file=sys.stderr)
        return 1

    dsk_path = Path(argv[0])
    nib_path = Path(argv[1])
    volume = int(argv[2]) if len(argv) > 2 else DEFAULT_VOLUME

    dsk_data = dsk_path.read_bytes()
    tracks = dsk_to_nibble_tracks(dsk_data, volume)

    out = bytearray()
    for i, track_data in enumerate(tracks):
        if len(track_data) > NIB_TRACK_SIZE:
            print(f"error: track {i} is {len(track_data)} bytes, exceeds fixed "
                  f"{NIB_TRACK_SIZE}-byte .nib track slot", file=sys.stderr)
            return 1
        padded = bytes(track_data) + bytes([0xFF] * (NIB_TRACK_SIZE - len(track_data)))
        out.extend(padded)

    assert len(out) == NIB_TOTAL_SIZE, f"expected {NIB_TOTAL_SIZE}, got {len(out)}"
    nib_path.write_bytes(bytes(out))
    print(f"Wrote {nib_path} ({len(out)} bytes, {NIB_TRACKS} tracks x {NIB_TRACK_SIZE} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
