#!/usr/bin/env python3
"""
tools/check_linker_placement.py -- verify linker.ld actually placed each
ELF section in the physical memory region it's supposed to be in.

A linker script can silently "work" (link succeeds, produces an ELF) while
still placing a section in the wrong memory -- e.g. .data accidentally
landing in ReRAM instead of SRAM, or a section straddling a boundary. This
catches that class of bug the way CLAUDE.md's TDD discipline demands:
verify the real, documented layout before trusting it, rather than
eyeballing linker.ld and assuming it's correct.

Usage: check_linker_placement.py path/to/baoregon.elf
Exits non-zero (with details) if any section is misplaced.
"""
import re
import subprocess
import sys
import os

RERAM_START = 0x60000000
RERAM_END = RERAM_START + 4 * 1024 * 1024  # 4.0 MiB
SRAM_START = 0x61000000
SRAM_END = SRAM_START + 2 * 1024 * 1024  # 2.0 MiB

# Which physical memory each named ELF section must fall entirely within.
# NOLOAD sections (.bss, .stack) still get an address assigned by the
# linker and show up in readelf -S output, so this check covers them too.
EXPECTED_REGION = {
    ".text": "RERAM",
    ".rodata": "RERAM",
    ".dsk_images": "RERAM",
    ".data": "SRAM",
    ".bss": "SRAM",
    ".stack": "SRAM",
}

REGIONS = {
    "RERAM": (RERAM_START, RERAM_END),
    "SRAM": (SRAM_START, SRAM_END),
}


class PlacementError(Exception):
    pass


def parse_readelf_sections(elf_path: str) -> dict:
    """Run `readelf -S` (or riscv64-elf-readelf) and parse section
    name -> (addr, size). Tries a few common cross-toolchain binary names."""
    candidates = ["riscv64-elf-readelf", "riscv32-unknown-elf-readelf", "readelf"]
    last_err = None
    for binname in candidates:
        try:
            result = subprocess.run(
                [binname, "-S", "-W", elf_path],
                capture_output=True, text=True, check=True,
            )
            return _parse_readelf_output(result.stdout)
        except FileNotFoundError as exc:
            last_err = exc
            continue
    raise RuntimeError(f"no readelf binary found (tried {candidates}): {last_err}")


def _parse_readelf_output(output: str) -> dict:
    """Parse the section header table from `readelf -S -W` output.

    Typical row (wide format):
      [ 1] .text  PROGBITS  60000000  001000  000123  00  AX  0  0  4
    Columns: [Nr] Name Type Addr Off Size ES Flg Lk Inf Al
    """
    sections = {}
    row_re = re.compile(
        r"^\s*\[\s*\d+\]\s+(\S+)\s+\S+\s+([0-9A-Fa-f]+)\s+[0-9A-Fa-f]+\s+([0-9A-Fa-f]+)\s"
    )
    for line in output.splitlines():
        match = row_re.match(line)
        if not match:
            continue
        name, addr_hex, size_hex = match.groups()
        sections[name] = (int(addr_hex, 16), int(size_hex, 16))
    return sections


def check_placement(sections: dict) -> list:
    """Return a list of human-readable error strings (empty if all OK)."""
    errors = []
    for section_name, region_name in EXPECTED_REGION.items():
        if section_name not in sections:
            # Section may legitimately be absent (e.g. .dsk_images before
            # any game is embedded) -- not an error, just nothing to check.
            continue
        addr, size = sections[section_name]
        if size == 0:
            continue  # empty section, nothing to place
        region_start, region_end = REGIONS[region_name]
        section_end = addr + size
        if addr < region_start or section_end > region_end:
            errors.append(
                f"{section_name}: [0x{addr:08X}, 0x{section_end:08X}) "
                f"is NOT fully within {region_name} "
                f"[0x{region_start:08X}, 0x{region_end:08X})"
            )
    return errors


def main(argv):
    if len(argv) != 2:
        print(f"usage: {argv[0]} path/to/file.elf", file=sys.stderr)
        return 2

    elf_path = argv[1]
    if not os.path.isfile(elf_path):
        print(f"error: no such file: {elf_path}", file=sys.stderr)
        return 1

    try:
        sections = parse_readelf_sections(elf_path)
    except subprocess.CalledProcessError as exc:
        stderr_tail = (exc.stderr or "").strip().splitlines()[-1:] or [""]
        print(f"error: readelf failed on {elf_path}: {stderr_tail[0]}", file=sys.stderr)
        return 1
    except RuntimeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    errors = check_placement(sections)

    if errors:
        print("FAIL: linker placement check found misplaced section(s):", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        return 1

    checked = [name for name in EXPECTED_REGION if name in sections]
    print(f"PASS: all {len(checked)} present section(s) correctly placed: {checked}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
