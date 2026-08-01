/*
 * Growing opcode test suite. Each test_* function is one vertical tracer
 * bullet: one opcode behavior, added only after being watched RED then
 * implemented minimally to GREEN. Do not add cases to cpu6502.c without a
 * failing test here first (see test-driven-development skill).
 */
#include <stdio.h>
#include <string.h>
#include "../src/cpu6502.h"

static uint8_t test_ram[65536];
static int failures = 0;

uint8_t read6502(uint16_t address) {
    return test_ram[address];
}

void write6502(uint16_t address, uint8_t value) {
    test_ram[address] = value;
}

/* Resets machine state and points PC at $0400 (standard Klaus Dormann
 * functional-test entry point convention). */
static void setup(void) {
    memset(test_ram, 0, sizeof(test_ram));
    test_ram[0xFFFC] = 0x00;
    test_ram[0xFFFD] = 0x04;
    reset6502();
    a = 0;
    x = 0;
    y = 0;
    status = FLAG_CONSTANT;
    clockticks6502 = 0;
}

#define CHECK(cond, name) do { \
    if (cond) { \
        printf("PASS: %s\n", name); \
    } else { \
        printf("FAIL: %s\n", name); \
        failures++; \
    } \
} while (0)

static void test_nop_advances_pc_and_takes_2_cycles(void) {
    setup();
    test_ram[0x0400] = 0xEA; /* NOP */
    step6502();
    CHECK(pc == 0x0401 && clockticks6502 == 2,
          "test_nop_advances_pc_and_takes_2_cycles");
}

static void test_lda_immediate_loads_value(void) {
    setup();
    test_ram[0x0400] = 0xA9; /* LDA #$42 */
    test_ram[0x0401] = 0x42;
    step6502();
    CHECK(a == 0x42 && pc == 0x0402 && clockticks6502 == 2,
          "test_lda_immediate_loads_value");
}

static void test_lda_immediate_sets_zero_flag_on_zero(void) {
    setup();
    test_ram[0x0400] = 0xA9; /* LDA #$00 */
    test_ram[0x0401] = 0x00;
    step6502();
    CHECK((status & FLAG_ZERO) != 0 && (status & FLAG_SIGN) == 0,
          "test_lda_immediate_sets_zero_flag_on_zero");
}

static void test_lda_immediate_sets_negative_flag_on_high_bit(void) {
    setup();
    test_ram[0x0400] = 0xA9; /* LDA #$80 */
    test_ram[0x0401] = 0x80;
    step6502();
    CHECK((status & FLAG_SIGN) != 0 && (status & FLAG_ZERO) == 0,
          "test_lda_immediate_sets_negative_flag_on_high_bit");
}

static void test_lda_zeropage_loads_value_from_memory(void) {
    setup();
    test_ram[0x0400] = 0xA5; /* LDA $10 */
    test_ram[0x0401] = 0x10;
    test_ram[0x0010] = 0x37;
    step6502();
    CHECK(a == 0x37 && pc == 0x0402 && clockticks6502 == 3,
          "test_lda_zeropage_loads_value_from_memory");
}

static void test_ldx_immediate_loads_value(void) {
    setup();
    test_ram[0x0400] = 0xA2; /* LDX #$07 */
    test_ram[0x0401] = 0x07;
    step6502();
    CHECK(x == 0x07 && pc == 0x0402 && clockticks6502 == 2,
          "test_ldx_immediate_loads_value");
}

static void test_ldy_immediate_loads_value(void) {
    setup();
    test_ram[0x0400] = 0xA0; /* LDY #$09 */
    test_ram[0x0401] = 0x09;
    step6502();
    CHECK(y == 0x09 && pc == 0x0402 && clockticks6502 == 2,
          "test_ldy_immediate_loads_value");
}

static void test_sta_zeropage_stores_accumulator(void) {
    setup();
    a = 0x55;
    test_ram[0x0400] = 0x85; /* STA $20 */
    test_ram[0x0401] = 0x20;
    step6502();
    CHECK(test_ram[0x0020] == 0x55 && pc == 0x0402 && clockticks6502 == 3,
          "test_sta_zeropage_stores_accumulator");
}

