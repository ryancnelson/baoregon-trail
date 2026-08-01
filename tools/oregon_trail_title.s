; tools/oregon_trail_title.s -- boots directly into a real, converted
; Oregon Trail title screen bitmap (tools/oregon_trail_hires.bin, see
; tools/convert_image_to_hires.py) baked in as a data blob, copied to
; $2000-$3FFF, then switches to full-screen Hi-Res graphics.
;
; Softswitch addresses verified against src/apple2_mem.c:
;   $C050 = GRAPHICS (TEXT off)    $C052 = MIXED off (full-screen)
;   $C054 = PAGE2 off (page 1)     $C057 = HIRES on

.setcpu "6502"
.segment "CODE"
.org $0800

PTR_SRC = $04   ; source pointer (in this program's own data blob)
PTR_DST = $06   ; dest pointer ($2000+)

start:
    lda #$00
    sta PTR_SRC
    lda #$40          ; bitmap_data lives at $4000+ -- verified clear of the
    sta PTR_SRC+1     ; $2000-$3FFF destination range (see build note below)
    lda #$00
    sta PTR_DST
    lda #$20
    sta PTR_DST+1

copy_page:
    ldy #$00
copy_byte:
    lda (PTR_SRC),y
    sta (PTR_DST),y
    iny
    bne copy_byte

    inc PTR_SRC+1
    inc PTR_DST+1
    lda PTR_DST+1
    cmp #$40          ; reached $4000 (one past $3FFF)?
    bne copy_page

    lda #$00
    sta $C057         ; HIRES on
    sta $C052         ; MIXED off (full-screen, not split)
    sta $C050         ; GRAPHICS on (TEXT off)
    sta $C054         ; PAGE2 off (page 1)

halt:
    jmp halt

.segment "RODATA"
bitmap_data:
.incbin "oregon_trail_hires.bin"
