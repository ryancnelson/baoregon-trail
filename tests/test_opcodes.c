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

static void test_lda_zeropage_x_loads_value_with_index(void) {
    setup();
    x = 0x05;
    test_ram[0x0400] = 0xB5; /* LDA $10,X */
    test_ram[0x0401] = 0x10;
    test_ram[0x0015] = 0x64; /* $10 + X = $15 */
    step6502();
    CHECK(a == 0x64 && pc == 0x0402 && clockticks6502 == 4,
          "test_lda_zeropage_x_loads_value_with_index");
}

static void test_lda_zeropage_x_wraps_within_zero_page(void) {
    setup();
    x = 0xFF;
    test_ram[0x0400] = 0xB5; /* LDA $80,X -> ($80+$FF)&$FF = $7F */
    test_ram[0x0401] = 0x80;
    test_ram[0x007F] = 0x22;
    test_ram[0x017F] = 0xEE; /* decoy: would be hit if wrap were wrong */
    step6502();
    CHECK(a == 0x22 && pc == 0x0402 && clockticks6502 == 4,
          "test_lda_zeropage_x_wraps_within_zero_page");
}

static void test_sta_zeropage_x_stores_value_with_index(void) {
    setup();
    a = 0x33;
    x = 0x02;
    test_ram[0x0400] = 0x95; /* STA $40,X */
    test_ram[0x0401] = 0x40;
    step6502();
    CHECK(test_ram[0x0042] == 0x33 && pc == 0x0402 && clockticks6502 == 4,
          "test_sta_zeropage_x_stores_value_with_index");
}

static void test_lda_absolute_x_loads_value_with_index(void) {
    setup();
    x = 0x05;
    test_ram[0x0400] = 0xBD; /* LDA $1000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x10;
    test_ram[0x1005] = 0x71;
    step6502();
    CHECK(a == 0x71 && pc == 0x0403 && clockticks6502 == 4,
          "test_lda_absolute_x_loads_value_with_index");
}

static void test_lda_absolute_x_extra_cycle_when_page_crossed(void) {
    setup();
    x = 0x01;
    test_ram[0x0400] = 0xBD; /* LDA $10FF,X -> $1100 crosses page */
    test_ram[0x0401] = 0xFF;
    test_ram[0x0402] = 0x10;
    test_ram[0x1100] = 0x5A;
    step6502();
    CHECK(a == 0x5A && pc == 0x0403 && clockticks6502 == 5,
          "test_lda_absolute_x_extra_cycle_when_page_crossed");
}

static void test_lda_absolute_y_loads_value_with_index(void) {
    setup();
    y = 0x03;
    test_ram[0x0400] = 0xB9; /* LDA $2000,Y */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x20;
    test_ram[0x2003] = 0x38;
    step6502();
    CHECK(a == 0x38 && pc == 0x0403 && clockticks6502 == 4,
          "test_lda_absolute_y_loads_value_with_index");
}

static void test_lda_absolute_y_extra_cycle_when_page_crossed(void) {
    setup();
    y = 0x01;
    test_ram[0x0400] = 0xB9; /* LDA $20FF,Y -> $2100 crosses page */
    test_ram[0x0401] = 0xFF;
    test_ram[0x0402] = 0x20;
    test_ram[0x2100] = 0x61;
    step6502();
    CHECK(a == 0x61 && pc == 0x0403 && clockticks6502 == 5,
          "test_lda_absolute_y_extra_cycle_when_page_crossed");
}

static void test_sta_absolute_x_stores_value_with_index(void) {
    setup();
    a = 0x11;
    x = 0x02;
    test_ram[0x0400] = 0x9D; /* STA $3000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x30;
    step6502();
    /* STA absolute,X is always 5 cycles regardless of page crossing. */
    CHECK(test_ram[0x3002] == 0x11 && pc == 0x0403 && clockticks6502 == 5,
          "test_sta_absolute_x_stores_value_with_index");
}

static void test_sta_absolute_y_stores_value_with_index(void) {
    setup();
    a = 0x44;
    y = 0x02;
    test_ram[0x0400] = 0x99; /* STA $4000,Y */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x40;
    step6502();
    /* STA absolute,Y is always 5 cycles regardless of page crossing. */
    CHECK(test_ram[0x4002] == 0x44 && pc == 0x0403 && clockticks6502 == 5,
          "test_sta_absolute_y_stores_value_with_index");
}

static void test_adc_absolute_x_adds_value_with_index(void) {
    setup();
    status &= (uint8_t)~FLAG_CARRY;
    a = 0x10;
    x = 0x04;
    test_ram[0x0400] = 0x7D; /* ADC $5000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x50;
    test_ram[0x5004] = 0x05;
    step6502();
    CHECK(a == 0x15 && pc == 0x0403 && clockticks6502 == 4,
          "test_adc_absolute_x_adds_value_with_index");
}

static void test_adc_absolute_y_extra_cycle_when_page_crossed(void) {
    setup();
    status &= (uint8_t)~FLAG_CARRY;
    a = 0x01;
    y = 0x01;
    test_ram[0x0400] = 0x79; /* ADC $50FF,Y -> $5100 crosses page */
    test_ram[0x0401] = 0xFF;
    test_ram[0x0402] = 0x50;
    test_ram[0x5100] = 0x01;
    step6502();
    CHECK(a == 0x02 && pc == 0x0403 && clockticks6502 == 5,
          "test_adc_absolute_y_extra_cycle_when_page_crossed");
}

static void test_cmp_absolute_x_compares_with_index(void) {
    setup();
    a = 0x50;
    x = 0x04;
    test_ram[0x0400] = 0xDD; /* CMP $6000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x60;
    test_ram[0x6004] = 0x50;
    step6502();
    CHECK((status & FLAG_ZERO) != 0 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0403 && clockticks6502 == 4,
          "test_cmp_absolute_x_compares_with_index");
}

static void test_cmp_absolute_y_extra_cycle_when_page_crossed(void) {
    setup();
    a = 0x10;
    y = 0x01;
    test_ram[0x0400] = 0xD9; /* CMP $70FF,Y -> $7100 crosses page */
    test_ram[0x0401] = 0xFF;
    test_ram[0x0402] = 0x70;
    test_ram[0x7100] = 0x10;
    step6502();
    CHECK((status & FLAG_ZERO) != 0 && pc == 0x0403 && clockticks6502 == 5,
          "test_cmp_absolute_y_extra_cycle_when_page_crossed");
}

static void test_ldx_absolute_y_loads_value_with_index(void) {
    setup();
    y = 0x05;
    test_ram[0x0400] = 0xBE; /* LDX $8000,Y */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x80;
    test_ram[0x8005] = 0x2B;
    step6502();
    CHECK(x == 0x2B && pc == 0x0403 && clockticks6502 == 4,
          "test_ldx_absolute_y_loads_value_with_index");
}

static void test_ldx_absolute_y_extra_cycle_when_page_crossed(void) {
    setup();
    y = 0x01;
    test_ram[0x0400] = 0xBE; /* LDX $80FF,Y -> $8100 crosses page */
    test_ram[0x0401] = 0xFF;
    test_ram[0x0402] = 0x80;
    test_ram[0x8100] = 0x3C;
    step6502();
    CHECK(x == 0x3C && pc == 0x0403 && clockticks6502 == 5,
          "test_ldx_absolute_y_extra_cycle_when_page_crossed");
}

static void test_ldy_absolute_x_loads_value_with_index(void) {
    setup();
    x = 0x05;
    test_ram[0x0400] = 0xBC; /* LDY $9000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x90;
    test_ram[0x9005] = 0x4D;
    step6502();
    CHECK(y == 0x4D && pc == 0x0403 && clockticks6502 == 4,
          "test_ldy_absolute_x_loads_value_with_index");
}

static void test_ldy_absolute_x_extra_cycle_when_page_crossed(void) {
    setup();
    x = 0x01;
    test_ram[0x0400] = 0xBC; /* LDY $90FF,X -> $9100 crosses page */
    test_ram[0x0401] = 0xFF;
    test_ram[0x0402] = 0x90;
    test_ram[0x9100] = 0x5E;
    step6502();
    CHECK(y == 0x5E && pc == 0x0403 && clockticks6502 == 5,
          "test_ldy_absolute_x_extra_cycle_when_page_crossed");
}

