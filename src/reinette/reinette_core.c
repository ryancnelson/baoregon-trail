/*
 * reinette_core.c -- SDL2-free port of reinette-II-plus's Apple II
 * hardware core (memory map, soft switches, Disk ][ nibble stepping),
 * for the spike-reinette-port branch.
 *
 * Derived from third_party/reinette-II-plus/reinetteII+.c (MIT, Arthur
 * Ferreira). Everything SDL2-touched (window/renderer setup, event
 * polling, on-screen rendering, audio queueing, BMP/PNG file I/O,
 * clipboard, screenshots) has been removed or replaced with calls into
 * this project's ramfb_display / bunnie_audio / uart_keyboard_bridge
 * modules -- see reinette_shim.c and NEXT_STEPS_REINETTE_SPIKE.md for
 * what's stubbed vs. real.
 *
 * NOT YET DONE (see NEXT_STEPS_REINETTE_SPIKE.md for the full list):
 *   - Video rendering (TEXT/LoRes/HiRes -> ramfb pixels): stubbed.
 *     Upstream draws characters from font-normal.bmp/font-reverse.bmp
 *     loaded from disk at runtime -- this bare-metal target has no
 *     filesystem, so the font bitmaps need to be embedded as C arrays
 *     (like this project's charrom_342_0133_a.h) before real glyph
 *     rendering can work. Not done in this spike.
 *   - Disk image loading: reinette's insertFloppy()/saveFloppy() use
 *     fopen/fread/fwrite against a host filesystem. Ported to accept a
 *     pointer to an embedded/RRAM-resident nibble image instead (see
 *     reinette_disk_attach()), matching this project's existing
 *     dos33_nib_disk_data.h / disk2_controller.c pattern.
 *   - Paddle/joystick input: kept as pure logic (no SDL dependency to
 *     begin with), untouched.
 */

#include "reinette_core.h"

/* ============================================================ MEMORY */

uint8_t reinette_ram[REINETTE_RAMSIZE];   /* 48K RAM $0000-$BFFF */
uint8_t reinette_rom[REINETTE_ROMSIZE];   /* 12K ROM $D000-$FFFF (appleII+.rom) */
uint8_t reinette_lgc[REINETTE_LGCSIZE];   /* Language Card 12K $D000-$FFFF */
uint8_t reinette_bk2[REINETTE_BK2SIZE];   /* LC bank 2, 4K $D000-$DFFF */
uint8_t reinette_sl6[REINETTE_SL6SIZE];   /* Disk ][ P5A PROM, slot 6 */

/* ================================================== SOFT SWITCH STATE */

uint8_t reinette_KBD = 0;
bool reinette_TEXT  = true;
bool reinette_MIXED = false;
bool reinette_PAGE2 = false;
bool reinette_HIRES = false;
static bool LCWR  = true;
static bool LCRD  = false;
static bool LCBK2 = true;
static bool LCWFF = false;

/* ======================================================== PADDLES */

uint8_t reinette_PB0 = 0;
uint8_t reinette_PB1 = 0;
uint8_t reinette_PB2 = 0;
static float GCP[2] = { 127.0f, 127.0f };
static float GCC[2] = { 0.0f, 0.0f };
static int GCD[2] = { 0, 0 };
static int GCA[2] = { 0, 0 };
uint8_t reinette_GCActionSpeed = 8;
uint8_t reinette_GCReleaseSpeed = 8;
static long long int GCCrigger = 0;

/* ticks: driven by puce6502.c's extern unsigned long long int ticks --
 * defined in reinette_shim.c which owns the emulator main loop timing. */
extern unsigned long long int ticks;

static void resetPaddles(void) {
    GCC[0] = GCP[0] * GCP[0];
    GCC[1] = GCP[1] * GCP[1];
    GCCrigger = (long long int)ticks;
}

static uint8_t readPaddle(int pdl) {
    const float GCFreq = 6.6f;
    GCC[pdl] -= ((float)((long long int)ticks - GCCrigger)) / GCFreq;
    if (GCC[pdl] <= 0) return (uint8_t)(GCC[pdl] = 0);
    return 0x80;
}

void reinette_paddle_tick(void) {
    for (int pdl = 0; pdl < 2; pdl++) {
        if (GCA[pdl]) {
            GCP[pdl] += (float)(GCD[pdl] * reinette_GCActionSpeed);
            if (GCP[pdl] > 255) GCP[pdl] = 255;
            if (GCP[pdl] < 0)   GCP[pdl] = 0;
        } else {
            GCP[pdl] += (float)(GCD[pdl] * reinette_GCReleaseSpeed);
            if (GCD[pdl] == 1  && GCP[pdl] > 127) GCP[pdl] = 127;
            if (GCD[pdl] == -1 && GCP[pdl] < 127) GCP[pdl] = 127;
        }
    }
}

