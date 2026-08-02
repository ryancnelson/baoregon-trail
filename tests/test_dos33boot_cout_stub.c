/*
 * tests/test_dos33boot_cout_stub.c -- TDD test for the minimal real COUT
 * ($FDED) machine-code stub patched into main_qemu_dos33boot.c's system
 * ROM (fable-5's emu_trace finding, dd86358: DOS 3.3 genuinely boots and
 * lands on its intentional JMP $E000 spin loop, but the "DOS VERSION 3.3"
 * banner never appears because COUT was a bare RTS no-op).
 *
 * This test doesn't link main_qemu_dos33boot.c directly (it's a
 * standalone QEMU-only entry point with its own main()) -- instead it
 * reproduces the exact same 18-byte machine-code sequence and system-ROM
 * placement here, so a) the sequence itself is proven correct against
 * real cpu6502.c/apple2_mem.c execution semantics on host, independent
 * of QEMU, and b) any future accidental drift between this test and the
 * real file is something a reviewer diffing the two byte-for-byte can
 * catch immediately.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"

#define SYSTEM_ROM_SIZE 16384
static uint8_t g_rom[SYSTEM_ROM_SIZE];

/* Same 18-byte sequence as main_qemu_dos33boot.c's init_system_rom(),
 * placed at offset $3DED ($FDED - $C000). */
static const uint8_t COUT_CODE[] = {
    0x85, 0xFF,             /* STA $FF */
    0x98,                   /* TYA */
    0x48,                   /* PHA */
    0xA4, 0x24,             /* LDY $24 */
    0xA5, 0xFF,             /* LDA $FF */
    0x99, 0x00, 0x04,       /* STA $0400,Y */
    0xE6, 0x24,             /* INC $24 */
    0x68,                   /* PLA */
    0xA8,                   /* TAY */
    0xA5, 0xFF,             /* LDA $FF */
    0x60                    /* RTS */
};

static void install_cout_stub(void) {
    memset(g_rom, 0x60, SYSTEM_ROM_SIZE); /* every other byte: RTS (matches real stub-ROM convention) */
    for (size_t i = 0; i < sizeof(COUT_CODE); i++) {
        g_rom[0x3DED + i] = COUT_CODE[i];
    }
    apple2_mem_load_system_rom(g_rom);
}

/* Drives a JSR $FDED with a given accumulator value from a small test
 * program at $0300 (well clear of the ROM/screen-memory regions), then
 * runs just enough cycles for JSR + the COUT body + RTS + a trailing
 * BRK to stop cleanly. */
static void call_cout(uint8_t char_with_high_bit_set) {
    /* $0300: LDA #char ; JSR $FDED ; BRK */
    uint8_t program[] = {
        0xA9, char_with_high_bit_set, /* LDA #char */
        0x20, 0xED, 0xFD,             /* JSR $FDED */
        0x00                          /* BRK */
    };
    for (size_t i = 0; i < sizeof(program); i++) {
        write6502((uint16_t)(0x0300 + i), program[i]);
    }
    pc = 0x0300;
    /* Generous cycle budget -- LDA(2)+JSR(6)+COUT body(~20)+RTS(6)+BRK(7)
     * is well under 100 cycles; this just needs to be enough to run to
     * completion without looping forever on a hung/incorrect program. */
    exec6502(200);
}

static void test_single_char_writes_to_screen_column_0(void) {
    apple2_mem_reset();
    install_cout_stub();

    call_cout('D' | 0x80); /* 0xC4 -- normal-video 'D', real Apple II convention */

    /* Real Apple II text-page memory: high-bit-set byte written as-is,
     * NOT stripped -- text_apple2.c's own glyph decoder already expects
     * exactly this encoding (rom_index = screen_byte & 0x7F). */
    assert(read6502(0x0400) == ('D' | 0x80));
    printf("PASS: test_single_char_writes_to_screen_column_0\n");
}