static void test_ldx_zeropage_y_loads_value_with_index(void) {
    setup();
    y = 0x03;
    test_ram[0x0400] = 0xB6; /* LDX $10,Y */
    test_ram[0x0401] = 0x10;
    test_ram[0x0013] = 0x6F; /* $10 + Y = $13 */
    step6502();
    CHECK(x == 0x6F && pc == 0x0402 && clockticks6502 == 4,
          "test_ldx_zeropage_y_loads_value_with_index");
}

static void test_ldx_zeropage_y_wraps_within_zero_page(void) {
    setup();
    y = 0xFF;
    test_ram[0x0400] = 0xB6; /* LDX $80,Y -> ($80+$FF)&$FF = $7F */
    test_ram[0x0401] = 0x80;
    test_ram[0x007F] = 0x19;
    test_ram[0x017F] = 0xEE; /* decoy: would be hit if wrap were wrong */
    step6502();
    CHECK(x == 0x19 && pc == 0x0402 && clockticks6502 == 4,
          "test_ldx_zeropage_y_wraps_within_zero_page");
}

static void test_asl_zeropage_shifts_left_and_sets_carry(void) {
    setup();
    test_ram[0x0010] = 0x81; /* 1000_0001 */
    test_ram[0x0400] = 0x06; /* ASL $10 */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0010] == 0x02 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0402 && clockticks6502 == 5,
          "test_asl_zeropage_shifts_left_and_sets_carry");
}

static void test_asl_zeropage_x_shifts_left_with_index(void) {
    setup();
    x = 0x02;
    test_ram[0x0012] = 0x40;
    test_ram[0x0400] = 0x16; /* ASL $10,X */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0012] == 0x80 && (status & FLAG_SIGN) != 0 &&
          pc == 0x0402 && clockticks6502 == 6,
          "test_asl_zeropage_x_shifts_left_with_index");
}

static void test_asl_absolute_shifts_left_and_sets_carry(void) {
    setup();
    test_ram[0xA000] = 0x81;
    test_ram[0x0400] = 0x0E; /* ASL $A000 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0xA0;
    step6502();
    CHECK(test_ram[0xA000] == 0x02 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0403 && clockticks6502 == 6,
          "test_asl_absolute_shifts_left_and_sets_carry");
}

static void test_asl_absolute_x_shifts_left_with_index(void) {
    setup();
    x = 0x02;
    test_ram[0xB002] = 0x40;
    test_ram[0x0400] = 0x1E; /* ASL $B000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0xB0;
    step6502();
    /* ASL absolute,X is always 7 cycles regardless of page crossing (RMW
     * instruction, no read-side early termination). */
    CHECK(test_ram[0xB002] == 0x80 && pc == 0x0403 && clockticks6502 == 7,
          "test_asl_absolute_x_shifts_left_with_index");
}

static void test_lsr_zeropage_shifts_right_and_sets_carry(void) {
    setup();
    test_ram[0x0010] = 0x03;
    test_ram[0x0400] = 0x46; /* LSR $10 */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0010] == 0x01 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0402 && clockticks6502 == 5,
          "test_lsr_zeropage_shifts_right_and_sets_carry");
}

static void test_lsr_zeropage_x_shifts_right_with_index(void) {
    setup();
    x = 0x02;
    test_ram[0x0012] = 0x03;
    test_ram[0x0400] = 0x56; /* LSR $10,X */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0012] == 0x01 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0402 && clockticks6502 == 6,
          "test_lsr_zeropage_x_shifts_right_with_index");
}

static void test_lsr_absolute_shifts_right_and_sets_carry(void) {
    setup();
    test_ram[0xA000] = 0x03;
    test_ram[0x0400] = 0x4E; /* LSR $A000 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0xA0;
    step6502();
    CHECK(test_ram[0xA000] == 0x01 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0403 && clockticks6502 == 6,
          "test_lsr_absolute_shifts_right_and_sets_carry");
}

static void test_lsr_absolute_x_shifts_right_with_index(void) {
    setup();
    x = 0x02;
    test_ram[0xB002] = 0x03;
    test_ram[0x0400] = 0x5E; /* LSR $B000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0xB0;
    step6502();
    CHECK(test_ram[0xB002] == 0x01 && pc == 0x0403 && clockticks6502 == 7,
          "test_lsr_absolute_x_shifts_right_with_index");
}

static void test_rol_zeropage_rotates_left_through_carry(void) {
    setup();
    status |= FLAG_CARRY;
    test_ram[0x0010] = 0x40; /* carry-in=1 -> 1000_0001, carry-out=0 */
    test_ram[0x0400] = 0x26; /* ROL $10 */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0010] == 0x81 && (status & FLAG_CARRY) == 0 &&
          pc == 0x0402 && clockticks6502 == 5,
          "test_rol_zeropage_rotates_left_through_carry");
}

static void test_rol_zeropage_x_rotates_left_with_index(void) {
    setup();
    status |= FLAG_CARRY;
    x = 0x02;
    test_ram[0x0012] = 0x40;
    test_ram[0x0400] = 0x36; /* ROL $10,X */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0012] == 0x81 && pc == 0x0402 && clockticks6502 == 6,
          "test_rol_zeropage_x_rotates_left_with_index");
}

static void test_rol_absolute_rotates_left_through_carry(void) {
    setup();
    status |= FLAG_CARRY;
    test_ram[0xA000] = 0x40;
    test_ram[0x0400] = 0x2E; /* ROL $A000 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0xA0;
    step6502();
    CHECK(test_ram[0xA000] == 0x81 && pc == 0x0403 && clockticks6502 == 6,
          "test_rol_absolute_rotates_left_through_carry");
}

static void test_rol_absolute_x_rotates_left_with_index(void) {
    setup();
    status |= FLAG_CARRY;
    x = 0x02;
    test_ram[0xB002] = 0x40;
    test_ram[0x0400] = 0x3E; /* ROL $B000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0xB0;
    step6502();
    CHECK(test_ram[0xB002] == 0x81 && pc == 0x0403 && clockticks6502 == 7,
          "test_rol_absolute_x_rotates_left_with_index");
}

static void test_ror_zeropage_rotates_right_through_carry(void) {
    setup();
    status |= FLAG_CARRY;
    test_ram[0x0010] = 0x02; /* carry-in=1 -> 1000_0001, carry-out=0 */
    test_ram[0x0400] = 0x66; /* ROR $10 */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0010] == 0x81 && (status & FLAG_CARRY) == 0 &&
          pc == 0x0402 && clockticks6502 == 5,
          "test_ror_zeropage_rotates_right_through_carry");
}

static void test_ror_zeropage_x_rotates_right_with_index(void) {
    setup();
    status |= FLAG_CARRY;
    x = 0x02;
    test_ram[0x0012] = 0x02;
    test_ram[0x0400] = 0x76; /* ROR $10,X */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0012] == 0x81 && pc == 0x0402 && clockticks6502 == 6,
          "test_ror_zeropage_x_rotates_right_with_index");
}

static void test_ror_absolute_rotates_right_through_carry(void) {
    setup();
    status |= FLAG_CARRY;
    test_ram[0xA000] = 0x02;
    test_ram[0x0400] = 0x6E; /* ROR $A000 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0xA0;
    step6502();
    CHECK(test_ram[0xA000] == 0x81 && pc == 0x0403 && clockticks6502 == 6,
          "test_ror_absolute_rotates_right_through_carry");
}

static void test_ror_absolute_x_rotates_right_with_index(void) {
    setup();
    status |= FLAG_CARRY;
    x = 0x02;
    test_ram[0xB002] = 0x02;
    test_ram[0x0400] = 0x7E; /* ROR $B000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0xB0;
    step6502();
    CHECK(test_ram[0xB002] == 0x81 && pc == 0x0403 && clockticks6502 == 7,
          "test_ror_absolute_x_rotates_right_with_index");
}

static void test_inc_zeropage_x_increments_memory_with_index(void) {
    setup();
    x = 0x02;
    test_ram[0x0012] = 0x7F;
    test_ram[0x0400] = 0xF6; /* INC $10,X */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0012] == 0x80 && (status & FLAG_SIGN) != 0 &&
          pc == 0x0402 && clockticks6502 == 6,
          "test_inc_zeropage_x_increments_memory_with_index");
}

