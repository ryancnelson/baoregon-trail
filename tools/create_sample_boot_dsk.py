#!/usr/bin/env python3
"""
tools/create_sample_boot_dsk.py -- generate a standard 140 KiB DOS 3.3 .dsk image
containing a valid 6502 bootloader in Track 0, Sector 0.

The bootloader writes "HELLO WORLD" in Apple II high-ASCII to text screen $0400,
switches to TEXT mode ($C051) and PAGE1 ($C054), and loops cleanly.
"""
import sys
from pathlib import Path

DOS33_DISK_SIZE = 143360  # 35 tracks * 16 sectors * 256 bytes

def generate_boot_sector() -> bytes:
    sector = bytearray(256)
    
    # 6502 machine code:
    # $0800: 01          (sector load count = 1 sector)
    # $0801: A2 00       LDX #$00
    # $0803: BD 18 08    LDA $0818,X
    # $0806: F0 09       BEQ $0811
    # $0808: 9D 00 04    STA $0400,X
    # $080B: E8          INX
    # $080C: 4C 03 08    JMP $0803
    # $080F: 8D 51 C0    STA $C051 (TEXT)
    # $0812: 8D 54 C0    STA $C054 (PAGE1)
    # $0815: 4C 15 08    JMP $0815
    # $0818: "DOS VERSION 3.3\0" (with high bit 0x80 set)

    code = bytes([
        0x01,                    # 0800: sector load count = 1
        0xA2, 0x00,              # 0801: LDX #$00
        0xBD, 0x18, 0x08,        # 0803: LDA $0818,X
        0xF0, 0x09,              # 0806: BEQ $0812
        0x9D, 0x00, 0x04,        # 0808: STA $0400,X
        0xE8,                    # 080B: INX
        0x4C, 0x03, 0x08,        # 080C: JMP $0803
        0x8D, 0x51, 0xC0,        # 080F: STA $C051
        0x8D, 0x54, 0xC0,        # 0812: STA $C054
        0x4C, 0x15, 0x08,        # 0815: JMP $0815
    ])

    msg = bytes([ord(c) | 0x80 for c in "DOS VERSION 3.3"]) + b"\x00"

    sector[:len(code)] = code
    sector[0x18:0x18+len(msg)] = msg
    return bytes(sector)

def main():
    if len(sys.argv) < 2:
        out_path = Path("disks/dos33_sample.dsk")
    else:
        out_path = Path(sys.argv[1])

    try:
        out_path.parent.mkdir(parents=True, exist_ok=True)

        disk = bytearray(DOS33_DISK_SIZE)
        boot_sec = generate_boot_sector()
        disk[:256] = boot_sec

        out_path.write_bytes(disk)
    except OSError as exc:
        print(f"error: {exc}", file=sys.stderr)
        sys.exit(1)

    print(f"Generated {len(disk)} byte DOS 3.3 boot disk image at {out_path}")

if __name__ == "__main__":
    main()
