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

static void test_and_immediate_masks_accumulator(void) {
    setup();
    a = 0xF0;
    test_ram[0x0400] = 0x29; /* AND #$0F */
    test_ram[0x0401] = 0x0F;
    step6502();
    CHECK(a == 0x00 && (status & FLAG_ZERO) != 0 && pc == 0x0402 &&
          clockticks6502 == 2,
          "test_and_immediate_masks_accumulator");
}

static void test_ora_immediate_sets_bits(void) {
    setup();
    a = 0x0F;
    test_ram[0x0400] = 0x09; /* ORA #$F0 */
    test_ram[0x0401] = 0xF0;
    step6502();
    CHECK(a == 0xFF && (status & FLAG_SIGN) != 0 && pc == 0x0402 &&
          clockticks6502 == 2,
          "test_ora_immediate_sets_bits");
}

static void test_eor_immediate_toggles_bits(void) {
    setup();
    a = 0xFF;
    test_ram[0x0400] = 0x49; /* EOR #$0F */
    test_ram[0x0401] = 0x0F;
    step6502();
    CHECK(a == 0xF0 && (status & FLAG_SIGN) != 0 && pc == 0x0402 &&
          clockticks6502 == 2,
          "test_eor_immediate_toggles_bits");
}

static void test_sbc_immediate_subtracts_with_carry_set(void) {
    setup();
    status |= FLAG_CARRY; /* carry set = no borrow, per NMOS 6502 semantics */
    a = 0x10;
    test_ram[0x0400] = 0xE9; /* SBC #$05 */
    test_ram[0x0401] = 0x05;
    step6502();
    CHECK(a == 0x0B && (status & FLAG_CARRY) != 0 && pc == 0x0402 &&
          clockticks6502 == 2,
          "test_sbc_immediate_subtracts_with_carry_set");
}

static void test_sbc_immediate_clears_carry_on_borrow(void) {
    setup();
    status |= FLAG_CARRY;
    a = 0x05;
    test_ram[0x0400] = 0xE9; /* SBC #$10 */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(a == 0xF5 && (status & FLAG_CARRY) == 0,
          "test_sbc_immediate_clears_carry_on_borrow");
}

static void test_cmp_immediate_sets_carry_when_accumulator_greater_or_equal(void) {
    setup();
    a = 0x50;
    test_ram[0x0400] = 0xC9; /* CMP #$30 */
    test_ram[0x0401] = 0x30;
    step6502();
    CHECK((status & FLAG_CARRY) != 0 && (status & FLAG_ZERO) == 0 &&
          pc == 0x0402 && clockticks6502 == 2,
          "test_cmp_immediate_sets_carry_when_accumulator_greater_or_equal");
}

static void test_cmp_immediate_sets_zero_when_equal(void) {
    setup();
    a = 0x30;
    test_ram[0x0400] = 0xC9; /* CMP #$30 */
    test_ram[0x0401] = 0x30;
    step6502();
    CHECK((status & FLAG_ZERO) != 0 && (status & FLAG_CARRY) != 0,
          "test_cmp_immediate_sets_zero_when_equal");
}

static void test_cmp_immediate_clears_carry_when_accumulator_less(void) {
    setup();
    a = 0x10;
    test_ram[0x0400] = 0xC9; /* CMP #$30 */
    test_ram[0x0401] = 0x30;
    step6502();
    CHECK((status & FLAG_CARRY) == 0 && (status & FLAG_SIGN) != 0,
          "test_cmp_immediate_clears_carry_when_accumulator_less");
}

static void test_cpx_immediate_compares_x_register(void) {
    setup();
    x = 0x40;
    test_ram[0x0400] = 0xE0; /* CPX #$40 */
    test_ram[0x0401] = 0x40;
    step6502();
    CHECK((status & FLAG_ZERO) != 0 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0402 && clockticks6502 == 2,
          "test_cpx_immediate_compares_x_register");
}

static void test_cpy_immediate_compares_y_register(void) {
    setup();
    y = 0x20;
    test_ram[0x0400] = 0xC0; /* CPY #$40 */
    test_ram[0x0401] = 0x40;
    step6502();
    CHECK((status & FLAG_CARRY) == 0 && pc == 0x0402 && clockticks6502 == 2,
          "test_cpy_immediate_compares_y_register");
}

static void test_asl_accumulator_shifts_left_and_sets_carry(void) {
    setup();
    a = 0x81; /* 1000_0001 */
    test_ram[0x0400] = 0x0A; /* ASL A */
    step6502();
    CHECK(a == 0x02 && (status & FLAG_CARRY) != 0 && pc == 0x0401 &&
          clockticks6502 == 2,
          "test_asl_accumulator_shifts_left_and_sets_carry");
}