static void test_inc_absolute_increments_memory(void) {
    setup();
    test_ram[0xA000] = 0x7F;
    test_ram[0x0400] = 0xEE; /* INC $A000 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0xA0;
    step6502();
    CHECK(test_ram[0xA000] == 0x80 && pc == 0x0403 && clockticks6502 == 6,
          "test_inc_absolute_increments_memory");
}

static void test_inc_absolute_x_increments_memory_with_index(void) {
    setup();
    x = 0x02;
    test_ram[0xB002] = 0x7F;
    test_ram[0x0400] = 0xFE; /* INC $B000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0xB0;
    step6502();
    CHECK(test_ram[0xB002] == 0x80 && pc == 0x0403 && clockticks6502 == 7,
          "test_inc_absolute_x_increments_memory_with_index");
}

static void test_dec_zeropage_x_decrements_memory_with_index(void) {
    setup();
    x = 0x02;
    test_ram[0x0012] = 0x01;
    test_ram[0x0400] = 0xD6; /* DEC $10,X */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0012] == 0x00 && (status & FLAG_ZERO) != 0 &&
          pc == 0x0402 && clockticks6502 == 6,
          "test_dec_zeropage_x_decrements_memory_with_index");
}

static void test_dec_absolute_decrements_memory(void) {
    setup();
    test_ram[0xA000] = 0x01;
    test_ram[0x0400] = 0xCE; /* DEC $A000 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0xA0;
    step6502();
    CHECK(test_ram[0xA000] == 0x00 && pc == 0x0403 && clockticks6502 == 6,
          "test_dec_absolute_decrements_memory");
}

static void test_dec_absolute_x_decrements_memory_with_index(void) {
    setup();
    x = 0x02;
    test_ram[0xB002] = 0x01;
    test_ram[0x0400] = 0xDE; /* DEC $B000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0xB0;
    step6502();
    CHECK(test_ram[0xB002] == 0x00 && pc == 0x0403 && clockticks6502 == 7,
          "test_dec_absolute_x_decrements_memory_with_index");
}

static void test_jmp_indirect_sets_pc_from_pointer(void) {
    setup();
    test_ram[0x0400] = 0x6C; /* JMP ($0300) */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x03;
    test_ram[0x0300] = 0x34; /* pointer lo */
    test_ram[0x0301] = 0x12; /* pointer hi -> target $1234 */
    step6502();
    CHECK(pc == 0x1234 && clockticks6502 == 5,
          "test_jmp_indirect_sets_pc_from_pointer");
}

static void test_jmp_indirect_has_page_boundary_bug(void) {
    /* Famous NMOS 6502 hardware bug: if the pointer address is at the end
     * of a page ($xxFF), the high byte is fetched from $xx00 (same page)
     * instead of correctly crossing into the next page. Real Apple II
     * software (and Klaus Dormann's suite) depends on replicating this
     * bug, not "fixing" it. */
    setup();
    test_ram[0x0400] = 0x6C; /* JMP ($02FF) */
    test_ram[0x0401] = 0xFF;
    test_ram[0x0402] = 0x02;
    test_ram[0x02FF] = 0x00; /* pointer lo */
    test_ram[0x0200] = 0x40; /* pointer hi -- WRONGLY read from $0200, not $0300 */
    test_ram[0x0300] = 0x99; /* decoy: correct-but-wrong-per-hardware hi byte */
    step6502();
    CHECK(pc == 0x4000 && clockticks6502 == 5,
          "test_jmp_indirect_has_page_boundary_bug");
}

static void test_php_pushes_status_with_break_and_constant_bits_set(void) {
    setup();
    status = FLAG_CARRY | FLAG_ZERO; /* plus FLAG_CONSTANT from setup() */
    test_ram[0x0400] = 0x08; /* PHP */
    step6502();
    /* PHP always pushes status with bits 4 (B) and 5 (constant) forced to 1,
     * per NMOS 6502 semantics -- the pushed byte differs from the live
     * status register whenever B/constant aren't already set. */
    CHECK(test_ram[0x0100 + sp + 1] == (FLAG_CARRY | FLAG_ZERO | FLAG_BREAK | FLAG_CONSTANT) &&
          pc == 0x0401 && clockticks6502 == 3,
          "test_php_pushes_status_with_break_and_constant_bits_set");
}

static void test_plp_pulls_status_but_ignores_break_bit(void) {
    setup();
    test_ram[0x0400] = 0x08; /* PHP: push CARRY|ZERO|BREAK|CONSTANT */
    status = FLAG_CARRY | FLAG_ZERO;
    step6502();
    status = 0; /* clobber so we can prove PLP actually restores it */
    test_ram[0x0401] = 0x28; /* PLP */
    step6502();
    /* PLP restores all flags except B, which stays a pushed/popped
     * convention bit rather than live CPU state (constant bit does come
     * back set since it was pushed set). */
    CHECK((status & FLAG_CARRY) != 0 && (status & FLAG_ZERO) != 0 &&
          pc == 0x0402 && clockticks6502 == 7,
          "test_plp_pulls_status_but_ignores_break_bit");
}

static void test_brk_pushes_pc_and_status_then_jumps_to_irq_vector(void) {
    setup();
    test_ram[0xFFFE] = 0x00; /* IRQ/BRK vector lo */
    test_ram[0xFFFF] = 0x09; /* IRQ/BRK vector hi -> $0900 */
    status = FLAG_CARRY;
    test_ram[0x0400] = 0x00; /* BRK */
    step6502();
    CHECK(pc == 0x0900 && (status & FLAG_INTERRUPT) != 0 &&
          clockticks6502 == 7,
          "test_brk_pushes_pc_and_status_then_jumps_to_irq_vector");
}

static void test_brk_pushed_return_address_is_pc_plus_2(void) {
    /* BRK is a 1-byte opcode but the 6502 treats it as 2 bytes for the
     * pushed return address (a padding/signature byte follows in real
     * usage), so RTI lands 2 bytes past the BRK opcode, not 1. */
    setup();
    test_ram[0xFFFE] = 0x00;
    test_ram[0xFFFF] = 0x09;
    test_ram[0x0400] = 0x00; /* BRK at $0400 */
    step6502();
    uint8_t hi = test_ram[0x0100 + sp + 3];
    uint8_t lo = test_ram[0x0100 + sp + 2];
    uint16_t pushed_pc = (uint16_t)((hi << 8) | lo);
    CHECK(pushed_pc == 0x0402,
          "test_brk_pushed_return_address_is_pc_plus_2");
}

static void test_rti_restores_status_and_pc_from_stack(void) {
    setup();
    test_ram[0xFFFE] = 0x00;
    test_ram[0xFFFF] = 0x09;
    status = FLAG_CARRY;
    test_ram[0x0400] = 0x00; /* BRK -> jumps to $0900 */
    test_ram[0x0900] = 0x40; /* RTI */
    step6502(); /* BRK */
    step6502(); /* RTI */
    CHECK(pc == 0x0402 && (status & FLAG_CARRY) != 0 &&
          clockticks6502 == 13,
          "test_rti_restores_status_and_pc_from_stack");
}

static void test_sta_absolute_stores_accumulator(void) {
    setup();
    a = 0x42;
    test_ram[0x0400] = 0x8D; /* STA $0200 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x02;
    step6502();
    CHECK(test_ram[0x0200] == 0x42 && pc == 0x0403 && clockticks6502 == 4,
          "test_sta_absolute_stores_accumulator");
}

static void test_stx_zeropage_stores_x_register(void) {
    setup();
    x = 0x33;
    test_ram[0x0400] = 0x86; /* STX $10 */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0010] == 0x33 && pc == 0x0402 && clockticks6502 == 3,
          "test_stx_zeropage_stores_x_register");
}

