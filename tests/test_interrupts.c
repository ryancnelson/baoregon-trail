/*
 * RED test: real hardware interrupt entry points -- irq6502() and
 * nmi6502() -- distinct from the software BRK instruction (case 0x00 in
 * step6502()). Apple II peripherals (e.g. a VBL interrupt, or a future
 * timer-driven use of Bunnie's audio trap) need a way to interrupt the
 * running program asynchronously, between instructions, the same way
 * real 6502 hardware does via its IRQ and NMI pins -- BRK alone can't
 * model that since it's a normal fetched opcode, not an out-of-band
 * signal.
 *
 * Real 6502 semantics modeled here:
 *   - irq6502(): pushes PC (unmodified -- NOT pc+1 like BRK's padding
 *     byte), then status WITHOUT the B flag set (this is how software
 *     tells an IRQ-triggered stack frame apart from a BRK-triggered one
 *     when it's later popped via RTI and inspected), sets the
 *     interrupt-disable flag, and jumps through the IRQ/BRK vector at
 *     $FFFE/$FFFF (shared with BRK on real hardware). Masked (a no-op)
 *     if FLAG_INTERRUPT is already set -- matches real 6502 IRQ pin
 *     behavior.
 *   - nmi6502(): identical stack/vector mechanics to irq6502() except it
 *     uses the dedicated NMI vector at $FFFA/$FFFB and is NEVER masked by
 *     FLAG_INTERRUPT (that's the entire point of "non-maskable").
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
    test_ram[0xFFFD] = 0x04; /* reset vector -> $0400 */
    reset6502();
    a = 0;
    x = 0;
    y = 0;
    status = 0;
    clockticks6502 = 0;
}

static void test_irq_jumps_through_irq_brk_vector(void) {
    setup();
    test_ram[0xFFFE] = 0x00;
    test_ram[0xFFFF] = 0x09; /* IRQ/BRK vector -> $0900 */
    pc = 0x0500;

    irq6502();

    CHECK(pc == 0x0900, "test_irq_jumps_through_irq_brk_vector");
}

static void test_irq_pushes_unmodified_pc_not_pc_plus_1(void) {
    /* Unlike BRK (which pushes pc+1 to skip a padding/signature byte),
     * a real hardware IRQ pushes the exact PC of the next instruction
     * that would have executed -- no offset. */
    setup();
    test_ram[0xFFFE] = 0x00;
    test_ram[0xFFFF] = 0x09;
    pc = 0x0500;

    irq6502();

    uint8_t lo = test_ram[0x0100 + sp + 2];
    uint8_t hi = test_ram[0x0100 + sp + 3];
    uint16_t pushed_pc = (uint16_t)(lo | (hi << 8));
    CHECK(pushed_pc == 0x0500,
          "test_irq_pushes_unmodified_pc_not_pc_plus_1");
}

static void test_irq_pushes_status_without_break_flag(void) {
    /* This is how RTI-side software distinguishes an IRQ-triggered stack
     * frame from a BRK-triggered one after the fact. */
    setup();
    test_ram[0xFFFE] = 0x00;
    test_ram[0xFFFF] = 0x09;
    status = FLAG_CARRY | FLAG_ZERO;

    irq6502();

    uint8_t pushed_status = test_ram[0x0100 + sp + 1];
    CHECK((pushed_status & FLAG_BREAK) == 0,
          "test_irq_pushes_status_without_break_flag");
}

static void test_irq_sets_interrupt_disable_flag(void) {
    setup();
    test_ram[0xFFFE] = 0x00;
    test_ram[0xFFFF] = 0x09;
    status &= (uint8_t)~FLAG_INTERRUPT;

    irq6502();

    CHECK((status & FLAG_INTERRUPT) != 0,
          "test_irq_sets_interrupt_disable_flag");
}

static void test_irq_is_masked_when_interrupt_disable_already_set(void) {
    /* Matches real 6502 IRQ pin behavior: while FLAG_INTERRUPT is set,
     * IRQ requests are ignored entirely -- no stack push, no PC jump. */
    setup();
    test_ram[0xFFFE] = 0x00;
    test_ram[0xFFFF] = 0x09;
    status |= FLAG_INTERRUPT;
    pc = 0x0500;
    uint8_t sp_before = sp;

    irq6502();

    CHECK(pc == 0x0500 && sp == sp_before,
          "test_irq_is_masked_when_interrupt_disable_already_set");
}

