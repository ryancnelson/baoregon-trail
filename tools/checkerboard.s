; tools/checkerboard.s -- Hi-Res checkerboard pattern bootloader, assembled
; with ca65 (cc65 toolchain).
;
; Fills the Hi-Res buffer ($2000-$3FFF) with an actual 2D checkerboard by
; iterating through the REAL, non-contiguous Apple II Hi-Res scanline
; address table (src/video_apple2.c's hires_line_offsets[]) rather than
; assuming rows are 40 linear bytes apart -- Hi-Res scanlines are
; interleaved (row 0 at $0000, row 1 at $0400, row 8 at $0080, etc), so a
; naive linear fill produces vertical-only stripes, not a checkerboard
; (this is exactly the bug in the first version of this file).
;
; For each of the 192 real display rows (looked up via a 192-entry,
; 16-bit table baked into this program at assemble time), fill its 40
; bytes with a pattern whose starting phase depends on the row's OWN
; index (even/odd), not just a running byte counter -- this is what
; actually produces alternating checkering both horizontally (byte to
; byte within a row, via the EOR #$7F flip) AND vertically (row to row,
; via the per-row starting-phase flip based on row parity).
;
; Softswitch addresses verified directly against src/apple2_mem.c:
;   $C050 = GRAPHICS (TEXT off)    $C052 = MIXED off (full-screen)
;   $C054 = PAGE2 off (page 1)     $C057 = HIRES on

.setcpu "6502"
.org $0800

FILLBYTE  = $06   ; zero-page scratch: current fill byte for this row
PTR       = $04   ; zero-page scratch: 16-bit pointer ($04/$05)
ROWIDX    = $08   ; zero-page scratch: current row index (0-191)

start:
    lda #$00
    sta ROWIDX

rowlabel:
    ; ptr = $2000 + hires_line_offsets[ROWIDX]  (table lookup, 2 bytes/entry)
    ldx ROWIDX
    lda offsets_lo,x
    clc
    adc #$00          ; low byte of $2000 is $00, so this is a no-op add,
    sta PTR           ; kept explicit for clarity/future base-addr changes
    lda offsets_hi,x
    adc #$20          ; add $20 for the $2000 base page
    sta PTR+1

    ; starting phase for this row: even row -> $2A, odd row -> $55
    lda ROWIDX
    and #$01
    beq even_row
    lda #$55
    jmp got_phase
even_row:
    lda #$2A
got_phase:
    sta FILLBYTE

    ldy #$00
byteloop:
    lda FILLBYTE
    sta (PTR),y
    eor #$7F          ; flip every byte within the row -> horizontal checkering
    sta FILLBYTE
    iny
    cpy #40           ; Apple II Hi-Res is 40 bytes/scanline
    bne byteloop

    inc ROWIDX
    lda ROWIDX
    cmp #192          ; all 192 rows done?
    bne rowlabel

    lda #$00
    sta $C057         ; HIRES on
    sta $C052         ; MIXED off (full-screen, not split)
    sta $C050         ; GRAPHICS on (TEXT off)
    sta $C054         ; PAGE2 off (page 1)

halt:
    jmp halt

; --- 192-entry hires_line_offsets table, low/high bytes split into two
; parallel tables so `lda offsets_lo,x` / `lda offsets_hi,x` can use
; single-byte X-indexed addressing (6502 has no 16-bit indexed loads).
; Values copied directly from src/video_apple2.c's hires_line_offsets[]
; (this is a real, already-tested, already-verified table -- not
; re-derived or guessed here).
.include "checkerboard_offsets.inc"