void reinette_paddle_set_direction(int pdl, int direction, int active) {
    if (pdl < 0 || pdl > 1) return;
    GCD[pdl] = direction;
    GCA[pdl] = active;
}

/* ====================================================== DISK ][ */

int reinette_curDrv = 0;
reinette_drive_t reinette_disk[2] = { { 0 } };

/* Attaches a pre-loaded nibble image (already resident in RAM/ROM/RRAM --
 * e.g. from dos33_nib_disk_data.h) to a drive, replacing upstream
 * insertFloppy()'s fopen()/fread() host file load. `data` must point to
 * REINETTE_NIB_IMAGE_SIZE (232960) bytes and outlive the drive attach. */
void reinette_disk_attach(int drv, const uint8_t *data, bool read_only) {
    if (drv < 0 || drv > 1 || !data) return;
    /* NOTE: reinette_drive_t.data is a value array (matches upstream's
     * struct layout) -- copies the image in. For a first spike this is
     * simplest; a real port would likely point directly at RRAM/ROM to
     * avoid the 232KB copy, same as this project's disk2_controller.c
     * already does for its own nibble tracks. */
    for (int i = 0; i < REINETTE_NIB_IMAGE_SIZE; i++) {
        reinette_disk[drv].data[i] = data[i];
    }
    reinette_disk[drv].readOnly = read_only;
    reinette_disk[drv].motorOn = false;
    reinette_disk[drv].writeMode = false;
    reinette_disk[drv].track = 0;
    reinette_disk[drv].nibble = 0;
}

static void stepMotor(uint16_t address) {
    static bool phases[2][4] = { { 0 } };
    static bool phasesB[2][4] = { { 0 } };
    static bool phasesBB[2][4] = { { 0 } };
    static int pIdx[2] = { 0 };
    static int pIdxB[2] = { 0 };
    static int halfTrackPos[2] = { 0 };

    address &= 7;
    int phase = address >> 1;

    phasesBB[reinette_curDrv][pIdxB[reinette_curDrv]] = phasesB[reinette_curDrv][pIdxB[reinette_curDrv]];
    phasesB[reinette_curDrv][pIdx[reinette_curDrv]]   = phases[reinette_curDrv][pIdx[reinette_curDrv]];
    pIdxB[reinette_curDrv] = pIdx[reinette_curDrv];
    pIdx[reinette_curDrv]  = phase;

    if (!(address & 1)) {
        phases[reinette_curDrv][phase] = false;
        return;
    }

    if ((phasesBB[reinette_curDrv][(phase + 1) & 3]) && (--halfTrackPos[reinette_curDrv] < 0))
        halfTrackPos[reinette_curDrv] = 0;

    if ((phasesBB[reinette_curDrv][(phase - 1) & 3]) && (++halfTrackPos[reinette_curDrv] > 140))
        halfTrackPos[reinette_curDrv] = 140;

    phases[reinette_curDrv][phase] = true;
    reinette_disk[reinette_curDrv].track = (uint8_t)((halfTrackPos[reinette_curDrv] + 1) / 2);
}

static void setDrv(int drv) {
    reinette_disk[drv].motorOn = reinette_disk[!drv].motorOn || reinette_disk[drv].motorOn;
    reinette_disk[!drv].motorOn = false;
    reinette_curDrv = drv;
}

/* ================================================== SOFT SWITCHES */

/* playSound()/speaker toggle: forwarded to bunnie_audio.c via a
 * function pointer set by reinette_shim.c at init, so this file stays
 * free of any direct dependency on this project's other modules
 * (keeps the "what came from reinette vs what's shim glue" boundary
 * clean for review). */
static void (*g_speaker_toggle_cb)(void) = 0;

void reinette_set_speaker_callback(void (*cb)(void)) {
    g_speaker_toggle_cb = cb;
}