static void test_irq_then_rti_returns_to_interrupted_pc(void) {
    setup();
    test_ram[0xFFFE] = 0x00;
    test_ram[0xFFFF] = 0x09;
    test_ram[0x0900] = 0x40; /* RTI */
    pc = 0x0500;
    status = FLAG_CARRY;

    irq6502();
    step6502(); /* RTI */

    CHECK(pc == 0x0500 && (status & FLAG_CARRY) != 0,
          "test_irq_then_rti_returns_to_interrupted_pc");
}

static void test_nmi_jumps_through_dedicated_nmi_vector(void) {
    setup();
    test_ram[0xFFFA] = 0x00;
    test_ram[0xFFFB] = 0x0A; /* NMI vector -> $0A00, distinct from IRQ/BRK */
    pc = 0x0500;

    nmi6502();

    CHECK(pc == 0x0A00, "test_nmi_jumps_through_dedicated_nmi_vector");
}

static void test_nmi_is_never_masked_by_interrupt_disable(void) {
    /* The defining property of "non-maskable": NMI fires even while
     * FLAG_INTERRUPT is set, unlike irq6502(). */
    setup();
    test_ram[0xFFFA] = 0x00;
    test_ram[0xFFFB] = 0x0A;
    status |= FLAG_INTERRUPT;
    pc = 0x0500;

    nmi6502();

    CHECK(pc == 0x0A00,
          "test_nmi_is_never_masked_by_interrupt_disable");
}

static void test_nmi_pushes_status_without_break_flag(void) {
    setup();
    test_ram[0xFFFA] = 0x00;
    test_ram[0xFFFB] = 0x0A;
    status = FLAG_ZERO;

    nmi6502();

    uint8_t pushed_status = test_ram[0x0100 + sp + 1];
    CHECK((pushed_status & FLAG_BREAK) == 0,
          "test_nmi_pushes_status_without_break_flag");
}

static void test_nmi_then_rti_returns_to_interrupted_pc(void) {
    /* Mirror of test_irq_then_rti_returns_to_interrupted_pc for NMI --
     * never verified before that NMI's stack frame round-trips cleanly
     * through RTI back to the exact interrupted PC/flags. */
    setup();
    test_ram[0xFFFA] = 0x00;
    test_ram[0xFFFB] = 0x0A;
    test_ram[0x0A00] = 0x40; /* RTI */
    pc = 0x0500;
    status = FLAG_OVERFLOW;

    nmi6502();
    step6502(); /* RTI */

    CHECK(pc == 0x0500 && (status & FLAG_OVERFLOW) != 0,
          "test_nmi_then_rti_returns_to_interrupted_pc");
}

static void test_nmi_fires_during_a_masked_irq_situation_unaffected(void) {
    /* Combined scenario: IRQ is masked (FLAG_INTERRUPT set, e.g. because
     * an earlier irq6502() call already ran), but NMI must still fire
     * and jump through its OWN vector, completely independent of IRQ's
     * masked state -- proves the two interrupt paths don't share any
     * masking logic that could accidentally cross-contaminate. */
    setup();
    test_ram[0xFFFE] = 0x00;
    test_ram[0xFFFF] = 0x09; /* IRQ/BRK vector -> $0900 */
    test_ram[0xFFFA] = 0x00;
    test_ram[0xFFFB] = 0x0A; /* NMI vector -> $0A00 */
    pc = 0x0500;

    irq6502(); /* sets FLAG_INTERRUPT, jumps to $0900 */
    CHECK(pc == 0x0900, "test_nmi_fires_during_a_masked_irq_situation_unaffected: IRQ landed first");

    pc = 0x0600; /* pretend some IRQ handler code ran and moved pc */
    nmi6502();

    CHECK(pc == 0x0A00,
          "test_nmi_fires_during_a_masked_irq_situation_unaffected: NMI still fires despite I-flag set");
}

