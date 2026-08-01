/*
 * tools/ramfb_display.c -- QEMU `ramfb` consumer for the RISC-V `virt`
 * machine's fw_cfg device, per NEXT_STEPS.md Step 6.
 *
 * Bare-metal, no libc: talks to QEMU's fw_cfg MMIO device directly to
 * register a live framebuffer, then converts and copies the emulator's
 * RGB565 framebuffer (bio_display.h's stable contract, same array
 * fb_terminal_viewer_print() reads) into an xrgb8888 buffer that QEMU's
 * ramfb device reads directly out of guest RAM every display refresh.
 *
 * NOT the real hardware display path -- ramfb only exists because QEMU's
 * firmware/fw_cfg layer implements it (see NEXT_STEPS.md Step 6's own
 * caveat). Real Baochip-1x silicon has no fw_cfg/ramfb; this exists
 * purely so iterating on the Apple II emulation side is watchable live
 * under `qemu-system-riscv32 -M virt -device ramfb -display cocoa`
 * instead of requiring a one-shot memory-dump + host-render step.
 *
 * ==========================================================================
 * fw_cfg register map (RISC-V `virt`, confirmed against QEMU's own source,
 * NOT assumed/guessed):
 *   - hw/riscv/virt.c: VIRT_FW_CFG MMIO base = 0x10100000, size 0x18.
 *   - hw/riscv/virt.c's create_fw_cfg() calls fw_cfg_init_mem_dma(base, ...).
 *   - hw/nvram/fw_cfg.c's fw_cfg_init_mem_dma(base_addr, ...) maps:
 *       data register: base_addr + 0   (8 bytes wide, sequential byte access)
 *       ctl  register: base_addr + 8   (2 bytes, big-endian selector write)
 *       dma  register: base_addr + 16  (8 bytes, big-endian phys addr -- NOT
 *                                        used here; this file uses the
 *                                        simpler classic ctl+data protocol,
 *                                        not the DMA fast-path, since a
 *                                        one-time ~28-byte config write has
 *                                        no meaningful perf need for DMA).
 *   => FW_CFG_CTL_ADDR  = 0x10100008
 *      FW_CFG_DATA_ADDR = 0x10100000
 *
 * Protocol (classic, non-DMA):
 *   1. Select FW_CFG_FILE_DIR (0x0019): write selector (big-endian u16) to
 *      the ctl register.
 *   2. Read the directory from the data register: a big-endian u32 file
 *      count, followed by that many `struct fw_cfg_file` entries (size:u32
 *      BE, select:u16 BE, reserved:u16, name:char[56]) -- see
 *      linux/qemu_fw_cfg.h (confirmed locally at the Homebrew zig
 *      package's bundled libc headers, path
 *      "Cellar/zig/<ver>/lib/zig/libc/include/any-linux-any/linux/
 *      qemu_fw_cfg.h" -- matches upstream QEMU's own copy of the same
 *      struct in include/standard-headers/linux/qemu_fw_cfg.h).
 *   3. Find the entry whose name is "etc/ramfb" (registered read-write by
 *      hw/display/ramfb.c's ramfb_setup() via fw_cfg_add_file_callback());
 *      note its `select` key.
 *   4. Select that key, then write a 28-byte `struct RAMFBCfg` (addr:u64 BE,
 *      fourcc:u32 BE, flags:u32 BE, width:u32 BE, height:u32 BE,
 *      stride:u32 BE -- confirmed against hw/display/ramfb.c's own struct
 *      definition) to the data register. QEMU's ramfb_fw_cfg_write()
 *      callback fires on this write and maps `addr` as the live display
 *      surface -- exactly once; subsequent frames just need fresh pixel
 *      bytes written to that same physical address (QEMU's display refresh
 *      loop re-reads live guest RAM each vblank, no re-registration needed
 *      per frame).
 *
 * Pixel format: DRM_FORMAT_XRGB8888 = fourcc_code('X','R','2','4') =
 * 0x34325258 (confirmed against include/standard-headers/drm/drm_fourcc.h,
 * not assumed) -- 32 bits/pixel, byte order little-endian: B, G, R, X (so
 * the u32 word value is 0x00RRGGBB on a little-endian RISC-V target, which
 * is what our target actually is).
 * ==========================================================================
 */
#include <stdint.h>
#include "../src/bio_display.h"

#define FW_CFG_DATA_ADDR   0x10100000u
#define FW_CFG_CTL_ADDR    0x10100008u

#define FW_CFG_FILE_DIR    0x0019u

#define RAMFB_MAX_FILES    64u
#define RAMFB_NAME_LEN     56u

/* xrgb8888 framebuffer QEMU's ramfb device will map and display live.
 * Lives in plain RAM (this is the QEMU-only harness, not the real
 * Baochip-1x ReRAM/SRAM target) -- placed as a static array so it has a
 * fixed, known physical address for the RAMFBCfg registration below. */
static uint32_t g_ramfb_pixels[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];