static void test_lsr_accumulator_shifts_right_and_sets_carry(void) {
    setup();
    a = 0x03; /* 0000_0011 */
    test_ram[0x0400] = 0x4A; /* LSR A */
    step6502();
    CHECK(a == 0x01 && (status & FLAG_CARRY) != 0 &&
          (status & FLAG_SIGN) == 0 && pc == 0x0401 && clockticks6502 == 2,
          "test_lsr_accumulator_shifts_right_and_sets_carry");
}

static void test_rol_accumulator_rotates_left_through_carry(void) {
    setup();
    status |= FLAG_CARRY;
    a = 0x40; /* 0100_0000, carry-in=1 -> 1000_0001, carry-out=0 */
    test_ram[0x0400] = 0x2A; /* ROL A */
    step6502();
    CHECK(a == 0x81 && (status & FLAG_CARRY) == 0,
          "test_rol_accumulator_rotates_left_through_carry");
}

static void test_ror_accumulator_rotates_right_through_carry(void) {
    setup();
    status |= FLAG_CARRY;
    a = 0x02; /* 0000_0010, carry-in=1 -> 1000_0001, carry-out=0 */
    test_ram[0x0400] = 0x6A; /* ROR A */
    step6502();
    CHECK(a == 0x81 && (status & FLAG_CARRY) == 0,
          "test_ror_accumulator_rotates_right_through_carry");
}

static void test_pha_pushes_accumulator_to_stack(void) {
    setup();
    a = 0x77;
    test_ram[0x0400] = 0x48; /* PHA */
    step6502();
    CHECK(test_ram[0x0100 + sp + 1] == 0x77 && pc == 0x0401 &&
          clockticks6502 == 3,
          "test_pha_pushes_accumulator_to_stack");
}

static void test_pla_pulls_accumulator_from_stack(void) {
    setup();
    test_ram[0x0400] = 0x48; /* PHA */
    a = 0x66;
    step6502();
    a = 0x00;
    test_ram[0x0401] = 0x68; /* PLA */
    step6502();
    CHECK(a == 0x66 && pc == 0x0402 && clockticks6502 == 7,
          "test_pla_pulls_accumulator_from_stack");
}

static void test_jsr_pushes_return_address_and_jumps(void) {
    setup();
    test_ram[0x0400] = 0x20; /* JSR $1234 */
    test_ram[0x0401] = 0x34;
    test_ram[0x0402] = 0x12;
    step6502();
    CHECK(pc == 0x1234 && clockticks6502 == 6,
          "test_jsr_pushes_return_address_and_jumps");
}

static void test_rts_returns_to_address_after_jsr(void) {
    setup();
    test_ram[0x0400] = 0x20; /* JSR $0500 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x05;
    test_ram[0x0500] = 0x60; /* RTS */
    step6502(); /* JSR */
    step6502(); /* RTS */
    CHECK(pc == 0x0403 && clockticks6502 == 12,
          "test_rts_returns_to_address_after_jsr");
}

static void test_bcc_branches_when_carry_clear(void) {
    setup();
    status &= (uint8_t)~FLAG_CARRY;
    test_ram[0x0400] = 0x90; /* BCC +4 */
    test_ram[0x0401] = 0x04;
    step6502();
    CHECK(pc == 0x0406 && clockticks6502 == 3,
          "test_bcc_branches_when_carry_clear");
}

static void test_bcc_does_not_branch_when_carry_set(void) {
    setup();
    status |= FLAG_CARRY;
    test_ram[0x0400] = 0x90; /* BCC +4 */
    test_ram[0x0401] = 0x04;
    step6502();
    CHECK(pc == 0x0402 && clockticks6502 == 2,
          "test_bcc_does_not_branch_when_carry_set");
}

static void test_bcs_branches_when_carry_set(void) {
    setup();
    status |= FLAG_CARRY;
    test_ram[0x0400] = 0xB0; /* BCS +6 */
    test_ram[0x0401] = 0x06;
    step6502();
    CHECK(pc == 0x0408 && clockticks6502 == 3,
          "test_bcs_branches_when_carry_set");
}

static void test_bpl_branches_when_sign_flag_clear(void) {
    setup();
    status &= (uint8_t)~FLAG_SIGN;
    test_ram[0x0400] = 0x10; /* BPL +2 */
    test_ram[0x0401] = 0x02;
    step6502();
    CHECK(pc == 0x0404 && clockticks6502 == 3,
          "test_bpl_branches_when_sign_flag_clear");
}

static void test_bmi_branches_when_sign_flag_set(void) {
    setup();
    status |= FLAG_SIGN;
    test_ram[0x0400] = 0x30; /* BMI +2 */
    test_ram[0x0401] = 0x02;
    step6502();
    CHECK(pc == 0x0404 && clockticks6502 == 3,
          "test_bmi_branches_when_sign_flag_set");
}