static void test_stx_zeropage_y_stores_x_register_with_index(void) {
    setup();
    x = 0x44;
    y = 0x02;
    test_ram[0x0400] = 0x96; /* STX $10,Y */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0012] == 0x44 && pc == 0x0402 && clockticks6502 == 4,
          "test_stx_zeropage_y_stores_x_register_with_index");
}

static void test_stx_absolute_stores_x_register(void) {
    setup();
    x = 0x55;
    test_ram[0x0400] = 0x8E; /* STX $0300 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x03;
    step6502();
    CHECK(test_ram[0x0300] == 0x55 && pc == 0x0403 && clockticks6502 == 4,
          "test_stx_absolute_stores_x_register");
}

static void test_sty_zeropage_stores_y_register(void) {
    setup();
    y = 0x66;
    test_ram[0x0400] = 0x84; /* STY $10 */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0010] == 0x66 && pc == 0x0402 && clockticks6502 == 3,
          "test_sty_zeropage_stores_y_register");
}

static void test_sty_zeropage_x_stores_y_register_with_index(void) {
    setup();
    y = 0x77;
    x = 0x02;
    test_ram[0x0400] = 0x94; /* STY $10,X */
    test_ram[0x0401] = 0x10;
    step6502();
    CHECK(test_ram[0x0012] == 0x77 && pc == 0x0402 && clockticks6502 == 4,
          "test_sty_zeropage_x_stores_y_register_with_index");
}

static void test_sty_absolute_stores_y_register(void) {
    setup();
    y = 0x88;
    test_ram[0x0400] = 0x8C; /* STY $0300 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x03;
    step6502();
    CHECK(test_ram[0x0300] == 0x88 && pc == 0x0403 && clockticks6502 == 4,
          "test_sty_absolute_stores_y_register");
}

static void test_txs_copies_x_to_stack_pointer(void) {
    setup();
    x = 0xAB;
    test_ram[0x0400] = 0x9A; /* TXS -- does NOT affect flags */
    step6502();
    CHECK(sp == 0xAB && pc == 0x0401 && clockticks6502 == 2,
          "test_txs_copies_x_to_stack_pointer");
}

static void test_tsx_copies_stack_pointer_to_x(void) {
    setup();
    sp = 0xCD;
    test_ram[0x0400] = 0xBA; /* TSX */
    step6502();
    CHECK(x == 0xCD && pc == 0x0401 && clockticks6502 == 2 &&
          (status & FLAG_SIGN) != 0,
          "test_tsx_copies_stack_pointer_to_x");
}

static void test_cld_clears_decimal_flag(void) {
    setup();
    status |= FLAG_DECIMAL;
    test_ram[0x0400] = 0xD8; /* CLD */
    step6502();
    CHECK((status & FLAG_DECIMAL) == 0 && pc == 0x0401 && clockticks6502 == 2,
          "test_cld_clears_decimal_flag");
}

static void test_sed_sets_decimal_flag(void) {
    setup();
    status &= (uint8_t)~FLAG_DECIMAL;
    test_ram[0x0400] = 0xF8; /* SED */
    step6502();
    CHECK((status & FLAG_DECIMAL) != 0 && pc == 0x0401 && clockticks6502 == 2,
          "test_sed_sets_decimal_flag");
}

static void test_lda_absolute_loads_value(void) {
    /* Found via Klaus Dormann's functional_test.bin integration run:
     * LDA absolute was missing, causing the CPU to consume only the
     * opcode byte and desync PC against the address operand bytes. */
    setup();
    test_ram[0x0400] = 0xAD; /* LDA $0200 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x02;
    test_ram[0x0200] = 0x5C;
    step6502();
    CHECK(a == 0x5C && pc == 0x0403 && clockticks6502 == 4,
          "test_lda_absolute_loads_value");
}

static void test_ldx_absolute_loads_value(void) {
    setup();
    test_ram[0x0400] = 0xAE; /* LDX $0200 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x02;
    test_ram[0x0200] = 0x6D;
    step6502();
    CHECK(x == 0x6D && pc == 0x0403 && clockticks6502 == 4,
          "test_ldx_absolute_loads_value");
}

static void test_ldy_absolute_loads_value(void) {
    setup();
    test_ram[0x0400] = 0xAC; /* LDY $0200 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x02;
    test_ram[0x0200] = 0x7E;
    step6502();
    CHECK(y == 0x7E && pc == 0x0403 && clockticks6502 == 4,
          "test_ldy_absolute_loads_value");
}

static void test_and_absolute_masks_accumulator(void) {
    /* Found via Klaus Dormann's functional_test.bin integration run:
     * AND/ORA/EOR/ADC/SBC/CMP absolute were all missing (only the
     * indexed and immediate forms existed). */
    setup();
    a = 0xF0;
    test_ram[0x0400] = 0x2D; /* AND $0200 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x02;
    test_ram[0x0200] = 0x0F;
    step6502();
    CHECK(a == 0x00 && pc == 0x0403 && clockticks6502 == 4,
          "test_and_absolute_masks_accumulator");
}

static void test_ora_absolute_sets_bits(void) {
    setup();
    a = 0x0F;
    test_ram[0x0400] = 0x0D; /* ORA $0200 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x02;
    test_ram[0x0200] = 0xF0;
    step6502();
    CHECK(a == 0xFF && pc == 0x0403 && clockticks6502 == 4,
          "test_ora_absolute_sets_bits");
}

static void test_eor_absolute_toggles_bits(void) {
    setup();
    a = 0xFF;
    test_ram[0x0400] = 0x4D; /* EOR $0200 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x02;
    test_ram[0x0200] = 0x0F;
    step6502();
    CHECK(a == 0xF0 && pc == 0x0403 && clockticks6502 == 4,
          "test_eor_absolute_toggles_bits");
}

static void test_adc_absolute_adds_value(void) {
    setup();
    status &= (uint8_t)~FLAG_CARRY;
    a = 0x10;
    test_ram[0x0400] = 0x6D; /* ADC $0200 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x02;
    test_ram[0x0200] = 0x05;
    step6502();
    CHECK(a == 0x15 && pc == 0x0403 && clockticks6502 == 4,
          "test_adc_absolute_adds_value");
}

static void test_sbc_absolute_subtracts_value(void) {
    setup();
    status |= FLAG_CARRY;
    a = 0x10;
    test_ram[0x0400] = 0xED; /* SBC $0200 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x02;
    test_ram[0x0200] = 0x05;
    step6502();
    CHECK(a == 0x0B && pc == 0x0403 && clockticks6502 == 4,
          "test_sbc_absolute_subtracts_value");
}

static void test_cmp_absolute_compares_value(void) {
    setup();
    a = 0x30;
    test_ram[0x0400] = 0xCD; /* CMP $0200 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x02;
    test_ram[0x0200] = 0x30;
    step6502();
    CHECK((status & FLAG_ZERO) != 0 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0403 && clockticks6502 == 4,
          "test_cmp_absolute_compares_value");
}

static void test_bvc_branches_when_overflow_flag_clear(void) {
    /* Found via Klaus Dormann's functional_test.bin integration run:
     * BVC/BVS were both missing. */
    setup();
    status &= (uint8_t)~FLAG_OVERFLOW;
    test_ram[0x0400] = 0x50; /* BVC +4 */
    test_ram[0x0401] = 0x04;
    step6502();
    CHECK(pc == 0x0406 && clockticks6502 == 3,
          "test_bvc_branches_when_overflow_flag_clear");
}

static void test_bvc_does_not_branch_when_overflow_flag_set(void) {
    setup();
    status |= FLAG_OVERFLOW;
    test_ram[0x0400] = 0x50; /* BVC +4 */
    test_ram[0x0401] = 0x04;
    step6502();
    CHECK(pc == 0x0402 && clockticks6502 == 2,
          "test_bvc_does_not_branch_when_overflow_flag_set");
}

static void test_bvs_branches_when_overflow_flag_set(void) {
    setup();
    status |= FLAG_OVERFLOW;
    test_ram[0x0400] = 0x70; /* BVS +3 */
    test_ram[0x0401] = 0x03;
    step6502();
    CHECK(pc == 0x0405 && clockticks6502 == 3,
          "test_bvs_branches_when_overflow_flag_set");
}