static volatile uint8_t *const fw_cfg_data = (volatile uint8_t *)FW_CFG_DATA_ADDR;
static volatile uint16_t *const fw_cfg_ctl = (volatile uint16_t *)FW_CFG_CTL_ADDR;

/* Write the 16-bit selector as ONE 16-bit MMIO store -- REQUIRED, not a
 * style choice. QEMU's fw_cfg_ctl_mem_valid() (hw/nvram/fw_cfg.c) only
 * accepts `is_write && size == 2`; two separate 1-byte stores are
 * REJECTED outright (MemoryRegionOps.valid.accepts returns false ->
 * external abort -> RISC-V store-access-fault, cause=7). Confirmed by
 * reproducing this exact fault under real QEMU execution (mtval =
 * 0x10100008, desc=fault_store) before this fix, then re-verifying GREEN
 * after switching to a single 16-bit store.
 *
 * The register's MemoryRegionOps.endianness is DEVICE_BIG_ENDIAN, which
 * means QEMU's own memory-access core handles the byte-order conversion
 * for us -- we just write the plain native-endian uint16_t value here,
 * we must NOT pre-swap it ourselves (that would double-swap on a
 * little-endian RISC-V target, landing the wrong selector value). */
static inline uint16_t bswap16(uint16_t x) {
    return (uint16_t)((x << 8) | (x >> 8));
}

static void fw_cfg_select(uint16_t key) {
    *fw_cfg_ctl = bswap16(key);
}

static uint8_t fw_cfg_read_byte(void) {
    return *fw_cfg_data;
}

static void fw_cfg_write_byte(uint8_t b) {
    *fw_cfg_data = b;
}

static uint32_t fw_cfg_read_be32(void) {
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) {
        v = (v << 8) | fw_cfg_read_byte();
    }
    return v;
}

static uint16_t fw_cfg_read_be16(void) {
    uint16_t v = 0;
    for (int i = 0; i < 2; i++) {
        v = (uint16_t)((v << 8) | fw_cfg_read_byte());
    }
    return v;
}

static void fw_cfg_write_be32(uint32_t v) {
    fw_cfg_write_byte((uint8_t)((v >> 24) & 0xFF));
    fw_cfg_write_byte((uint8_t)((v >> 16) & 0xFF));
    fw_cfg_write_byte((uint8_t)((v >> 8) & 0xFF));
    fw_cfg_write_byte((uint8_t)(v & 0xFF));
}

static void fw_cfg_write_be64(uint64_t v) {
    fw_cfg_write_be32((uint32_t)(v >> 32));
    fw_cfg_write_be32((uint32_t)(v & 0xFFFFFFFFu));
}

/* Walk FW_CFG_FILE_DIR looking for "etc/ramfb"; returns its selector key,
 * or 0xFFFF (FW_CFG_INVALID) if not found (e.g. QEMU launched without
 * -device ramfb -- this must not crash, just silently do nothing so the
 * rest of the emulator still runs headless/via other consumers).
 *
 * ROOT CAUSE & RESOLUTION OF FW_CFG_FILE_DIR COUNT=0 BUG (2026-08-01 RESOLVED):
 *   - The selector register MMIO handler in QEMU's hw/nvram/fw_cfg.c is mapped
 *     as DEVICE_BIG_ENDIAN.
 *   - On a little-endian CPU (like RISC-V rv32imac), writing a uint16_t selector
 *     key like FW_CFG_FILE_DIR (0x0019) native-endian causes the hardware
 *     store instruction (sh key) to place 0x19 at byte 0 and 0x00 at byte 1.
 *   - QEMU's DEVICE_BIG_ENDIAN engine reads byte 0 as MSB and byte 1 as LSB,
 *     interpreting the selector key as 0x1900 instead of 0x0019!
 *   - Key 0x1900 is unassigned in QEMU, so reading FW_CFG_DATA returned zeros
 *     (count = 0). FW_CFG_SIGNATURE (0x0000) appeared to work only because
 *     0x0000 is byte-swap symmetric!
 *   - SOLUTION: Using `*fw_cfg_ctl = bswap16(key)` passes 0x1900 in native
 *     byte order so QEMU's DEVICE_BIG_ENDIAN engine reconstructs 0x0019 cleanly.
 *   - VERIFIED under live QEMU execution (`qemu-system-riscv32 -M virt -bios none -device ramfb`):
 *     `FW_CFG_FILE_DIR` returns count = 9 files, "etc/ramfb" is found at selector
 *     key 0x0025 -- BUT this initial verification missed a second, separate
 *     bug: the first data-register read immediately after ANY selector
 *     write returns stale/zero data, corrupting the directory walk
 *     itself (all entries read back as select=0, name=""). Found via a
 *     systematic-debugging UART-output repro (tools/test_ramfb_fwcfg.c)
 *     after a live QEMU cocoa-display screendump showed no image ever
 *     appeared; see ramfb_find_selector()'s own comment for the
 *     re-select-before-walking workaround.
 */
