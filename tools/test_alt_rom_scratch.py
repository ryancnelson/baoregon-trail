#!/usr/bin/env python3
"""
tools/test_alt_rom_scratch.py -- PARALLEL/SCRATCH test, does NOT touch
src/apple2e_system_rom.h or the real extraction path
(tools/extract_apple2e_system_rom.py). Tests whether an alternate ROM
image with genuine Applesoft BASIC resolves the DOS 3.3 banner-text gap
(the current Monitor-only ROM set has no Applesoft, see
NEXT_STEPS.md's DOS 3.3 banner-text investigation).

Candidate ROMs (real files in ~/devel/retrobios/bios/Apple/Apple II/),
verified via direct byte inspection before use, not assumption:

1. apple2-asoft-auto.rom (12288 bytes) -- confirmed via per-2KB-chunk
   SHA1 comparison against MAME's known apple2p (Apple II+) romset
   entries to be a byte-for-byte concatenation of:
     341-0011.d0 ($D000-$D7FF) Applesoft BASIC part 1
     341-0012.d8 ($D800-$DFFF) Applesoft BASIC part 2
     341-0013.e0 ($E000-$E7FF) Applesoft BASIC part 3
     341-0014.e8 ($E800-$EFFF) Applesoft BASIC part 4
     341-0015.f0 ($F000-$F7FF) Applesoft BASIC part 5
     341-0020-00.f8 ($F800-$FFFF) Autostart Monitor ROM
   Base address $D000, NOT $C000 -- this is a 12KB image covering
   $D000-$FFFF only (real Apple II+ has no ROM at $C000-$CFFF, that's
   peripheral card I/O space). Confirmed with real SHA1 comparison,
   not assumed from the 12KB size alone.

2. apple2.zip -- inspected and REJECTED as a candidate: its plain
   "apple2" driver chip set (341-0001/0002/0003/0004 + 341-0016-00.d0)
   is the ORIGINAL Apple II's Integer BASIC ROM, not Applesoft. DOS
   3.3's HELLO-triggered banner print needs Applesoft specifically;
   Integer BASIC has a different (incompatible) startup/HELLO
   mechanism. Not tested further for this reason -- documented here
   with the real SHA1s so this isn't re-discovered by guesswork.

This script builds a 16KB $C000-$FFFF composite image for our existing
apple2_mem_load_system_rom() API (which expects the full 16KB window):
  $C000-$CFFF: zeroed (real Apple II+ has no ROM here either -- this
               matches roms/apple2e.zip's real IIe ROM which is ALSO
               blank in the $C000-$C0FF softswitch-adjacent region,
               see src/apple2e_system_rom.h's own history notes)
  $D000-$FFFF: the real 12KB apple2-asoft-auto.rom content

Writes to build-scratch/ (gitignored-equivalent scratch dir), NOT
src/apple2e_system_rom.h.
"""
import hashlib
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SCRATCH_DIR = REPO_ROOT / "build-scratch"
ROM_SRC = Path.home() / "devel" / "retrobios" / "bios" / "Apple" / "Apple II" / "apple2-asoft-auto.rom"

# Known real apple2p (Apple II+ Autostart) chip SHA1s, from `mame -listroms apple2p`,
# in $D000-$FFFF address order -- used to verify the candidate file's real byte
# content before treating it as a drop-in Applesoft+Monitor ROM.
EXPECTED_CHUNK_SHA1S = [
    "0287ebcef2c1ce11dc71be15a99d2d7e0e128b1e",  # 341-0011.d0 ($D000)
    "a75ce5aab6401355bf1ab01b04e4946a424879b5",  # 341-0012.d8 ($D800)
    "8d82a1da63224859bd619005fab62c4714b25dd7",  # 341-0013.e0 ($E000)
    "37501be96d36d041667c15d63e0c1eff2f7dd4e9",  # 341-0014.e8 ($E800)
    "e6bf91ed28464f42b807f798fc6422e5948bf581",  # 341-0015.f0 ($F000)
    "a28852ff997b4790e53d8d0352112c4b1a395098",  # 341-0020-00.f8 ($F800, autostart monitor)
]


def main():
    if not ROM_SRC.exists():
        print(f"error: {ROM_SRC} not found", file=sys.stderr)
        sys.exit(1)

    rom_bytes = ROM_SRC.read_bytes()
    print(f"Loaded {ROM_SRC}: {len(rom_bytes)} bytes")
    if len(rom_bytes) != 12288:
        print(f"error: expected 12288 bytes (6x2KB chips), got {len(rom_bytes)}", file=sys.stderr)
        sys.exit(1)

    # Verify real byte content chunk-by-chunk against known chip SHA1s --
    # do NOT assume this is Applesoft just because it's 12KB.
    chunks = [rom_bytes[i:i + 2048] for i in range(0, len(rom_bytes), 2048)]
    for i, (chunk, expected) in enumerate(zip(chunks, EXPECTED_CHUNK_SHA1S)):
        actual = hashlib.sha1(chunk).hexdigest()
        status = "OK" if actual == expected else "MISMATCH"
        print(f"  chunk {i} (${0xD000 + i*2048:04X}): sha1={actual} expected={expected} [{status}]")
        if actual != expected:
            print(f"error: chunk {i} does not match expected real apple2p chip content", file=sys.stderr)
            sys.exit(1)

    print("All 6 chunks verified authentic (Applesoft $D000-$F7FF + Autostart Monitor $F800-$FFFF).")

    # Build the 16KB $C000-$FFFF composite: zero-fill $C000-$CFFF, real
    # ROM content at $D000-$FFFF.
    composite = bytearray(16384)
    composite[0x1000:0x1000 + len(rom_bytes)] = rom_bytes  # offset 0x1000 = $D000 - $C000
    assert len(composite) == 16384

    SCRATCH_DIR.mkdir(parents=True, exist_ok=True)
    out_bin = SCRATCH_DIR / "alt_rom_asoft_auto_c000_ffff.bin"
    out_bin.write_bytes(bytes(composite))
    print(f"\nWrote {out_bin} (16384 bytes, \\$C000-\\$FFFF composite)")
    print(f"Composite SHA1: {hashlib.sha1(bytes(composite)).hexdigest()}")

    # Also emit a C header (same style as gen_rom_header.py, but written
    # to build-scratch/ -- NOT src/apple2e_system_rom.h).
    out_header = SCRATCH_DIR / "alt_rom_asoft_auto.h"
    with open(out_header, "w") as f:
        f.write("#ifndef ALT_ROM_ASOFT_AUTO_H\n#define ALT_ROM_ASOFT_AUTO_H\n\n#include <stdint.h>\n\n")
        f.write("/* SCRATCH TEST ROM -- apple2-asoft-auto.rom (Apple II+ Autostart,\n")
        f.write(" * real Applesoft BASIC $D000-$F7FF + Autostart Monitor $F800-$FFFF)\n")
        f.write(" * padded with zeros at $C000-$CFFF for apple2_mem_load_system_rom()'s\n")
        f.write(" * 16KB window. NOT the project's real ROM (src/apple2e_system_rom.h) --\n")
        f.write(" * parallel/scratch test only, see tools/test_alt_rom_scratch.py. */\n")
        f.write(f"static const uint8_t g_alt_rom_asoft_auto[16384] = {{\n")
        for i, b in enumerate(composite):
            f.write(f"0x{b:02X},")
            if (i + 1) % 16 == 0:
                f.write("\n")
        f.write("};\n\n#endif\n")
    print(f"Wrote {out_header}")


if __name__ == "__main__":
    main()