static void test_bvs_does_not_branch_when_overflow_flag_clear(void) {
    setup();
    status &= (uint8_t)~FLAG_OVERFLOW;
    test_ram[0x0400] = 0x70; /* BVS +3 */
    test_ram[0x0401] = 0x03;
    step6502();
    CHECK(pc == 0x0402 && clockticks6502 == 2,
          "test_bvs_does_not_branch_when_overflow_flag_clear");
}

static void test_ldx_zeropage_loads_value(void) {
    /* Found via Klaus Dormann's functional_test.bin integration run:
     * LDX/LDY zeropage were both missing. */
    setup();
    test_ram[0x0400] = 0xA6; /* LDX $10 */
    test_ram[0x0401] = 0x10;
    test_ram[0x0010] = 0x3D;
    step6502();
    CHECK(x == 0x3D && pc == 0x0402 && clockticks6502 == 3,
          "test_ldx_zeropage_loads_value");
}

static void test_ldy_zeropage_loads_value(void) {
    setup();
    test_ram[0x0400] = 0xA4; /* LDY $10 */
    test_ram[0x0401] = 0x10;
    test_ram[0x0010] = 0x4E;
    step6502();
    CHECK(y == 0x4E && pc == 0x0402 && clockticks6502 == 3,
          "test_ldy_zeropage_loads_value");
}

static void test_ldy_zeropage_x_loads_value_with_index(void) {
    /* Found via Klaus Dormann's functional_test.bin integration run:
     * LDY zeropage,X was missing. */
    setup();
    x = 0x03;
    test_ram[0x0400] = 0xB4; /* LDY $10,X */
    test_ram[0x0401] = 0x10;
    test_ram[0x0013] = 0x5F; /* $10 + X = $13 */
    step6502();
    CHECK(y == 0x5F && pc == 0x0402 && clockticks6502 == 4,
          "test_ldy_zeropage_x_loads_value_with_index");
}

static void test_and_zeropage_x_masks_accumulator_with_index(void) {
    /* Found via Klaus Dormann's functional_test.bin integration run:
     * AND/ORA/EOR/ADC/SBC/CMP zeropage,X were all missing. */
    setup();
    a = 0xF0;
    x = 0x02;
    test_ram[0x0400] = 0x35; /* AND $10,X */
    test_ram[0x0401] = 0x10;
    test_ram[0x0012] = 0x0F;
    step6502();
    CHECK(a == 0x00 && pc == 0x0402 && clockticks6502 == 4,
          "test_and_zeropage_x_masks_accumulator_with_index");
}

static void test_ora_zeropage_x_sets_bits_with_index(void) {
    setup();
    a = 0x0F;
    x = 0x02;
    test_ram[0x0400] = 0x15; /* ORA $10,X */
    test_ram[0x0401] = 0x10;
    test_ram[0x0012] = 0xF0;
    step6502();
    CHECK(a == 0xFF && pc == 0x0402 && clockticks6502 == 4,
          "test_ora_zeropage_x_sets_bits_with_index");
}

static void test_eor_zeropage_x_toggles_bits_with_index(void) {
    setup();
    a = 0xFF;
    x = 0x02;
    test_ram[0x0400] = 0x55; /* EOR $10,X */
    test_ram[0x0401] = 0x10;
    test_ram[0x0012] = 0x0F;
    step6502();
    CHECK(a == 0xF0 && pc == 0x0402 && clockticks6502 == 4,
          "test_eor_zeropage_x_toggles_bits_with_index");
}

static void test_adc_zeropage_x_adds_value_with_index(void) {
    setup();
    status &= (uint8_t)~FLAG_CARRY;
    a = 0x10;
    x = 0x02;
    test_ram[0x0400] = 0x75; /* ADC $10,X */
    test_ram[0x0401] = 0x10;
    test_ram[0x0012] = 0x05;
    step6502();
    CHECK(a == 0x15 && pc == 0x0402 && clockticks6502 == 4,
          "test_adc_zeropage_x_adds_value_with_index");
}

static void test_sbc_zeropage_x_subtracts_value_with_index(void) {
    setup();
    status |= FLAG_CARRY;
    a = 0x10;
    x = 0x02;
    test_ram[0x0400] = 0xF5; /* SBC $10,X */
    test_ram[0x0401] = 0x10;
    test_ram[0x0012] = 0x05;
    step6502();
    CHECK(a == 0x0B && pc == 0x0402 && clockticks6502 == 4,
          "test_sbc_zeropage_x_subtracts_value_with_index");
}

static void test_cmp_zeropage_x_compares_value_with_index(void) {
    setup();
    a = 0x30;
    x = 0x02;
    test_ram[0x0400] = 0xD5; /* CMP $10,X */
    test_ram[0x0401] = 0x10;
    test_ram[0x0012] = 0x30;
    step6502();
    CHECK((status & FLAG_ZERO) != 0 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0402 && clockticks6502 == 4,
          "test_cmp_zeropage_x_compares_value_with_index");
}

static void test_and_zeropage_masks_accumulator(void) {
    /* Found via Klaus Dormann's functional_test.bin integration run:
     * AND/ORA/EOR/ADC/SBC/CMP zeropage were all missing (only the
     * immediate/zeropage,X/absolute forms existed). */
    setup();
    a = 0xF0;
    test_ram[0x0400] = 0x25; /* AND $10 */
    test_ram[0x0401] = 0x10;
    test_ram[0x0010] = 0x0F;
    step6502();
    CHECK(a == 0x00 && pc == 0x0402 && clockticks6502 == 3,
          "test_and_zeropage_masks_accumulator");
}

static void test_ora_zeropage_sets_bits(void) {
    setup();
    a = 0x0F;
    test_ram[0x0400] = 0x05; /* ORA $10 */
    test_ram[0x0401] = 0x10;
    test_ram[0x0010] = 0xF0;
    step6502();
    CHECK(a == 0xFF && pc == 0x0402 && clockticks6502 == 3,
          "test_ora_zeropage_sets_bits");
}

static void test_eor_zeropage_toggles_bits(void) {
    setup();
    a = 0xFF;
    test_ram[0x0400] = 0x45; /* EOR $10 */
    test_ram[0x0401] = 0x10;
    test_ram[0x0010] = 0x0F;
    step6502();
    CHECK(a == 0xF0 && pc == 0x0402 && clockticks6502 == 3,
          "test_eor_zeropage_toggles_bits");
}

static void test_adc_zeropage_adds_value(void) {
    setup();
    status &= (uint8_t)~FLAG_CARRY;
    a = 0x10;
    test_ram[0x0400] = 0x65; /* ADC $10 */
    test_ram[0x0401] = 0x10;
    test_ram[0x0010] = 0x05;
    step6502();
    CHECK(a == 0x15 && pc == 0x0402 && clockticks6502 == 3,
          "test_adc_zeropage_adds_value");
}

static void test_sbc_zeropage_subtracts_value(void) {
    setup();
    status |= FLAG_CARRY;
    a = 0x10;
    test_ram[0x0400] = 0xE5; /* SBC $10 */
    test_ram[0x0401] = 0x10;
    test_ram[0x0010] = 0x05;
    step6502();
    CHECK(a == 0x0B && pc == 0x0402 && clockticks6502 == 3,
          "test_sbc_zeropage_subtracts_value");
}

static void test_cmp_zeropage_compares_value(void) {
    setup();
    a = 0x30;
    test_ram[0x0400] = 0xC5; /* CMP $10 */
    test_ram[0x0401] = 0x10;
    test_ram[0x0010] = 0x30;
    step6502();
    CHECK((status & FLAG_ZERO) != 0 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0402 && clockticks6502 == 3,
          "test_cmp_zeropage_compares_value");
}

static void test_adc_decimal_adds_bcd_digits_without_carry(void) {
    /* Fable-5's independent review found ADC/SBC decimal (BCD) mode was
     * entirely unimplemented -- confirmed by running
     * tests/test_functional_suite.c against Klaus Dormann's suite, which
     * fails once it reaches decimal-mode ADC/SBC tests. Klaus's suite
     * (see .a65 header) explicitly does NOT check N/V/Z flags in decimal
     * mode, only the numeric result and carry -- so these tests follow
     * that same contract. */
    setup();
    status |= FLAG_DECIMAL;
    status &= (uint8_t)~FLAG_CARRY;
    a = 0x25; /* BCD 25 */
    test_ram[0x0400] = 0x69; /* ADC #$25 (BCD 25) */
    test_ram[0x0401] = 0x25;
    step6502();
    CHECK(a == 0x50 && (status & FLAG_CARRY) == 0,
          "test_adc_decimal_adds_bcd_digits_without_carry"); /* 25+25=50 */
}