static void test_bit_zeropage_sets_zero_when_no_overlap(void) {
    setup();
    a = 0x0F;
    test_ram[0x0010] = 0xF0;
    test_ram[0x0400] = 0x24; /* BIT $10 */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK((status & FLAG_ZERO) != 0 && pc == 0x0402 && clockticks6502 == 3,
          "test_bit_zeropage_sets_zero_when_no_overlap");
}

static void test_bit_zeropage_copies_bits_6_and_7_to_flags(void) {
    setup();
    a = 0xFF;
    test_ram[0x0010] = 0xC0; /* bits 7 and 6 set */
    test_ram[0x0400] = 0x24; /* BIT $10 */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK((status & FLAG_SIGN) != 0 && (status & FLAG_OVERFLOW) != 0 &&
          (status & FLAG_ZERO) == 0,
          "test_bit_zeropage_copies_bits_6_and_7_to_flags");
}

static void test_inc_zeropage_increments_memory(void) {
    setup();
    test_ram[0x0020] = 0x7F;
    test_ram[0x0400] = 0xE6; /* INC $20 */
    test_ram[0x0401] = 0x20;
    step6502();
    CHECK(test_ram[0x0020] == 0x80 && (status & FLAG_SIGN) != 0 &&
          pc == 0x0402 && clockticks6502 == 5,
          "test_inc_zeropage_increments_memory");
}

static void test_dec_zeropage_decrements_memory(void) {
    setup();
    test_ram[0x0020] = 0x01;
    test_ram[0x0400] = 0xC6; /* DEC $20 */
    test_ram[0x0401] = 0x20;
    step6502();
    CHECK(test_ram[0x0020] == 0x00 && (status & FLAG_ZERO) != 0 &&
          pc == 0x0402 && clockticks6502 == 5,
          "test_dec_zeropage_decrements_memory");
}

static void test_cli_clears_interrupt_disable_flag(void) {
    setup();
    status |= FLAG_INTERRUPT;
    test_ram[0x0400] = 0x58; /* CLI */
    step6502();
    CHECK((status & FLAG_INTERRUPT) == 0 && pc == 0x0401 &&
          clockticks6502 == 2,
          "test_cli_clears_interrupt_disable_flag");
}

static void test_sei_sets_interrupt_disable_flag(void) {
    setup();
    status &= (uint8_t)~FLAG_INTERRUPT;
    test_ram[0x0400] = 0x78; /* SEI */
    step6502();
    CHECK((status & FLAG_INTERRUPT) != 0 && pc == 0x0401 &&
          clockticks6502 == 2,
          "test_sei_sets_interrupt_disable_flag");
}

static void test_clv_clears_overflow_flag(void) {
    setup();
    status |= FLAG_OVERFLOW;
    test_ram[0x0400] = 0xB8; /* CLV */
    step6502();
    CHECK((status & FLAG_OVERFLOW) == 0 && pc == 0x0401 &&
          clockticks6502 == 2,
          "test_clv_clears_overflow_flag");
}

static void test_lda_indexed_indirect_loads_via_zp_x_pointer(void) {
    /* (zp,X): pointer = zeropage[(operand + X) & 0xFF .. +1], little-endian */
    setup();
    x = 0x04;
    test_ram[0x0400] = 0xA1; /* LDA ($20,X) */
    test_ram[0x0401] = 0x20;
    test_ram[0x0024] = 0x00; /* pointer lo, at $20+X=$24 */
    test_ram[0x0025] = 0x05; /* pointer hi -> effective addr $0500 */
    test_ram[0x0500] = 0x9A;
    step6502();
    CHECK(a == 0x9A && pc == 0x0402 && clockticks6502 == 6,
          "test_lda_indexed_indirect_loads_via_zp_x_pointer");
}

static void test_lda_indexed_indirect_wraps_zeropage_pointer_address(void) {
    /* (operand + X) must wrap within zero page (stay 8-bit), not carry into
     * page 1. */
    setup();
    x = 0xFF;
    test_ram[0x0400] = 0xA1; /* LDA ($02,X) -> ($02+$FF)&$FF = $01 */
    test_ram[0x0401] = 0x02;
    test_ram[0x0001] = 0x34;
    test_ram[0x0002] = 0x12; /* effective addr $1234 */
    test_ram[0x1234] = 0x77;
    step6502();
    CHECK(a == 0x77 && pc == 0x0402,
          "test_lda_indexed_indirect_wraps_zeropage_pointer_address");
}

