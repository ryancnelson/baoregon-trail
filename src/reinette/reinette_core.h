/*
 * reinette_core.h -- SDL2-free Apple II hardware core, ported from
 * reinette-II-plus's reinetteII+.c (see reinette_core.c for details).
 */
#ifndef REINETTE_CORE_H
#define REINETTE_CORE_H

#include <stdint.h>
#include <stdbool.h>

#define REINETTE_RAMSIZE   0xC000u
#define REINETTE_ROMSTART  0xD000u
#define REINETTE_ROMSIZE   0x3000u
#define REINETTE_LGCSTART  0xD000u
#define REINETTE_LGCSIZE   0x3000u
#define REINETTE_BK2START  0xD000u
#define REINETTE_BK2SIZE   0x1000u
#define REINETTE_SL6START  0xC600u
#define REINETTE_SL6SIZE   0x0100u

#define REINETTE_NIB_IMAGE_SIZE 232960

extern uint8_t reinette_ram[REINETTE_RAMSIZE];
extern uint8_t reinette_rom[REINETTE_ROMSIZE];
extern uint8_t reinette_lgc[REINETTE_LGCSIZE];
extern uint8_t reinette_bk2[REINETTE_BK2SIZE];
extern uint8_t reinette_sl6[REINETTE_SL6SIZE];

extern uint8_t reinette_KBD;
extern bool reinette_TEXT;
extern bool reinette_MIXED;
extern bool reinette_PAGE2;
extern bool reinette_HIRES;

extern uint8_t reinette_PB0;
extern uint8_t reinette_PB1;
extern uint8_t reinette_PB2;
extern uint8_t reinette_GCActionSpeed;
extern uint8_t reinette_GCReleaseSpeed;

typedef struct {
    char     filename[400];
    bool     readOnly;
    uint8_t  data[REINETTE_NIB_IMAGE_SIZE];
    bool     motorOn;
    bool     writeMode;
    uint8_t  track;
    uint16_t nibble;
} reinette_drive_t;

extern int reinette_curDrv;
extern reinette_drive_t reinette_disk[2];

/* Attach a pre-loaded 232960-byte nibble image to a drive (replaces
 * upstream's file-based insertFloppy()). */
void reinette_disk_attach(int drv, const uint8_t *data, bool read_only);

/* Register a callback invoked on every $C030/$C020/$C033 speaker access
 * (wire this to bunnie_audio_trigger_toggle() at init time). */
void reinette_set_speaker_callback(void (*cb)(void));

/* Paddle/joystick state machine -- call once per frame + on key edges. */
void reinette_paddle_tick(void);
void reinette_paddle_set_direction(int pdl, int direction, int active);

/* readMem()/writeMem(): called directly by puce6502_riscv.c (matches
 * upstream's extern uint8_t readMem()/void writeMem() contract exactly --
 * no function pointer indirection, same as the original). */
uint8_t readMem(uint16_t address);
void writeMem(uint16_t address, uint8_t value);

#endif /* REINETTE_CORE_H */