static void test_adc_decimal_sets_carry_on_bcd_overflow(void) {
    setup();
    status |= FLAG_DECIMAL;
    status &= (uint8_t)~FLAG_CARRY;
    a = 0x99; /* BCD 99 */
    test_ram[0x0400] = 0x69; /* ADC #$01 (BCD 1) */
    test_ram[0x0401] = 0x01;
    step6502();
    CHECK(a == 0x00 && (status & FLAG_CARRY) != 0,
          "test_adc_decimal_sets_carry_on_bcd_overflow"); /* 99+1=100 -> 00 c=1 */
}

static void test_adc_decimal_low_nibble_carries_into_high_nibble(void) {
    setup();
    status |= FLAG_DECIMAL;
    status &= (uint8_t)~FLAG_CARRY;
    a = 0x58; /* BCD 58 */
    test_ram[0x0400] = 0x69; /* ADC #$46 (BCD 46) */
    test_ram[0x0401] = 0x46;
    step6502();
    CHECK(a == 0x04 && (status & FLAG_CARRY) != 0,
          "test_adc_decimal_low_nibble_carries_into_high_nibble"); /* 58+46=104 -> 04 c=1 */
}

static void test_sbc_decimal_subtracts_bcd_digits_with_carry_set(void) {
    setup();
    status |= FLAG_DECIMAL;
    status |= FLAG_CARRY; /* carry set = no borrow, per NMOS convention */
    a = 0x46; /* BCD 46 */
    test_ram[0x0400] = 0xE9; /* SBC #$12 (BCD 12) */
    test_ram[0x0401] = 0x12;
    step6502();
    CHECK(a == 0x34 && (status & FLAG_CARRY) != 0,
          "test_sbc_decimal_subtracts_bcd_digits_with_carry_set"); /* 46-12=34 */
}

static void test_sbc_decimal_borrows_across_nibble_boundary(void) {
    setup();
    status |= FLAG_DECIMAL;
    status |= FLAG_CARRY;
    a = 0x40; /* BCD 40 */
    test_ram[0x0400] = 0xE9; /* SBC #$13 (BCD 13) */
    test_ram[0x0401] = 0x13;
    step6502();
    CHECK(a == 0x27 && (status & FLAG_CARRY) != 0,
          "test_sbc_decimal_borrows_across_nibble_boundary"); /* 40-13=27 */
}

static void test_cpx_zeropage_compares_x_register(void) {
    /* Found via Klaus Dormann's functional_test.bin integration run:
     * CPX/CPY zeropage were both missing. */
    setup();
    x = 0x40;
    test_ram[0x0400] = 0xE4; /* CPX $10 */
    test_ram[0x0401] = 0x10;
    test_ram[0x0010] = 0x40;
    step6502();
    CHECK((status & FLAG_ZERO) != 0 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0402 && clockticks6502 == 3,
          "test_cpx_zeropage_compares_x_register");
}

static void test_cpy_zeropage_compares_y_register(void) {
    setup();
    y = 0x20;
    test_ram[0x0400] = 0xC4; /* CPY $10 */
    test_ram[0x0401] = 0x10;
    test_ram[0x0010] = 0x40;
    step6502();
    CHECK((status & FLAG_CARRY) == 0 && pc == 0x0402 && clockticks6502 == 3,
          "test_cpy_zeropage_compares_y_register");
}

static void test_cpx_absolute_compares_x_register(void) {
    /* Found via Klaus Dormann's functional_test.bin integration run:
     * CPX/CPY absolute were both missing. */
    setup();
    x = 0xC3;
    test_ram[0x0400] = 0xEC; /* CPX $0200 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x02;
    test_ram[0x0200] = 0xC3;
    step6502();
    CHECK((status & FLAG_ZERO) != 0 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0403 && clockticks6502 == 4,
          "test_cpx_absolute_compares_x_register");
}

static void test_cpy_absolute_compares_y_register(void) {
    setup();
    y = 0x10;
    test_ram[0x0400] = 0xCC; /* CPY $0200 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x02;
    test_ram[0x0200] = 0x40;
    step6502();
    CHECK((status & FLAG_CARRY) == 0 && pc == 0x0403 && clockticks6502 == 4,
          "test_cpy_absolute_compares_y_register");
}

static void test_and_indexed_indirect_masks_accumulator(void) {
    /* Found via Klaus Dormann's functional_test.bin integration run:
     * AND/ORA/EOR/ADC/SBC/CMP for BOTH (zp,X) and (zp),Y were entirely
     * missing -- only LDA/STA had these addressing modes implemented. */
    setup();
    a = 0xF0;
    x = 0x04;
    test_ram[0x0400] = 0x21; /* AND ($20,X) */
    test_ram[0x0401] = 0x20;
    test_ram[0x0024] = 0x00;
    test_ram[0x0025] = 0x05; /* effective addr $0500 */
    test_ram[0x0500] = 0x0F;
    step6502();
    CHECK(a == 0x00 && pc == 0x0402 && clockticks6502 == 6,
          "test_and_indexed_indirect_masks_accumulator");
}

static void test_ora_indexed_indirect_sets_bits(void) {
    setup();
    a = 0x0F;
    x = 0x04;
    test_ram[0x0400] = 0x01; /* ORA ($20,X) */
    test_ram[0x0401] = 0x20;
    test_ram[0x0024] = 0x00;
    test_ram[0x0025] = 0x05;
    test_ram[0x0500] = 0xF0;
    step6502();
    CHECK(a == 0xFF && pc == 0x0402 && clockticks6502 == 6,
          "test_ora_indexed_indirect_sets_bits");
}

static void test_eor_indexed_indirect_toggles_bits(void) {
    setup();
    a = 0xFF;
    x = 0x04;
    test_ram[0x0400] = 0x41; /* EOR ($20,X) */
    test_ram[0x0401] = 0x20;
    test_ram[0x0024] = 0x00;
    test_ram[0x0025] = 0x05;
    test_ram[0x0500] = 0x0F;
    step6502();
    CHECK(a == 0xF0 && pc == 0x0402 && clockticks6502 == 6,
          "test_eor_indexed_indirect_toggles_bits");
}

static void test_adc_indexed_indirect_adds_value(void) {
    setup();
    status &= (uint8_t)~FLAG_CARRY;
    a = 0x10;
    x = 0x04;
    test_ram[0x0400] = 0x61; /* ADC ($20,X) */
    test_ram[0x0401] = 0x20;
    test_ram[0x0024] = 0x00;
    test_ram[0x0025] = 0x05;
    test_ram[0x0500] = 0x05;
    step6502();
    CHECK(a == 0x15 && pc == 0x0402 && clockticks6502 == 6,
          "test_adc_indexed_indirect_adds_value");
}

static void test_sbc_indexed_indirect_subtracts_value(void) {
    setup();
    status |= FLAG_CARRY;
    a = 0x10;
    x = 0x04;
    test_ram[0x0400] = 0xE1; /* SBC ($20,X) */
    test_ram[0x0401] = 0x20;
    test_ram[0x0024] = 0x00;
    test_ram[0x0025] = 0x05;
    test_ram[0x0500] = 0x05;
    step6502();
    CHECK(a == 0x0B && pc == 0x0402 && clockticks6502 == 6,
          "test_sbc_indexed_indirect_subtracts_value");
}

static void test_cmp_indexed_indirect_compares_value(void) {
    setup();
    a = 0x30;
    x = 0x04;
    test_ram[0x0400] = 0xC1; /* CMP ($20,X) */
    test_ram[0x0401] = 0x20;
    test_ram[0x0024] = 0x00;
    test_ram[0x0025] = 0x05;
    test_ram[0x0500] = 0x30;
    step6502();
    CHECK((status & FLAG_ZERO) != 0 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0402 && clockticks6502 == 6,
          "test_cmp_indexed_indirect_compares_value");
}

