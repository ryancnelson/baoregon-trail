# Oregon Trail Title Screen Hi-Res Demo

Converts a real PNG (`tools/assets/oregon_trail_title.png`) into an Apple II
Hi-Res bitmap and boots it directly through the real emulator pipeline
(`apple2_mem.c` + `cpu6502.c` + `video_apple2.c`/`bio_display.c`), rendered
live via `tools/oregon_trail_runner.c`.

## Pipeline

1. **Convert the source image** (`tools/convert_image_to_hires.py`): loads
   the PNG, greyscale-thresholds each pixel to 1-bit black/white (no dithering
   yet -- a v1 simplification, see the script's own docstring), and packs it
   into the real 8KB Hi-Res buffer layout using the *actual*
   `hires_line_offsets[]` table from `src/video_apple2.c` (extracted
   programmatically via regex, never retyped by hand -- avoids the exact
   class of transcription bug `tools/checkerboard.s` hit and fixed earlier).
   Run from the repo root (the script reads `src/video_apple2.c` via a
   relative path):
   ```
   python3 tools/convert_image_to_hires.py tools/assets/oregon_trail_title.png tools/oregon_trail_hires.bin
   ```

2. **Assemble the bootloader** (`tools/oregon_trail_title.s`, ca65/ld65 --
   `brew install cc65`): copies an 8KB bitmap blob to $2000-$3FFF, then
   switches to full-screen Hi-Res graphics ($C057/$C052/$C050/$C054 --
   verified against `src/apple2_mem.c`'s real softswitch dispatch, not
   assumed).
   ```
   cd tools && ca65 oregon_trail_title.s -o oregon_trail_title.o
   ld65 -C oregon_trail_title.cfg -o oregon_trail_title.bin oregon_trail_title.o
   ```
   This produces **two separate output files**, not one:
   - `oregon_trail_title.bin` (52 bytes) -- the 6502 code, loads at `$0800`
   - `oregon_trail_title_data.bin` (8192 bytes) -- the bitmap blob, loads at
     `$4000` (see `oregon_trail_title.cfg`'s `RODATA` memory area)

   **Why two files, and why $4000 specifically (real bug found + fixed):**
   The first version of this demo linked the bitmap data right after the
   code (~`$0900`) and had the 6502 copy it from there to `$2000-$3FFF` in a
   single contiguous load-and-jump, same pattern as `checkerboard_demo.dsk`.
   That looked fine in isolation (verified the assembled `.bin`'s bitmap
   bytes matched the source file exactly) but was WRONG at runtime: with an
   8192-byte source blob starting at `$0900`, the source range extends to
   `$0900 + $2000 = $2900` -- which overlaps the `$2000-$3FFF` destination
   range the copy loop is simultaneously writing to. Partway through the
   copy, `PTR_SRC` catches up to `$2000` and starts reading bytes the same
   loop had *just written*, corrupting the back half of the image (confirmed
   by diffing the emulator's actual `$2000-$3FFF` memory against the source
   `.bin` byte-for-byte: 530/8192 bytes differed, starting exactly at the
   byte offset where `PTR_SRC` reached `$2000`). This produced a real,
   visually confirmed rendering bug -- a "ghosted" second, incorrectly
   offset copy of the text bleeding through the real image.

   Fix: put the bitmap data at `$4000+`, fully outside the `$2000-$3FFF`
   destination range, so source and destination never overlap. Since ld65
   concatenates multiple `MEMORY` regions into one flat file when they share
   `file = %O`, discarding the real address gap between them, the `.cfg`
   routes `RODATA` to its own separate output file
   (`oregon_trail_title_data.bin`) instead -- `oregon_trail_runner.c` then
   loads each file at its own real address explicitly, rather than relying
   on a single contiguous blob-at-$0800 load (which is what
   `hires_demo_runner.c`/`checkerboard_demo.dsk` still correctly does, since
   that demo's tiny inline data never had this overlap problem).

3. **Pack into a .dsk** -- NOT CURRENTLY WORKING for this two-file demo.
   `tools/pack_oregon_trail_title_dsk.py` only packs the single 52-byte code
   file into a `.dsk`, which is stale/incomplete now that the bitmap lives in
   a separate file at a different load address -- a `.dsk` alone can't
   represent two files loading to two different addresses without a real
   DOS-boot-chain (which this demo intentionally bypasses, same as
   `checkerboard_demo.dsk`). **Don't use the packed `.dsk` for this demo --
   run directly from the two `.bin` files instead (step 4).** Fixing this
   properly would mean either a custom two-segment container format or
   accepting the RODATA blob has to be re-derived from
   `oregon_trail_hires.bin` directly rather than round-tripped through a
   `.dsk`.

4. **Run live** (`tools/oregon_trail_runner.c` -- a dedicated two-file loader,
   not `hires_demo_runner.c`, because this demo needs two different load
   addresses):
   ```
   cc -std=c99 -Wall -Wextra -g -Isrc -DFB_TERMINAL_VIEWER_NO_MAIN \
     -o tools/bin/oregon_trail_runner tools/oregon_trail_runner.c tools/fb_terminal_viewer.c \
     src/apple2_mem.c src/cpu6502.c src/disk_sector_layout.c src/disk_trap.c \
     src/bunnie_audio.c src/video_apple2.c src/bio_display.c src/lores_apple2.c
   ./tools/bin/oregon_trail_runner tools/oregon_trail_title.bin tools/oregon_trail_title_data.bin
   ```
   Built to `tools/bin/`, not `build/` -- the shared crew `build/` directory
   gets wiped by `make clean` on ~5min automated cadence (see
   `MEMO_BUILD_LOCK_PROTOCOL.md`), which would silently delete this binary
   since it isn't a Makefile-tracked test target.

## Verified

Ran through the real pipeline (not assumed): `apple2_mem_is_hires_mode()`
reports 1 (HIRES) and `is_text_mode()` reports 0 after boot. Confirmed the
overlap-bug fix directly by diffing the emulator's actual `$2000-$3FFF`
memory against `oregon_trail_hires.bin` byte-for-byte after boot: **0/8192
bytes differ** (was 530/8192 before the fix). Visually confirmed by the user
in a live terminal render -- correct image, no ghosting, matches the real
MobyGames screenshot of the Apple II title screen.