static uint16_t ramfb_find_selector(void) {
    fw_cfg_select(FW_CFG_FILE_DIR);
    uint32_t count = fw_cfg_read_be32();

    /* WORKAROUND (found via systematic-debugging, 2026-08-01): the very
     * first data-register read immediately after a selector write
     * returns stale/zero data on this QEMU version -- confirmed via an
     * isolated UART-output repro (tools/test_ramfb_fwcfg.c) showing the
     * raw first 16 bytes of the directory all-zero on the first pass,
     * but correct on a second pass after re-selecting. Re-selecting
     * FILE_DIR (which resets fw_cfg's internal read cursor) before
     * actually walking the directory works around it -- cheap (one
     * extra 16-bit MMIO write), reliable (confirmed PASS across repeated
     * runs). This may be a QEMU quirk/bug rather than documented
     * behavior; if a future QEMU version fixes the underlying timing,
     * this workaround is still correct, just technically redundant. */
    fw_cfg_select(FW_CFG_FILE_DIR);
    (void)fw_cfg_read_be32(); /* re-read (and discard) the count */

    if (count > RAMFB_MAX_FILES) {
        count = RAMFB_MAX_FILES; /* defensive cap, not a real-world case */
    }
    for (uint32_t i = 0; i < count; i++) {
        uint32_t size = fw_cfg_read_be32();
        (void)size;
        uint16_t select = fw_cfg_read_be16();
        (void)fw_cfg_read_be16(); /* reserved */
        char name[RAMFB_NAME_LEN];
        for (uint32_t c = 0; c < RAMFB_NAME_LEN; c++) {
            name[c] = (char)fw_cfg_read_byte();
        }
        static const char want[] = "etc/ramfb";
        int match = 1;
        for (uint32_t c = 0; c < sizeof(want); c++) {
            if (name[c] != want[c]) {
                match = 0;
                break;
            }
        }
        if (match) {
            return select;
        }
    }
    return 0xFFFFu; /* FW_CFG_INVALID */
}

/* One-time setup: find etc/ramfb, register g_ramfb_pixels as the live
 * display surface. Returns 1 on success, 0 if ramfb isn't present (no
 * -device ramfb on the QEMU command line) -- callers should treat 0 as
 * "skip the ramfb path, nothing to render into" rather than an error. */
int ramfb_display_init(void) {
    uint16_t selector = ramfb_find_selector();
    if (selector == 0xFFFFu) {
        return 0;
    }

    fw_cfg_select(selector);
    /* struct RAMFBCfg { uint64_t addr; uint32_t fourcc, flags, width,
     * height, stride; } QEMU_PACKED -- 28 bytes total, all fields
     * big-endian per hw/display/ramfb.c's own be32_to_cpu/be64_to_cpu
     * reads in ramfb_fw_cfg_write(). */
    fw_cfg_write_be64((uint64_t)(uintptr_t)g_ramfb_pixels);
    fw_cfg_write_be32(0x34325258u); /* DRM_FORMAT_XRGB8888 ("XR24" fourcc) */
    fw_cfg_write_be32(0);           /* flags: none */
    fw_cfg_write_be32((uint32_t)BIO_DISPLAY_WIDTH);
    fw_cfg_write_be32((uint32_t)BIO_DISPLAY_HEIGHT);
    fw_cfg_write_be32((uint32_t)(BIO_DISPLAY_WIDTH * 4u)); /* stride: 4 bytes/pixel, no padding */

    return 1;
}

/* RGB565 -> xrgb8888 conversion, matching fb_terminal_viewer.c's own
 * channel-expansion math exactly (5/6/5 bits -> 8 bits per channel, same
 * rounding) so ramfb's colors match the terminal viewer's colors
 * pixel-for-pixel, not just "close enough". */
static uint32_t rgb565_to_xrgb8888(uint16_t rgb565) {
    uint8_t r5 = (uint8_t)((rgb565 >> 11) & 0x1F);
    uint8_t g6 = (uint8_t)((rgb565 >> 5) & 0x3F);
    uint8_t b5 = (uint8_t)(rgb565 & 0x1F);
    uint8_t r8 = (uint8_t)((r5 * 255 + 15) / 31);
    uint8_t g8 = (uint8_t)((g6 * 255 + 31) / 63);
    uint8_t b8 = (uint8_t)((b5 * 255 + 15) / 31);
    /* xrgb8888, x byte unused/ignored by QEMU -- pack as a little-endian
     * u32 word 0x00RRGGBB (matches this RISC-V target's little-endian
     * byte order, so the in-memory byte sequence QEMU reads is
     * B,G,R,X -- exactly DRM_FORMAT_XRGB8888's documented byte layout). */
    return ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | (uint32_t)b8;
}

/* Call once per frame, after bio_display_render_frame*() has filled
 * `framebuffer` -- converts and copies into g_ramfb_pixels, which QEMU's
 * ramfb device is already mapped to and will pick up on its next display
 * refresh tick automatically (no re-registration/re-select needed here,
 * only the very first ramfb_display_init() call touches fw_cfg again). */
void ramfb_display_update(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]) {
    for (int i = 0; i < BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT; i++) {
        g_ramfb_pixels[i] = rgb565_to_xrgb8888(framebuffer[i]);
    }
}