static void test_and_indirect_indexed_masks_accumulator(void) {
    setup();
    a = 0xF0;
    y = 0x10;
    test_ram[0x0400] = 0x31; /* AND ($30),Y */
    test_ram[0x0401] = 0x30;
    test_ram[0x0030] = 0x00;
    test_ram[0x0031] = 0x06; /* base $0600, +Y=$0610 */
    test_ram[0x0610] = 0x0F;
    step6502();
    CHECK(a == 0x00 && pc == 0x0402 && clockticks6502 == 5,
          "test_and_indirect_indexed_masks_accumulator");
}

static void test_ora_indirect_indexed_sets_bits(void) {
    setup();
    a = 0x0F;
    y = 0x10;
    test_ram[0x0400] = 0x11; /* ORA ($30),Y */
    test_ram[0x0401] = 0x30;
    test_ram[0x0030] = 0x00;
    test_ram[0x0031] = 0x06;
    test_ram[0x0610] = 0xF0;
    step6502();
    CHECK(a == 0xFF && pc == 0x0402 && clockticks6502 == 5,
          "test_ora_indirect_indexed_sets_bits");
}

static void test_eor_indirect_indexed_toggles_bits(void) {
    setup();
    a = 0xFF;
    y = 0x10;
    test_ram[0x0400] = 0x51; /* EOR ($30),Y */
    test_ram[0x0401] = 0x30;
    test_ram[0x0030] = 0x00;
    test_ram[0x0031] = 0x06;
    test_ram[0x0610] = 0x0F;
    step6502();
    CHECK(a == 0xF0 && pc == 0x0402 && clockticks6502 == 5,
          "test_eor_indirect_indexed_toggles_bits");
}

static void test_adc_indirect_indexed_adds_value(void) {
    setup();
    status &= (uint8_t)~FLAG_CARRY;
    a = 0x10;
    y = 0x10;
    test_ram[0x0400] = 0x71; /* ADC ($30),Y */
    test_ram[0x0401] = 0x30;
    test_ram[0x0030] = 0x00;
    test_ram[0x0031] = 0x06;
    test_ram[0x0610] = 0x05;
    step6502();
    CHECK(a == 0x15 && pc == 0x0402 && clockticks6502 == 5,
          "test_adc_indirect_indexed_adds_value");
}

static void test_sbc_indirect_indexed_subtracts_value(void) {
    setup();
    status |= FLAG_CARRY;
    a = 0x10;
    y = 0x10;
    test_ram[0x0400] = 0xF1; /* SBC ($30),Y */
    test_ram[0x0401] = 0x30;
    test_ram[0x0030] = 0x00;
    test_ram[0x0031] = 0x06;
    test_ram[0x0610] = 0x05;
    step6502();
    CHECK(a == 0x0B && pc == 0x0402 && clockticks6502 == 5,
          "test_sbc_indirect_indexed_subtracts_value");
}

static void test_cmp_indirect_indexed_compares_value(void) {
    setup();
    a = 0x30;
    y = 0x10;
    test_ram[0x0400] = 0xD1; /* CMP ($30),Y */
    test_ram[0x0401] = 0x30;
    test_ram[0x0030] = 0x00;
    test_ram[0x0031] = 0x06;
    test_ram[0x0610] = 0x30;
    step6502();
    CHECK((status & FLAG_ZERO) != 0 && (status & FLAG_CARRY) != 0 &&
          pc == 0x0402 && clockticks6502 == 5,
          "test_cmp_indirect_indexed_compares_value");
}

static void test_bit_absolute_sets_flags_from_memory(void) {
    /* Found via Klaus Dormann's functional_test.bin integration run:
     * BIT absolute was missing (only zeropage existed). */
    setup();
    a = 0xFF;
    test_ram[0x0400] = 0x2C; /* BIT $0200 */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x02;
    test_ram[0x0200] = 0xC3; /* bits 7,6 set -> N=1,V=1; A&M!=0 -> Z=0 */
    step6502();
    CHECK((status & FLAG_SIGN) != 0 && (status & FLAG_OVERFLOW) != 0 &&
          (status & FLAG_ZERO) == 0 && pc == 0x0403 && clockticks6502 == 4,
          "test_bit_absolute_sets_flags_from_memory");
}

static void test_and_absolute_x_masks_accumulator_with_index(void) {
    /* Found via Klaus Dormann's functional_test.bin integration run:
     * AND/ORA/EOR absolute,X and absolute,Y were all missing. */
    setup();
    a = 0xF0;
    x = 0x04;
    test_ram[0x0400] = 0x3D; /* AND $5000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x50;
    test_ram[0x5004] = 0x0F;
    step6502();
    CHECK(a == 0x00 && pc == 0x0403 && clockticks6502 == 4,
          "test_and_absolute_x_masks_accumulator_with_index");
}

static void test_and_absolute_y_masks_accumulator_with_index(void) {
    setup();
    a = 0xF0;
    y = 0x04;
    test_ram[0x0400] = 0x39; /* AND $5000,Y */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x50;
    test_ram[0x5004] = 0x0F;
    step6502();
    CHECK(a == 0x00 && pc == 0x0403 && clockticks6502 == 4,
          "test_and_absolute_y_masks_accumulator_with_index");
}

static void test_ora_absolute_x_sets_bits_with_index(void) {
    setup();
    a = 0x0F;
    x = 0x04;
    test_ram[0x0400] = 0x1D; /* ORA $5000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x50;
    test_ram[0x5004] = 0xF0;
    step6502();
    CHECK(a == 0xFF && pc == 0x0403 && clockticks6502 == 4,
          "test_ora_absolute_x_sets_bits_with_index");
}

static void test_ora_absolute_y_sets_bits_with_index(void) {
    setup();
    a = 0x0F;
    y = 0x04;
    test_ram[0x0400] = 0x19; /* ORA $5000,Y */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x50;
    test_ram[0x5004] = 0xF0;
    step6502();
    CHECK(a == 0xFF && pc == 0x0403 && clockticks6502 == 4,
          "test_ora_absolute_y_sets_bits_with_index");
}

static void test_eor_absolute_x_toggles_bits_with_index(void) {
    setup();
    a = 0xFF;
    x = 0x04;
    test_ram[0x0400] = 0x5D; /* EOR $5000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x50;
    test_ram[0x5004] = 0x0F;
    step6502();
    CHECK(a == 0xF0 && pc == 0x0403 && clockticks6502 == 4,
          "test_eor_absolute_x_toggles_bits_with_index");
}

static void test_eor_absolute_y_toggles_bits_with_index(void) {
    setup();
    a = 0xFF;
    y = 0x04;
    test_ram[0x0400] = 0x59; /* EOR $5000,Y */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x50;
    test_ram[0x5004] = 0x0F;
    step6502();
    CHECK(a == 0xF0 && pc == 0x0403 && clockticks6502 == 4,
          "test_eor_absolute_y_toggles_bits_with_index");
}

static void test_sbc_absolute_x_subtracts_value_with_index(void) {
    /* Found via Klaus Dormann's functional_test.bin integration run:
     * SBC absolute,X and absolute,Y were missing (ADC had them, SBC
     * did not -- an asymmetric gap between the two). */
    setup();
    status |= FLAG_CARRY;
    a = 0x10;
    x = 0x04;
    test_ram[0x0400] = 0xFD; /* SBC $5000,X */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x50;
    test_ram[0x5004] = 0x05;
    step6502();
    CHECK(a == 0x0B && pc == 0x0403 && clockticks6502 == 4,
          "test_sbc_absolute_x_subtracts_value_with_index");
}

static void test_sbc_absolute_y_subtracts_value_with_index(void) {
    setup();
    status |= FLAG_CARRY;
    a = 0x10;
    y = 0x04;
    test_ram[0x0400] = 0xF9; /* SBC $5000,Y */
    test_ram[0x0401] = 0x00;
    test_ram[0x0402] = 0x50;
    test_ram[0x5004] = 0x05;
    step6502();
    CHECK(a == 0x0B && pc == 0x0403 && clockticks6502 == 4,
          "test_sbc_absolute_y_subtracts_value_with_index");
}

