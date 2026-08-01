#!/usr/bin/env python3
"""
tools/create_hires_demo_dsk.py -- generate a standard 140 KiB DOS 3.3 .dsk
image containing a bootloader that paints a visible Hi-Res color pattern
and switches the softswitches to full-screen Hi-Res graphics mode.

Unlike create_sample_boot_dsk.py (TEXT mode "HELLO WORLD" -- renders BLACK
right now, since no character-ROM text renderer exists yet per
src/bio_display.c), this exercises the Hi-Res color pipeline, which IS
implemented and tested (video_apple2.c / bio_display.c color decode).

Pattern: fills all 8192 bytes of the Hi-Res buffer ($2000-$3FFF) with 0x7F
(all 7 low bits set = 7 lit pixels per byte, high bit 0 = "violet/blue"
palette per Apple II hi-res artifacting rules) -- produces solid vertical
color bars across the full 280x192 frame, not a blank/black screen.

6502 code (assembled by hand, same style as create_sample_boot_dsk.py):
  $0800: A2 00          LDX #$00
  $0802: A9 7F          LDA #$7F
  loop (fills $2000-$20FF via absolute,X, then bumps a page-hi counter
  using a zero-page byte, 32 pages = 8192 bytes total):
  ... see generate_boot_sector() below for the exact byte sequence.
"""
import sys
from pathlib import Path

DOS33_DISK_SIZE = 143360  # 35 tracks * 16 sectors * 256 bytes


def generate_boot_sector() -> bytes:
    sector = bytearray(256)

    # 6502 machine code, assembled by hand:
    #
    # $0800: A9 7F       LDA #$7F        ; pattern byte -> lit pixels, low palette
    # $0802: A2 00       LDX #$00        ; inner loop index (0..255)
    # $0804: 85 06       STA $06          ; zp $06 = pattern byte (freed A for STA use)
    #                    -- actually simpler: keep A loaded, use Y as page counter
    #
    # Revised, simpler plan (avoid needing indirect addressing):
    # zp $04/$05 = 16-bit pointer into Hi-Res buffer, walked from $2000 to $3FFF.
    #
    # $0800: A9 7F       LDA #$7F         ; A = fill byte
    # $0802: 85 06       STA $06          ; $06 = fill byte (spare, not strictly needed)
    # $0804: A9 00       LDA #$00
    # $0806: 85 04       STA $04          ; ptrlo = $00
    # $0808: A9 20       LDA #$20
    # $080A: 85 05       STA $05          ; ptrhi = $20  -> ptr = $2000
    # $080C: A0 00       LDY #$00         ; Y = 0 (byte-within-page index)
    # loop:
    # $080E: A9 7F       LDA #$7F
    # $0810: 91 04       STA ($04),Y      ; store fill byte at [ptr + Y]
    # $0812: C8          INY
    # $0813: D0 F9       BNE loop         ; loop while Y != 0 (256 iters -> wraps to 0)
    # $0815: E6 05       INC $05          ; ptrhi++  (advance to next 256-byte page)
    # $0817: A5 05       LDA $05
    # $0819: C9 40       CMP #$40         ; reached $4000? (one past $3FFF, end of Hi-Res)
    # $081B: D0 F1       BNE loop         ; keep going if not yet at $4000
    # $081D: 8D 57 C0    STA $C057        ; HIRES on
    # $0820: 8D 52 C0    STA $C052        ; MIXED off (full-screen graphics, not split)
    # $0823: 8D 50 C0    STA $C050        ; GRAPHICS (TEXT off)
    # $0826: 8D 54 C0    STA $C054        ; PAGE2 off (use page 1, $2000 base)
    # $0829: 4C 29 08    JMP $0829        ; halt (self-jump, matches other bootloader style)

    code = bytes([
        0xA9, 0x7F,              # 0800: LDA #$7F
        0x85, 0x06,              # 0802: STA $06 (spare/documentation only)
        0xA9, 0x00,              # 0804: LDA #$00
        0x85, 0x04,              # 0806: STA $04       ; ptrlo = 0
        0xA9, 0x20,              # 0808: LDA #$20
        0x85, 0x05,              # 080A: STA $05       ; ptrhi = $20 -> ptr=$2000
        0xA0, 0x00,              # 080C: LDY #$00
        # loop ($080E):
        0xA9, 0x7F,              # 080E: LDA #$7F
        0x91, 0x04,              # 0810: STA ($04),Y
        0xC8,                    # 0812: INY
        0xD0, 0xF9,              # 0813: BNE loop (-7 -> $080E)
        0xE6, 0x05,              # 0815: INC $05
        0xA5, 0x05,              # 0817: LDA $05
        0xC9, 0x40,              # 0819: CMP #$40
        0xD0, 0xF1,              # 081B: BNE loop (-15 -> $080E)
        0x8D, 0x57, 0xC0,        # 081D: STA $C057     ; HIRES on
        0x8D, 0x52, 0xC0,        # 0820: STA $C052     ; MIXED off
        0x8D, 0x50, 0xC0,        # 0823: STA $C050     ; GRAPHICS on (TEXT off)
        0x8D, 0x54, 0xC0,        # 0826: STA $C054     ; PAGE2 off
        0x4C, 0x29, 0x08,        # 0829: JMP $0829     ; halt
    ])

    sector[: len(code)] = code
    return bytes(sector)


def main():
    if len(sys.argv) < 2:
        out_path = Path("disks/hires_demo.dsk")
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

    print(f"Generated {len(disk)} byte DOS 3.3 Hi-Res demo disk image at {out_path}")


if __name__ == "__main__":
    main()