static void test_cursor_column_advances_between_calls(void) {
    apple2_mem_reset();
    install_cout_stub();

    call_cout('D' | 0x80);
    call_cout('O' | 0x80);
    call_cout('S' | 0x80);

    /* Real known-sequence test: three characters written in order must
     * land at consecutive screen-memory columns, matching a real
     * cursor-column advance, not all overwriting column 0. */
    assert(read6502(0x0400) == ('D' | 0x80));
    assert(read6502(0x0401) == ('O' | 0x80));
    assert(read6502(0x0402) == ('S' | 0x80));
    printf("PASS: test_cursor_column_advances_between_calls\n");
}

static void test_known_banner_sequence_produces_known_screen_bytes(void) {
    /* The actual real-world validation target: "DOS VERSION 3.3" as DOS
     * 3.3 itself would print it via repeated COUT calls, character by
     * character, normal-video (high bit set). Deterministic known-input
     * -> known-output TDD, per the task's explicit ask. */
    apple2_mem_reset();
    install_cout_stub();

    const char *banner = "DOS VERSION 3.3";
    for (const char *p = banner; *p; p++) {
        call_cout((uint8_t)(*p) | 0x80);
    }

    for (size_t i = 0; banner[i]; i++) {
        uint8_t expected = (uint8_t)(banner[i]) | 0x80;
        uint8_t actual = read6502((uint16_t)(0x0400 + i));
        if (actual != expected) {
            fprintf(stderr, "FAIL: screen[%zu] = 0x%02X, expected 0x%02X ('%c')\n",
                    i, actual, expected, banner[i]);
            assert(0);
        }
    }
    printf("PASS: test_known_banner_sequence_produces_known_screen_bytes\n");
}

static void test_registers_preserved_across_cout_call(void) {
    /* Real COUT's contract: A holds the same character on return, and X/Y
     * must be unclobbered -- calling code (DOS's print routines) relies
     * on this exactly like every other Apple II monitor ROM entry point. */
    apple2_mem_reset();
    install_cout_stub();

    uint8_t program[] = {
        0xA9, 'X' | 0x80,   /* LDA #'X'|0x80 */
        0xA2, 0x42,         /* LDX #$42 */
        0xA0, 0x24,         /* LDY #$24 -- deliberately nonzero, distinguishable from $24's real value */
        0x20, 0xED, 0xFD,   /* JSR $FDED */
        0x00                /* BRK */
    };
    for (size_t i = 0; i < sizeof(program); i++) {
        write6502((uint16_t)(0x0300 + i), program[i]);
    }
    pc = 0x0300;
    exec6502(200);

    assert(a == ('X' | 0x80));
    assert(x == 0x42);
    assert(y == 0x24);
    printf("PASS: test_registers_preserved_across_cout_call\n");
}

static void test_f88e_entry_point_also_routes_to_real_cout(void) {
    /* Some real Apple II software calls $F88E (Applesoft's own
     * COUT-wrapper entry point) instead of the canonical $FDED
     * monitor-ROM entry directly -- main_qemu_dos33boot.c patches $F88E
     * to JMP $FDED so both converge on the same real implementation
     * rather than leaving $F88E as a second silent no-op. */
    apple2_mem_reset();
    install_cout_stub();
    g_rom[0x388E] = 0x4C; /* JMP $FDED */
    g_rom[0x388F] = 0xED;
    g_rom[0x3890] = 0xFD;

    uint8_t program[] = {
        0xA9, 'Z' | 0x80,   /* LDA #'Z'|0x80 */
        0x20, 0x8E, 0xF8,   /* JSR $F88E */
        0x00                /* BRK */
    };
    for (size_t i = 0; i < sizeof(program); i++) {
        write6502((uint16_t)(0x0300 + i), program[i]);
    }
    pc = 0x0300;
    exec6502(200);

    assert(read6502(0x0400) == ('Z' | 0x80));
    printf("PASS: test_f88e_entry_point_also_routes_to_real_cout\n");
}

int main(void) {
    test_single_char_writes_to_screen_column_0();
    test_cursor_column_advances_between_calls();
    test_known_banner_sequence_produces_known_screen_bytes();
    test_registers_preserved_across_cout_call();
    test_f88e_entry_point_also_routes_to_real_cout();
    printf("All tests passed.\n");
    return 0;
}