static void test_illegal_opcode_consumes_one_byte_without_crashing(void) {
    /* baochip confirmed illegal/undocumented NMOS opcodes are out of
     * scope for DOS 3.3/ProDOS/Oregon Trail 1985 (documented opcodes
     * only). But the default-case fallback itself must still be SAFE:
     * it must not crash, hang, or corrupt registers -- just consume the
     * opcode byte and continue, so a stray illegal byte in memory (from
     * misaligned execution, a decode bug elsewhere, etc.) degrades
     * gracefully instead of taking down the whole emulator. 0x02 is one
     * of the unassigned/illegal NMOS opcode slots (real hardware treats
     * it as a JAM/KIL instruction that hangs the bus -- deliberately NOT
     * replicating that hang here, since a soft "safe no-op" is strictly
     * better for an emulator than freezing the whole system). */
    setup();
    a = 0x55;
    x = 0x66;
    y = 0x77;
    status = FLAG_CARRY | FLAG_ZERO;
    test_ram[0x0400] = 0x02; /* illegal/unassigned opcode */
    step6502();
    CHECK(pc == 0x0401 && a == 0x55 && x == 0x66 && y == 0x77 &&
          status == (FLAG_CARRY | FLAG_ZERO) && clockticks6502 == 2,
          "test_illegal_opcode_consumes_one_byte_without_crashing");
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
    test_lda_zeropage_x_loads_value_with_index();
    test_lda_zeropage_x_wraps_within_zero_page();
    test_sta_zeropage_x_stores_value_with_index();
    test_lda_absolute_x_loads_value_with_index();
    test_lda_absolute_x_extra_cycle_when_page_crossed();
    test_lda_absolute_y_loads_value_with_index();
    test_lda_absolute_y_extra_cycle_when_page_crossed();
    test_sta_absolute_x_stores_value_with_index();
    test_sta_absolute_y_stores_value_with_index();
    test_adc_absolute_x_adds_value_with_index();
    test_adc_absolute_y_extra_cycle_when_page_crossed();
    test_cmp_absolute_x_compares_with_index();
    test_cmp_absolute_y_extra_cycle_when_page_crossed();
    test_ldx_absolute_y_loads_value_with_index();
    test_ldx_absolute_y_extra_cycle_when_page_crossed();
    test_ldy_absolute_x_loads_value_with_index();
    test_ldy_absolute_x_extra_cycle_when_page_crossed();
    test_ldx_zeropage_y_loads_value_with_index();
    test_ldx_zeropage_y_wraps_within_zero_page();
    test_asl_zeropage_shifts_left_and_sets_carry();
    test_asl_zeropage_x_shifts_left_with_index();
    test_asl_absolute_shifts_left_and_sets_carry();
    test_asl_absolute_x_shifts_left_with_index();
    test_lsr_zeropage_shifts_right_and_sets_carry();
    test_lsr_zeropage_x_shifts_right_with_index();
    test_lsr_absolute_shifts_right_and_sets_carry();
    test_lsr_absolute_x_shifts_right_with_index();
    test_rol_zeropage_rotates_left_through_carry();
    test_rol_zeropage_x_rotates_left_with_index();
    test_rol_absolute_rotates_left_through_carry();
    test_rol_absolute_x_rotates_left_with_index();
    test_ror_zeropage_rotates_right_through_carry();
    test_ror_zeropage_x_rotates_right_with_index();
    test_ror_absolute_rotates_right_through_carry();
    test_ror_absolute_x_rotates_right_with_index();
    test_inc_zeropage_x_increments_memory_with_index();
    test_inc_absolute_increments_memory();
    test_inc_absolute_x_increments_memory_with_index();
    test_dec_zeropage_x_decrements_memory_with_index();
    test_dec_absolute_decrements_memory();
    test_dec_absolute_x_decrements_memory_with_index();
    test_jmp_indirect_sets_pc_from_pointer();
    test_jmp_indirect_has_page_boundary_bug();
    test_php_pushes_status_with_break_and_constant_bits_set();
    test_plp_pulls_status_but_ignores_break_bit();
    test_brk_pushes_pc_and_status_then_jumps_to_irq_vector();
    test_brk_pushed_return_address_is_pc_plus_2();
    test_rti_restores_status_and_pc_from_stack();
    test_sta_absolute_stores_accumulator();
    test_stx_zeropage_stores_x_register();
    test_stx_zeropage_y_stores_x_register_with_index();
    test_stx_absolute_stores_x_register();
    test_sty_zeropage_stores_y_register();
    test_sty_zeropage_x_stores_y_register_with_index();
    test_sty_absolute_stores_y_register();
    test_txs_copies_x_to_stack_pointer();
    test_tsx_copies_stack_pointer_to_x();
    test_cld_clears_decimal_flag();
    test_sed_sets_decimal_flag();
    test_lda_absolute_loads_value();
    test_ldx_absolute_loads_value();
    test_ldy_absolute_loads_value();
    test_and_absolute_masks_accumulator();
    test_ora_absolute_sets_bits();
    test_eor_absolute_toggles_bits();
    test_adc_absolute_adds_value();
    test_sbc_absolute_subtracts_value();
    test_cmp_absolute_compares_value();
    test_bvc_branches_when_overflow_flag_clear();
    test_bvc_does_not_branch_when_overflow_flag_set();
    test_bvs_branches_when_overflow_flag_set();
    test_bvs_does_not_branch_when_overflow_flag_clear();
    test_ldx_zeropage_loads_value();
    test_ldy_zeropage_loads_value();
    test_ldy_zeropage_x_loads_value_with_index();
    test_and_zeropage_x_masks_accumulator_with_index();
    test_ora_zeropage_x_sets_bits_with_index();
    test_eor_zeropage_x_toggles_bits_with_index();
    test_adc_zeropage_x_adds_value_with_index();
    test_sbc_zeropage_x_subtracts_value_with_index();
    test_cmp_zeropage_x_compares_value_with_index();
    test_and_zeropage_masks_accumulator();
    test_ora_zeropage_sets_bits();
    test_eor_zeropage_toggles_bits();
    test_adc_zeropage_adds_value();
    test_sbc_zeropage_subtracts_value();
    test_cmp_zeropage_compares_value();
    test_adc_decimal_adds_bcd_digits_without_carry();
    test_adc_decimal_sets_carry_on_bcd_overflow();
    test_adc_decimal_low_nibble_carries_into_high_nibble();
    test_sbc_decimal_subtracts_bcd_digits_with_carry_set();
    test_sbc_decimal_borrows_across_nibble_boundary();
    test_cpx_zeropage_compares_x_register();
    test_cpy_zeropage_compares_y_register();
    test_cpx_absolute_compares_x_register();
    test_cpy_absolute_compares_y_register();
    test_and_indexed_indirect_masks_accumulator();
    test_ora_indexed_indirect_sets_bits();
    test_eor_indexed_indirect_toggles_bits();
    test_adc_indexed_indirect_adds_value();
    test_sbc_indexed_indirect_subtracts_value();
    test_cmp_indexed_indirect_compares_value();
    test_and_indirect_indexed_masks_accumulator();
    test_ora_indirect_indexed_sets_bits();
    test_eor_indirect_indexed_toggles_bits();
    test_adc_indirect_indexed_adds_value();
    test_sbc_indirect_indexed_subtracts_value();
    test_cmp_indirect_indexed_compares_value();
    test_bit_absolute_sets_flags_from_memory();
    test_and_absolute_x_masks_accumulator_with_index();
    test_and_absolute_y_masks_accumulator_with_index();
    test_ora_absolute_x_sets_bits_with_index();
    test_ora_absolute_y_sets_bits_with_index();
    test_eor_absolute_x_toggles_bits_with_index();
    test_eor_absolute_y_toggles_bits_with_index();
    test_sbc_absolute_x_subtracts_value_with_index();
    test_sbc_absolute_y_subtracts_value_with_index();
    test_illegal_opcode_consumes_one_byte_without_crashing();

    if (failures > 0) {
        printf("\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("\nAll tests passed\n");
    return 0;
}
