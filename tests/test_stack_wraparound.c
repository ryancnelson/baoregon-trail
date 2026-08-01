/*
 * RED test: stack pointer wraparound. sp is a uint8_t with no bounds
 * checking on real 6502 hardware -- pushing when sp==0x00 must wrap to
 * 0xFF (not crash/clamp), and pulling when sp==0xFF must wrap to 0x00.
 * This matters because Apple II software occasionally (ab)uses the full
 * $0100-$01FF stack page and a wraparound bug here would silently corrupt
 * memory outside the stack page instead of staying within it.
 */
#include <stdio.h>
#include <string.h>
#include "../src/cpu6502.h"

static uint8_t test_ram[65536];
static int failures = 0;

#define CHECK(cond, label) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", label); \
            failures++; \
        } else { \
            printf("PASS: %s\n", label); \
        } \
    } while (0)

uint8_t read6502(uint16_t address) {
    return test_ram[address];
}

void write6502(uint16_t address, uint8_t value) {
    test_ram[address] = value;
}

static void setup(void) {
    memset(test_ram, 0, sizeof(test_ram));
    test_ram[0xFFFC] = 0x00;
    test_ram[0xFFFD] = 0x04;
    reset6502();
    a = 0;
    x = 0;
    y = 0;
    status = 0;
    clockticks6502 = 0;
}

static void test_push_wraps_from_0x00_to_0xff(void) {
    /* PHA when sp==0x00 must write to $0100 (sp+0x0100) then wrap sp to
     * 0xFF, NOT clamp at 0x00 or write outside the $0100-$01FF page. */
    setup();
    sp = 0x00;
    a = 0x42;
    test_ram[0x0400] = 0x48; /* PHA */

    step6502();

    CHECK(test_ram[0x0100] == 0x42 && sp == 0xFF,
          "test_push_wraps_from_0x00_to_0xff");
}

static void test_pull_wraps_from_0xff_to_0x00(void) {
    /* PLA when sp==0xFF must read from $0100 (after wrapping sp to
     * 0x00), the mirror image of the push wraparound above. */
    setup();
    sp = 0xFF;
    test_ram[0x0100] = 0x99;
    test_ram[0x0400] = 0x68; /* PLA */

    step6502();

    CHECK(a == 0x99 && sp == 0x00,
          "test_pull_wraps_from_0xff_to_0x00");
}

static void test_jsr_rts_round_trip_survives_stack_wraparound(void) {
    /* A JSR when sp is right at the wrap boundary (0x01, so pushing two
     * bytes crosses through 0x00 to 0xFF) must still round-trip cleanly
     * through a matching RTS -- proves the wraparound doesn't corrupt
     * the pushed return address itself. */
    setup();
    sp = 0x01;
    test_ram[0x0400] = 0x20; /* JSR $0500 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x05;
    test_ram[0x0500] = 0x60; /* RTS */

    step6502(); /* JSR */
    uint8_t sp_after_jsr = sp;
    step6502(); /* RTS */

    CHECK(pc == 0x0403 && sp == 0x01,
          "test_jsr_rts_round_trip_survives_stack_wraparound");
    (void)sp_after_jsr;
}

static void test_brk_rti_round_trip_survives_stack_wraparound(void) {
    /* BRK/interrupt handling pushes THREE bytes (PC hi, PC lo, status),
     * a different combination than JSR's two-byte push above -- an
     * intersection of two features hardened separately this session
     * (interrupt B-flag/vector semantics, and push8()/pull8()
     * wraparound) that was never tested TOGETHER. sp=0x02 means the
     * 3-byte BRK push crosses through 0x01, 0x00, and wraps to 0xFF --
     * confirms the full push/pull cycle round-trips cleanly through
     * that boundary via a matching RTI, not just push8()/pull8() in
     * isolation (test_push_wraps_from_0x00_to_0xff /
     * test_pull_wraps_from_0xff_to_0x00 above) or JSR/RTS's two-byte
     * case. */
    setup();
    sp = 0x02;
    status = FLAG_CARRY; /* arbitrary non-zero status to round-trip */
    test_ram[0xFFFE] = 0x00; /* IRQ/BRK vector -> $0500 */
    test_ram[0xFFFF] = 0x05;
    test_ram[0x0400] = 0x00; /* BRK */
    test_ram[0x0401] = 0xEA; /* padding byte BRK skips over */
    test_ram[0x0500] = 0x40; /* RTI */

    step6502(); /* BRK: pushes PC=0x0402, status -- sp 0x02->0x01->0x00->0xFF */
    uint8_t sp_after_brk = sp;
    step6502(); /* RTI: pulls status, PC -- sp 0xFF->0x00->0x01->0x02 */

    CHECK(sp_after_brk == 0xFF && pc == 0x0402 && sp == 0x02 &&
          (status & FLAG_CARRY) != 0,
          "test_brk_rti_round_trip_survives_stack_wraparound");
}

int main(void) {
    test_push_wraps_from_0x00_to_0xff();
    test_pull_wraps_from_0xff_to_0x00();
    test_jsr_rts_round_trip_survives_stack_wraparound();
    test_brk_rti_round_trip_survives_stack_wraparound();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