static void test_tax_copies_accumulator_to_x(void) {
    setup();
    a = 0x99;
    x = 0x00;
    test_ram[0x0400] = 0xAA; /* TAX */
    step6502();
    CHECK(x == 0x99 && pc == 0x0401 && clockticks6502 == 2 &&
          (status & FLAG_SIGN) != 0,
          "test_tax_copies_accumulator_to_x");
}

static void test_tay_copies_accumulator_to_y(void) {
    setup();
    a = 0x00;
    y = 0xFF;
    test_ram[0x0400] = 0xA8; /* TAY */
    step6502();
    CHECK(y == 0x00 && pc == 0x0401 && clockticks6502 == 2 &&
          (status & FLAG_ZERO) != 0,
          "test_tay_copies_accumulator_to_y");
}

static void test_txa_copies_x_to_accumulator(void) {
    setup();
    x = 0x21;
    a = 0x00;
    test_ram[0x0400] = 0x8A; /* TXA */
    step6502();
    CHECK(a == 0x21 && pc == 0x0401 && clockticks6502 == 2,
          "test_txa_copies_x_to_accumulator");
}

static void test_tya_copies_y_to_accumulator(void) {
    setup();
    y = 0x21;
    a = 0x00;
    test_ram[0x0400] = 0x98; /* TYA */
    step6502();
    CHECK(a == 0x21 && pc == 0x0401 && clockticks6502 == 2,
          "test_tya_copies_y_to_accumulator");
}

static void test_inx_increments_x_with_wraparound(void) {
    setup();
    x = 0xFF;
    test_ram[0x0400] = 0xE8; /* INX */
    step6502();
    CHECK(x == 0x00 && pc == 0x0401 && clockticks6502 == 2 &&
          (status & FLAG_ZERO) != 0,
          "test_inx_increments_x_with_wraparound");
}

static void test_iny_increments_y(void) {
    setup();
    y = 0x10;
    test_ram[0x0400] = 0xC8; /* INY */
    step6502();
    CHECK(y == 0x11 && pc == 0x0401 && clockticks6502 == 2,
          "test_iny_increments_y");
}

static void test_dex_decrements_x_with_wraparound(void) {
    setup();
    x = 0x00;
    test_ram[0x0400] = 0xCA; /* DEX */
    step6502();
    CHECK(x == 0xFF && pc == 0x0401 && clockticks6502 == 2 &&
          (status & FLAG_SIGN) != 0,
          "test_dex_decrements_x_with_wraparound");
}

static void test_dey_decrements_y(void) {
    setup();
    y = 0x05;
    test_ram[0x0400] = 0x88; /* DEY */
    step6502();
    CHECK(y == 0x04 && pc == 0x0401 && clockticks6502 == 2,
          "test_dey_decrements_y");
}

static void test_clc_clears_carry_flag(void) {
    setup();
    status |= FLAG_CARRY;
    test_ram[0x0400] = 0x18; /* CLC */
    step6502();
    CHECK((status & FLAG_CARRY) == 0 && pc == 0x0401 && clockticks6502 == 2,
          "test_clc_clears_carry_flag");
}

static void test_sec_sets_carry_flag(void) {
    setup();
    status &= (uint8_t)~FLAG_CARRY;
    test_ram[0x0400] = 0x38; /* SEC */
    step6502();
    CHECK((status & FLAG_CARRY) != 0 && pc == 0x0401 && clockticks6502 == 2,
          "test_sec_sets_carry_flag");
}

static void test_adc_immediate_adds_without_carry_in(void) {
    setup();
    status &= (uint8_t)~FLAG_CARRY;
    a = 0x10;
    test_ram[0x0400] = 0x69; /* ADC #$05 */
    test_ram[0x0401] = 0x05;
    step6502();
    CHECK(a == 0x15 && pc == 0x0402 && clockticks6502 == 2 &&
          (status & FLAG_CARRY) == 0,
          "test_adc_immediate_adds_without_carry_in");
}