static uint8_t softSwitches(uint16_t address, uint8_t value, bool WRT) {
    static uint8_t dLatch = 0;

    switch (address) {
        case 0xC000: return reinette_KBD;
        case 0xC010: reinette_KBD &= 0x7F; return reinette_KBD;

        case 0xC020:
        case 0xC030:
        case 0xC033:
            if (g_speaker_toggle_cb) g_speaker_toggle_cb();
            break;

        case 0xC050: reinette_TEXT  = false; break;
        case 0xC051: reinette_TEXT  = true;  break;
        case 0xC052: reinette_MIXED = false; break;
        case 0xC053: reinette_MIXED = true;  break;
        case 0xC054: reinette_PAGE2 = false; break;
        case 0xC055: reinette_PAGE2 = true;  break;
        case 0xC056: reinette_HIRES = false; break;
        case 0xC057: reinette_HIRES = true;  break;

        case 0xC061: return reinette_PB0;
        case 0xC062: return reinette_PB1;
        case 0xC063: return reinette_PB2;
        case 0xC064: return readPaddle(0);
        case 0xC065: return readPaddle(1);

        case 0xC070: resetPaddles(); break;

        case 0xC080:
        case 0xC084: LCBK2 = 1; LCRD = 1; LCWR = 0;      LCWFF = 0;    break;
        case 0xC081:
        case 0xC085: LCBK2 = 1; LCRD = 0; LCWR |= LCWFF; LCWFF = !WRT; break;
        case 0xC082:
        case 0xC086: LCBK2 = 1; LCRD = 0; LCWR = 0;      LCWFF = 0;    break;
        case 0xC083:
        case 0xC087: LCBK2 = 1; LCRD = 1; LCWR |= LCWFF; LCWFF = !WRT; break;
        case 0xC088:
        case 0xC08C: LCBK2 = 0; LCRD = 1; LCWR = 0;      LCWFF = 0;    break;
        case 0xC089:
        case 0xC08D: LCBK2 = 0; LCRD = 0; LCWR |= LCWFF; LCWFF = !WRT; break;
        case 0xC08A:
        case 0xC08E: LCBK2 = 0; LCRD = 0; LCWR = 0;      LCWFF = 0;    break;
        case 0xC08B:
        case 0xC08F: LCBK2 = 0; LCRD = 1; LCWR |= LCWFF; LCWFF = !WRT; break;

        case 0xC0E0:
        case 0xC0E1:
        case 0xC0E2:
        case 0xC0E3:
        case 0xC0E4:
        case 0xC0E5:
        case 0xC0E6:
        case 0xC0E7: stepMotor(address); break;

        case 0xCFFF:
        case 0xC0E8: reinette_disk[reinette_curDrv].motorOn = false; break;
        case 0xC0E9: reinette_disk[reinette_curDrv].motorOn = true;  break;

        case 0xC0EA: setDrv(0); break;
        case 0xC0EB: setDrv(1); break;

        case 0xC0EC:
            if (reinette_disk[reinette_curDrv].writeMode)
                reinette_disk[reinette_curDrv].data[reinette_disk[reinette_curDrv].track * 0x1A00 + reinette_disk[reinette_curDrv].nibble] = dLatch;
            else
                dLatch = reinette_disk[reinette_curDrv].data[reinette_disk[reinette_curDrv].track * 0x1A00 + reinette_disk[reinette_curDrv].nibble];
            reinette_disk[reinette_curDrv].nibble = (uint16_t)((reinette_disk[reinette_curDrv].nibble + 1) % 0x1A00);
            return dLatch;

        case 0xC0ED: dLatch = value; break;

        case 0xC0EE:
            reinette_disk[reinette_curDrv].writeMode = false;
            return reinette_disk[reinette_curDrv].readOnly ? 0x80 : 0;

        case 0xC0EF: reinette_disk[reinette_curDrv].writeMode = true; break;
    }
    return (uint8_t)(ticks % 0xFF);
}

/* ================================================================ MEMORY
 * readMem()/writeMem() -- imported into puce6502_riscv.c via extern
 * declarations in that file (matches upstream's exact contract: puce6502.c
 * declares `extern uint8_t readMem(...)` / `extern void writeMem(...)`
 * and calls them directly, no function pointer indirection). */

uint8_t readMem(uint16_t address) {
    if (address < REINETTE_RAMSIZE)
        return reinette_ram[address];

    if (address >= REINETTE_ROMSTART) {
        if (!LCRD)
            return reinette_rom[address - REINETTE_ROMSTART];
        if (LCBK2 && (address < 0xE000))
            return reinette_bk2[address - REINETTE_BK2START];
        return reinette_lgc[address - REINETTE_LGCSTART];
    }

    if ((address & 0xFF00) == REINETTE_SL6START)
        return reinette_sl6[address - REINETTE_SL6START];

    if ((address & 0xF000) == 0xC000)
        return softSwitches(address, 0, false);

    return (uint8_t)(ticks & 0xFF);
}

void writeMem(uint16_t address, uint8_t value) {
    if (address < REINETTE_RAMSIZE) {
        reinette_ram[address] = value;
        return;
    }

    if (LCWR && (address >= REINETTE_ROMSTART)) {
        if (LCBK2 && (address < 0xE000)) {
            reinette_bk2[address - REINETTE_BK2START] = value;
            return;
        }
        reinette_lgc[address - REINETTE_LGCSTART] = value;
        return;
    }

    if ((address & 0xF000) == 0xC000) {
        softSwitches(address, value, true);
        return;
    }
}
