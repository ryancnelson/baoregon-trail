#ifndef APPLE2_MEM_H
#define APPLE2_MEM_H

#include <stdint.h>
#include "bunnie_audio.h"
#include "disk2_controller.h"

/*
 * apple2_mem.c -- 64KB Apple II memory map + $C000-$C0FF soft-switch
 * dispatch. Implements the read6502()/write6502() bus functions declared
 * in cpu6502.h (contract locked 2026-07-31: baochip/Woz/Bunnie/Duke).
 *
 * Address map:
 *   $0000-$CFFF, $E000-$FFFF : plain RAM (includes the $2000-$3FFF Hi-Res
 *                               buffer Bunnie's video_apple2.c reads via
 *                               its own read6502_fn parameter -- no special
 *                               casing needed here, it's just RAM).
 *   $C000-$C0FF              : soft-switch dispatch (see below).
 *   $D000-$FFFF              : Apple IIe Autostart ROM region (per
 *                               PROJECT_GOALS.md Milestone 3 / BRAINSTORM.md)
 *                               -- write-protected. NOTE: $D000-$FFFF
 *                               overlaps $E000-$FFFF above; ROM protection
 *                               wins for that overlap (writes there are
 *                               ignored, reads return whatever was loaded
 *                               at init/reset time). Real ROM image loading
 *                               is a later iteration (Step 5); for now the
 *                               region simply enforces write-protection
 *                               semantics over backing RAM bytes.
 *
 * Soft-switch dispatch inside $C000-$C0FF (per baochip's sketch, locked
 * 2026-07-31):
 *   $C000          : keyboard input latch (read) -- returns the last
 *                     injected key's ASCII value with the high bit
 *                     (0x80, "strobe") set if a key is pending.
 *   $C010          : keyboard strobe clear (any access, read or write).
 *   $C030          : Bunnie's speaker toggle -- calls
 *                     bunnie_audio_trigger_toggle() (fire-and-forget,
 *                     non-blocking, mechanism (b) memory-mapped flag).
 *   $C050/$C051    : GRAPHICS / TEXT mode select (any access).
 *   $C052/$C053    : FULL-screen / MIXED mode select (any access).
 *   $C054/$C055    : PAGE1 / PAGE2 select (any access).
 *   $C056/$C057    : LORES / HIRES mode select (any access).
 *   $C058-$C05F    : AN0-AN3 annunciator outputs, off/on pairs (any access).
 *   $C061/$C062/$C063 : PB0/PB1/PB2 pushbutton state (read) -- bit 7
 *                     (0x80) reflects the button's held/released state.
 *   $C064/$C065    : PADDLE0/PADDLE1 analog timer state (read) -- bit 7
 *                     (0x80) set while the RC countdown armed by $C070 is
 *                     still running.
 *   $C070          : PDRIVE -- arms the paddle RC countdown (any access).
 *   $C080-$C08F    : Language Card bank-switching -- see
 *                     apply_language_card_switch() in apple2_mem.c for
 *                     the full read/write/bank truth table.
 *   $C0E0          : Duke's disk trap -- stage a track number (write-only).
 *   $C0E1          : Duke's disk trap -- stage a sector number and select
 *                     the (track, sector) via disk_trap_select_sector();
 *                     resets the internal byte cursor to 0 (write-only).
 *   $C0EC          : Duke's disk trap -- data port. Each read returns the
 *                     next byte of the currently selected 256-byte sector
 *                     via disk_trap_read_byte(), auto-incrementing (and
 *                     wrapping mod 256) so DOS 3.3/ProDOS can stream a
 *                     whole sector with repeated reads of one address --
 *                     no physical GCR/stepper timing, per BRAINSTORM.md
 *                     section 4's explicit simplification.
 *   anything else in $C000-$C0FF : reads as 0x00, writes ignored (soft
 *                     switches not yet implemented are inert, not crashes).
 */

/* Reset the 64KB RAM to all zero, reset the disk trap's byte cursor, and
 * re-initialize Bunnie's audio state. Call once before use and between
 * tests for a clean, deterministic starting point. */
void apple2_mem_reset(void);