static void test_adc_immediate_sets_carry_on_overflow(void) {
    setup();
    status &= (uint8_t)~FLAG_CARRY;
    a = 0xFF;
    test_ram[0x0400] = 0x69; /* ADC #$02 */
    test_ram[0x0401] = 0x02;
    step6502();
    CHECK(a == 0x01 && (status & FLAG_CARRY) != 0,
          "test_adc_immediate_sets_carry_on_overflow");
}

static void test_adc_immediate_sets_overflow_flag_on_signed_overflow(void) {
    setup();
    status &= (uint8_t)~FLAG_CARRY;
    a = 0x7F; /* +127 */
    test_ram[0x0400] = 0x69; /* ADC #$01 */
    test_ram[0x0401] = 0x01;
    step6502();
    CHECK(a == 0x80 && (status & FLAG_OVERFLOW) != 0 && (status & FLAG_SIGN) != 0,
          "test_adc_immediate_sets_overflow_flag_on_signed_overflow");
}

static void test_jmp_absolute_sets_pc(void) {
    setup();
    test_ram[0x0400] = 0x4C; /* JMP $1234 */
    test_ram[0x0401] = 0x34;
    test_ram[0x0402] = 0x12;
    step6502();
    CHECK(pc == 0x1234 && clockticks6502 == 3,
          "test_jmp_absolute_sets_pc");
}

static void test_beq_branches_when_zero_flag_set(void) {
    setup();
    status |= FLAG_ZERO;
    test_ram[0x0400] = 0xF0; /* BEQ +5 */
    test_ram[0x0401] = 0x05;
    step6502();
    CHECK(pc == 0x0407 && clockticks6502 == 3,
          "test_beq_branches_when_zero_flag_set");
}

static void test_beq_does_not_branch_when_zero_flag_clear(void) {
    setup();
    status &= (uint8_t)~FLAG_ZERO;
    test_ram[0x0400] = 0xF0; /* BEQ +5 */
    test_ram[0x0401] = 0x05;
    step6502();
    CHECK(pc == 0x0402 && clockticks6502 == 2,
          "test_beq_does_not_branch_when_zero_flag_clear");
}

static void test_bne_branches_when_zero_flag_clear(void) {
    setup();
    status &= (uint8_t)~FLAG_ZERO;
    test_ram[0x0400] = 0xD0; /* BNE +3 */
    test_ram[0x0401] = 0x03;
    step6502();
    CHECK(pc == 0x0405 && clockticks6502 == 3,
          "test_bne_branches_when_zero_flag_clear");
}

int main(void) {
    test_nop_advances_pc_and_takes_2_cycles();
    test_lda_immediate_loads_value();
    test_lda_immediate_sets_zero_flag_on_zero();
    test_lda_immediate_sets_negative_flag_on_high_bit();
    test_lda_zeropage_loads_value_from_memory();
    test_ldx_immediate_loads_value();
    test_ldy_immediate_loads_value();
    test_sta_zeropage_stores_accumulator();
    test_tax_copies_accumulator_to_x();
    test_tay_copies_accumulator_to_y();
    test_txa_copies_x_to_accumulator();
    test_tya_copies_y_to_accumulator();
    test_inx_increments_x_with_wraparound();
    test_iny_increments_y();
    test_dex_decrements_x_with_wraparound();
    test_dey_decrements_y();
    test_clc_clears_carry_flag();
    test_sec_sets_carry_flag();
    test_adc_immediate_adds_without_carry_in();
    test_adc_immediate_sets_carry_on_overflow();
    test_adc_immediate_sets_overflow_flag_on_signed_overflow();
    test_jmp_absolute_sets_pc();
    test_beq_branches_when_zero_flag_set();
    test_beq_does_not_branch_when_zero_flag_clear();
    test_bne_branches_when_zero_flag_clear();

    if (failures > 0) {
        printf("\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("\nAll tests passed\n");
    return 0;
}