static void test_irq_and_nmi_round_trip_survives_stack_wraparound(void) {
    /* Same intersection-of-features gap as
     * test_brk_rti_round_trip_survives_stack_wraparound in
     * test_stack_wraparound.c, but for the hardware entry points
     * (irq6502()/nmi6502()) rather than the BRK opcode -- these are
     * separate C functions with their own copy of the same 3-byte push
     * sequence, never previously exercised at the sp 0x00/0xFF
     * wraparound boundary. sp=0x02 means the 3-byte push crosses
     * through 0x01, 0x00, and wraps to 0xFF for BOTH irq6502() and
     * nmi6502() in sequence, confirming neither corrupts the other's
     * pushed frame nor mishandles the wraparound. */
    setup();
    test_ram[0xFFFE] = 0x00;
    test_ram[0xFFFF] = 0x09; /* IRQ/BRK vector -> $0900 */
    test_ram[0x0900] = 0x40; /* RTI, for the IRQ round trip */
    sp = 0x02;
    pc = 0x0500;
    status = FLAG_CARRY;

    irq6502(); /* pushes PC/status: sp 0x02->0x01->0x00->0xFF */
    uint8_t sp_after_irq = sp;
    step6502(); /* RTI: sp 0xFF->0x00->0x01->0x02 */

    CHECK(sp_after_irq == 0xFF && pc == 0x0500 && sp == 0x02 &&
          (status & FLAG_CARRY) != 0,
          "test_irq_and_nmi_round_trip_survives_stack_wraparound: IRQ leg");

    test_ram[0xFFFA] = 0x00;
    test_ram[0xFFFB] = 0x0A; /* NMI vector -> $0A00 */
    test_ram[0x0A00] = 0x40; /* RTI, for the NMI round trip */
    sp = 0x02;
    pc = 0x0600;
    status = FLAG_OVERFLOW;

    nmi6502(); /* pushes PC/status: sp 0x02->0x01->0x00->0xFF */
    uint8_t sp_after_nmi = sp;
    step6502(); /* RTI: sp 0xFF->0x00->0x01->0x02 */

    CHECK(sp_after_nmi == 0xFF && pc == 0x0600 && sp == 0x02 &&
          (status & FLAG_OVERFLOW) != 0,
          "test_irq_and_nmi_round_trip_survives_stack_wraparound: NMI leg");
}

static void test_irq_correctly_pushes_and_restores_pc_at_top_of_address_space(void) {
    /* Intersection of two previously-separately-tested classes: PC
     * wraparound at the $FFFF/$0000 boundary (test_pc_wraps_from_0xffff_
     * to_0x0000_on_single_byte_opcode in test_opcodes.c) and interrupt
     * push/RTI semantics -- never tested TOGETHER. If the interrupted PC
     * is exactly $FFFF (the boundary value itself, not yet wrapped)
     * when an IRQ fires, the pushed PC bytes (hi=0xFF, lo=0xFF) must
     * round-trip through RTI back to exactly $FFFF, not get corrupted
     * by any wraparound-adjacent arithmetic in the push/pull path. */
    setup();
    test_ram[0xFFFE] = 0x00;
    test_ram[0xFFFF] = 0x09; /* IRQ/BRK vector -> $0900 (overlaps the same
                                 vector bytes read here, but that's fine --
                                 the vector read happens AFTER the push,
                                 using the just-pushed pc's old value is
                                 what's under test, not what's stored at
                                 $FFFF as data) */
    pc = 0xFFFF; /* the interrupted PC is the boundary value itself */
    test_ram[0x0900] = 0x40; /* RTI */

    irq6502();
    CHECK(pc == 0x0900, "test_irq_correctly_pushes_and_restores_pc_at_top_of_address_space: jumped to vector");

    step6502(); /* RTI: pulls the pushed PC back */
    CHECK(pc == 0xFFFF,
          "test_irq_correctly_pushes_and_restores_pc_at_top_of_address_space: PC=0xFFFF round-tripped intact");
}

int main(void) {
    test_irq_jumps_through_irq_brk_vector();
    test_irq_pushes_unmodified_pc_not_pc_plus_1();
    test_irq_pushes_status_without_break_flag();
    test_irq_sets_interrupt_disable_flag();
    test_irq_is_masked_when_interrupt_disable_already_set();
    test_irq_then_rti_returns_to_interrupted_pc();
    test_nmi_jumps_through_dedicated_nmi_vector();
    test_nmi_is_never_masked_by_interrupt_disable();
    test_nmi_pushes_status_without_break_flag();
    test_nmi_then_rti_returns_to_interrupted_pc();
    test_nmi_fires_during_a_masked_irq_situation_unaffected();
    test_irq_and_nmi_round_trip_survives_stack_wraparound();
    test_irq_correctly_pushes_and_restores_pc_at_top_of_address_space();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