/* Loads a real (or synthetic) 16KB Apple IIe system firmware ROM image
 * covering $C000-$FFFF, backing reads at that range when the Language
 * Card softswitch state selects ROM (g_lc_read_ram == 0, the default
 * post-reset state -- matches real hardware).
 *
 * HOST-BUILD-ONLY, opt-in feature: without calling this, $D000-$FFFF
 * reads simply return zeroed g_ram[] (the RISC-V/ReRAM target's existing
 * behavior is completely unaffected -- this function is never called
 * from that build path). Added to let the host test/tools harnesses run
 * real Apple IIe system software (DOS 3.3, Applesoft) end-to-end for
 * integration testing, per BRAINSTORM.md section 8's external-sourcing
 * follow-up -- the real ROM bytes now exist locally (see
 * tools/README_ROMS.md) and this is what actually uses them.
 *
 * image must point to exactly 16384 bytes (the concatenation of
 * 342-0134-a.64 + 342-0135-b.64 in that order -- Part A is $C000-$DFFF,
 * Part B is $E000-$FFFF) and must outlive any subsequent read6502() calls.
 * Pass NULL to unload (reverts to the zeroed-RAM fallback). */
void apple2_mem_load_system_rom(const uint8_t *image);

/* Register the ReRAM-resident disk image backing the $C0E0-$C0EF disk
 * trap. Thin pass-through to disk_trap_set_image() -- see disk_trap.h. */
void apple2_mem_set_disk_image(const uint8_t *image);

/*
 * Disk II controller mode switch (NEXT_STEPS.md Step 7).
 *
 * $C0E0-$C0EF is a genuine address-space collision: disk_trap.c's
 * fast-sector-read shortcut and disk2_controller.c's real Disk II
 * softswitch/nibble emulation both need the exact same 16 addresses for
 * slot 6. Architecture decision (2026-08-01, confirmed with Ryan): a
 * runtime mode flag gates which one apple2_mem.c's read6502()/write6502()
 * actually dispatch $C0E0-$C0EF to -- the two are NEVER both wired up at
 * the same time.
 *
 *   APPLE2_MEM_DISK_CONTROLLER_DISK_TRAP (default, and restored by
 *     apple2_mem_reset()): disk_trap.c's existing fast-sector-read
 *     protocol -- used by the boot-splash menu's own synthetic
 *     bootloaders (checkerboard/hires demos, dos33_sample.dsk) and any
 *     existing test/tool that already depends on disk_trap.c's
 *     $C0E0/$C0E1/$C0EC contract. This is the ONLY mode that existed
 *     before this switch was added -- selecting it (or never calling
 *     apple2_mem_set_disk_controller_mode() at all) preserves every
 *     prior behavior exactly.
 *
 *   APPLE2_MEM_DISK_CONTROLLER_DISK2: disk2_controller.c's real Disk II
 *     phase/motor/Q6/Q7 softswitch dispatch + nibble-level track
 *     read/write -- needed to boot real, unmodified disk images (Zork
 *     I, Apple DOS 3.3 System Master) whose boot code drives these
 *     softswitches directly rather than calling disk_trap.c's
 *     shortcut.
 */
typedef enum {
    APPLE2_MEM_DISK_CONTROLLER_DISK_TRAP = 0,
    APPLE2_MEM_DISK_CONTROLLER_DISK2 = 1,
} apple2_mem_disk_controller_mode_t;

/* Selects which controller $C0E0-$C0EF routes to. Takes effect
 * immediately (does not itself reset either controller's internal
 * state -- disk_trap.c's selection state and disk2_controller.c's
 * drive/track state are independent and persist across a mode switch,
 * matching real hardware where swapping which peripheral card occupies
 * a slot doesn't affect the other card's own internal state). */
void apple2_mem_set_disk_controller_mode(apple2_mem_disk_controller_mode_t mode);

/* Exposes the single disk2_controller_t instance apple2_mem.c owns, so
 * callers (test harnesses, and eventually a real disk-image loader) can
 * call disk2_controller_load_nibble_disk() on it directly -- mirrors the
 * existing apple2_mem_set_disk_image()/disk_trap_set_image() pattern for
 * the disk_trap.c path. Never returns NULL. */
disk2_controller_t *apple2_mem_get_disk2_controller(void);

/* Expose Bunnie's audio state so BIO Core 1's polling loop (and tests) can
 * call bunnie_audio_poll_and_apply() on it. apple2_mem.c owns the single
 * instance; this returns a pointer to it, not a copy. */
bunnie_audio_state_t *apple2_mem_get_audio_state(void);

