#ifndef CPU6502_H
#define CPU6502_H

#include <stdint.h>

/*
 * Bus interface contract (locked with baochip/Bunnie/Duke, 2026-07-31):
 *
 *   uint8_t read6502(uint16_t address);
 *   void    write6502(uint16_t address, uint8_t value);
 *
 * cpu6502.c is bus-topology-agnostic: every memory access -- RAM, ROM, or
 * soft-switch -- goes through these two functions. It does not know about
 * address ranges, MMIO, or traps. In Step 2 (this harness) the host test
 * runner provides a trivial flat 64KB array. In Step 3, apple2_mem.c
 * (Bunnie/Duke) implements these same signatures to add the $C000-$C0FF
 * soft-switch dispatch, Hi-Res video buffer backing, and ReRAM disk traps.
 */
uint8_t read6502(uint16_t address);
void write6502(uint16_t address, uint8_t value);

/* CPU registers, exposed for test inspection. */
extern uint16_t pc;
extern uint8_t a, x, y, sp, status;
extern uint32_t clockticks6502;

/* Status flag bits (NMOS 6502 processor status register layout). */
#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_CONSTANT  0x20 /* unused bit, always reads as 1 */
#define FLAG_OVERFLOW  0x40
#define FLAG_SIGN      0x80

/* CPU control. */
void reset6502(void);
void exec6502(uint32_t tickcount);
void step6502(void);

#endif /* CPU6502_H */