static void test_lda_indirect_indexed_loads_via_zp_pointer_plus_y(void) {
    /* (zp),Y: pointer = zeropage[operand..+1], effective addr = pointer + Y */
    setup();
    y = 0x10;
    test_ram[0x0400] = 0xB1; /* LDA ($30),Y */
    test_ram[0x0401] = 0x30;
    test_ram[0x0030] = 0x00; /* pointer lo */
    test_ram[0x0031] = 0x06; /* pointer hi -> base $0600 */
    test_ram[0x0610] = 0x55; /* base + Y = $0610 */
    step6502();
    CHECK(a == 0x55 && pc == 0x0402 && clockticks6502 == 5,
          "test_lda_indirect_indexed_loads_via_zp_pointer_plus_y");
}

static void test_lda_indirect_indexed_extra_cycle_when_page_crossed(void) {
    /* Page-crossing on the base+Y add costs one extra cycle (6 vs 5), a
     * real NMOS 6502 timing quirk relevant to Bunnie's audio-sync work. */
    setup();
    y = 0x01;
    test_ram[0x0400] = 0xB1; /* LDA ($30),Y */
    test_ram[0x0401] = 0x30;
    test_ram[0x0030] = 0xFF; /* pointer lo */
    test_ram[0x0031] = 0x06; /* pointer hi -> base $06FF, +Y=1 -> $0700 */
    test_ram[0x0700] = 0x88;
    step6502();
    CHECK(a == 0x88 && pc == 0x0402 && clockticks6502 == 6,
          "test_lda_indirect_indexed_extra_cycle_when_page_crossed");
}

static void test_sta_indexed_indirect_stores_via_zp_x_pointer(void) {
    setup();
    a = 0x42;
    x = 0x04;
    test_ram[0x0400] = 0x81; /* STA ($20,X) */
    test_ram[0x0401] = 0x20;
    test_ram[0x0024] = 0x00;
    test_ram[0x0025] = 0x05; /* effective addr $0500 */
    step6502();
    CHECK(test_ram[0x0500] == 0x42 && pc == 0x0402 && clockticks6502 == 6,
          "test_sta_indexed_indirect_stores_via_zp_x_pointer");
}

static void test_sta_indirect_indexed_stores_via_zp_pointer_plus_y(void) {
    setup();
    a = 0x99;
    y = 0x10;
    test_ram[0x0400] = 0x91; /* STA ($30),Y */
    test_ram[0x0401] = 0x30;
    test_ram[0x0030] = 0x00;
    test_ram[0x0031] = 0x06; /* base $0600, +Y=$0610 */
    step6502();
    /* STA (zp),Y is always 6 cycles regardless of page crossing (no early-
     * termination optimization on a write, per NMOS 6502 reference). */
    CHECK(test_ram[0x0610] == 0x99 && pc == 0x0402 && clockticks6502 == 6,
          "test_sta_indirect_indexed_stores_via_zp_pointer_plus_y");
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
    test_and_immediate_masks_accumulator();
    test_ora_immediate_sets_bits();
    test_eor_immediate_toggles_bits();
    test_sbc_immediate_subtracts_with_carry_set();
    test_sbc_immediate_clears_carry_on_borrow();
    test_cmp_immediate_sets_carry_when_accumulator_greater_or_equal();
    test_cmp_immediate_sets_zero_when_equal();
    test_cmp_immediate_clears_carry_when_accumulator_less();
    test_cpx_immediate_compares_x_register();
    test_cpy_immediate_compares_y_register();
    test_asl_accumulator_shifts_left_and_sets_carry();
    test_lsr_accumulator_shifts_right_and_sets_carry();
    test_rol_accumulator_rotates_left_through_carry();
    test_ror_accumulator_rotates_right_through_carry();
    test_pha_pushes_accumulator_to_stack();
    test_pla_pulls_accumulator_from_stack();
    test_jsr_pushes_return_address_and_jumps();
    test_rts_returns_to_address_after_jsr();
    test_bcc_branches_when_carry_clear();
    test_bcc_does_not_branch_when_carry_set();
    test_bcs_branches_when_carry_set();
    test_bpl_branches_when_sign_flag_clear();
    test_bmi_branches_when_sign_flag_set();
    test_bit_zeropage_sets_zero_when_no_overlap();
    test_bit_zeropage_copies_bits_6_and_7_to_flags();
    test_inc_zeropage_increments_memory();
    test_dec_zeropage_decrements_memory();
    test_cli_clears_interrupt_disable_flag();
    test_sei_sets_interrupt_disable_flag();
    test_clv_clears_overflow_flag();
    test_lda_indexed_indirect_loads_via_zp_x_pointer();
    test_lda_indexed_indirect_wraps_zeropage_pointer_address();
    test_lda_indirect_indexed_loads_via_zp_pointer_plus_y();
    test_lda_indirect_indexed_extra_cycle_when_page_crossed();
    test_sta_indexed_indirect_stores_via_zp_x_pointer();
    test_sta_indirect_indexed_stores_via_zp_pointer_plus_y();

    if (failures > 0) {
        printf("\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("\nAll tests passed\n");
    return 0;
}