/* Display-mode softswitch state ($C050-$C057), read by Bunnie's
 * video_apple2.c to pick which decode path/page to render. All four are
 * plain booleans (1/0); default state after apple2_mem_reset() is TEXT
 * mode, PAGE1, LORES (matches real Apple II post-reset state) -- MIXED
 * defaults to off (full-screen). */
int apple2_mem_is_text_mode(void);   /* 1 = TEXT ($C051), 0 = GRAPHICS ($C050) */
int apple2_mem_is_mixed_mode(void);  /* 1 = MIXED ($C053), 0 = FULL ($C052) */
int apple2_mem_is_page2_selected(void); /* 1 = PAGE2 ($C055), 0 = PAGE1 ($C054) */
int apple2_mem_is_hires_mode(void);  /* 1 = HIRES ($C057), 0 = LORES ($C056) */

/* Keyboard input latch ($C000 read) + strobe-clear ($C010 access).
 * apple2_mem_inject_key() is the test/host-side hook standing in for a
 * real physical keystroke -- sets the ASCII value and raises the strobe
 * (high bit) exactly as real Apple II keyboard hardware would. */
void apple2_mem_inject_key(uint8_t ascii_value);

/* Pushbutton/paddle button inputs ($C061-$C063 reads). Real Apple II
 * hardware reports each button's state in bit 7 (0x80) of the
 * corresponding address, bits 0-6 undefined/ignored here (we return 0
 * there, matching the "unimplemented reads as 0" convention used
 * elsewhere in this file):
 *   $C061 = PB0 (button 0, typically the Apple II's OpenApple/joystick
 *           button 0 -- also doubles as the Enter key equivalent on some
 *           software)
 *   $C062 = PB1 (button 1, ClosedApple/joystick button 1)
 *   $C063 = PB2 (button 2, joystick button 2 -- present on some clones
 *           and later machines, wired here for completeness)
 * apple2_mem_set_button_state() is the test/host-side hook standing in
 * for a real physical button press -- button_index is 0/1/2 matching
 * PB0/PB1/PB2, pressed is nonzero for "held down". */
void apple2_mem_set_button_state(int button_index, int pressed);

/* Getter counterpart to apple2_mem_set_button_state() -- reads the
 * current pressed/released state of PB0/PB1/PB2 without going through
 * the 6502 $C061-$C063 soft-switch read path. Needed for non-CPU callers
 * (e.g. boot_splash.c's real-hardware button poll adapter) that want
 * current button state as a plain C API rather than emulating a 6502
 * memory read. Returns 1 = pressed, 0 = released or button_index out of
 * range (not one of 0/1/2). */
int apple2_mem_get_button_state(int button_index);
void apple2_mem_clear_button_states(void);

/* Paddle analog timer inputs ($C064/$C065 PADDLE0/PADDLE1 reads,
 * $C070 PDRIVE trigger). Real Apple II hardware charges an RC circuit
 * proportional to the paddle's position when $C070 is accessed (any
 * access, read or write); PADDLE0/PADDLE1 report bit 7 (0x80) set while
 * that countdown is still running, clearing once it expires -- software
 * measures elapsed time between the $C070 trigger and the bit clearing
 * to derive the paddle's analog position.
 *
 * Simplification (documented, matches this file's precedent for other
 * un-timed simplifications like Disk II's stepper motor): since there is
 * no real-time clock to race against here, the "RC countdown" is modeled
 * as a simple counter that decrements by one on each PADDLEn read after
 * $C070 has armed it -- apple2_mem_set_paddle_value() sets how many
 * reads a countdown takes to expire (0-255, an analog stand-in for a
 * physical paddle position: 0 = wired straight through, higher = "turned
 * further", takes longer to discharge). */
void apple2_mem_set_paddle_value(int paddle_index, uint8_t value);

/* Annunciator outputs AN0-AN3 ($C058-$C05F). Each is an independent
 * on/off softswitch that any access (read or write) sets, matching this
 * file's other soft-switch semantics:
 *   $C058/$C059 = AN0 off/on   $C05A/$C05B = AN1 off/on
 *   $C05C/$C05D = AN2 off/on   $C05E/$C05F = AN3 off/on
 * annunciator_index is 0-3 matching AN0-AN3; returns 1 = on, 0 = off.
 * Defaults to off (0) after apple2_mem_reset(), matching real hardware. */
int apple2_mem_get_annunciator_state(int annunciator_index);
void apple2_mem_clear_annunciator_states(void);

#endif /* APPLE2_MEM_H */
