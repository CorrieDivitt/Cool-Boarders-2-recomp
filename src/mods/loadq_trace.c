/* Load Queue Trace (developer diagnostic, never shipped).
 *
 * Investigates the 5-8 s black screen before the title menu. During boot the
 * game's ModeTransition (0x800142F0) waits, one VSync per frame, until the
 * main program's background-load service (0x80013C70) reports nothing
 * pending. That service walks a 20-slot job table and has a 181-frame
 * timeout with retry (0x80013FB8).
 *
 * Every VBlank this plugin snapshots the job table, the service's current
 * index and its timeout state, and appends a line to cb2_loadq_trace.log
 * (next to the executable) whenever anything changes. It only reads guest RAM;
 * it never writes it. Logging stops after MAX_FRAMES to bound the file size.
 *
 * Layout (NTSC-U, SCUS-94358), from static analysis:
 *   job table   0x800502E8, 20 slots x 0x20 bytes
 *     +0x00 result / +0x04 handle / +0x08 out pointer / +0x0C flags / +0x10 file id
 *   current slot index        0x80045368
 *   timeout start (vblanks)   0x80045350, timeout armed flag 0x80045354
 *   file-name pointer table   0x800580AC + id * 8
 *   overlay slot module word  0x8005AB68
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mod_plugins.h"

#define Q_TABLE      0x800502E8u
#define Q_SLOTS      20u
#define Q_STRIDE     0x20u
#define Q_INDEX      0x80045368u
#define Q_TO_START   0x80045350u
#define Q_TO_ARMED   0x80045354u
#define NAME_TABLE   0x800580ACu
#define MODULE_WORD  0x8005AB68u
#define MAX_FRAMES   6000u            /* ~100 s at 60 Hz */

typedef struct {
    uint32_t slot[Q_SLOTS][5];
    uint32_t index, to_start, to_armed, module;
} QSnap;

static FILE*    s_log;
static QSnap    s_prev;
static int      s_have_prev;
static uint32_t s_frame;
static clock_t  s_t0;

static int guest_ram(uint32_t a) { return a >= 0x80000000u && a < 0x80200000u; }

static void file_name(uint32_t id, char* out, size_t cap) {
    out[0] = 0;
    if (id > 4096u) return;
    uint32_t p = psx_mod_read_word(NAME_TABLE + id * 8u);
    if (!guest_ram(p)) return;
    size_t i = 0;
    for (; i + 1 < cap; i++) {
        uint8_t c = psx_mod_read_byte(p + (uint32_t)i);
        if (c < 0x20 || c > 0x7E) break;
        out[i] = (char)c;
    }
    out[i] = 0;
}

static void take(QSnap* s) {
    for (uint32_t i = 0; i < Q_SLOTS; i++)
        for (uint32_t f = 0; f < 5; f++)
            s->slot[i][f] = psx_mod_read_word(Q_TABLE + i * Q_STRIDE + f * 4u);
    s->index    = psx_mod_read_word(Q_INDEX);
    s->to_start = psx_mod_read_word(Q_TO_START);
    s->to_armed = psx_mod_read_word(Q_TO_ARMED);
    s->module   = psx_mod_read_word(MODULE_WORD);
}

static void loadq_vblank(void) {
    if (s_frame++ > MAX_FRAMES) {
        if (s_log) { fprintf(s_log, "-- stopped after %u frames --\n", MAX_FRAMES); fclose(s_log); s_log = NULL; }
        return;
    }
    if (!s_log) {
        if (s_frame > 1) return;             /* failed to open on the first frame */
        s_log = fopen("cb2_loadq_trace.log", "w");
        if (!s_log) return;
        s_t0 = clock();
        fprintf(s_log, "# frame  ms  module index timeout(start,armed) | changed slots: id flags handle result name\n");
    }
    QSnap now;
    take(&now);
    if (s_have_prev && memcmp(&now, &s_prev, sizeof now) == 0) return;
    long ms = (long)((clock() - s_t0) * 1000 / CLOCKS_PER_SEC);
    fprintf(s_log, "%6u %7ld  mod=%u idx=%u to=(%u,%u) |", s_frame, ms,
            now.module, now.index, now.to_start, now.to_armed);
    for (uint32_t i = 0; i < Q_SLOTS; i++) {
        if (s_have_prev && memcmp(now.slot[i], s_prev.slot[i], sizeof now.slot[i]) == 0) continue;
        if (!s_have_prev && now.slot[i][3] == 0) continue;
        char name[40];
        file_name(now.slot[i][4], name, sizeof name);
        fprintf(s_log, " [%u] id=%u fl=0x%X h=0x%X r=0x%X %s;", i, now.slot[i][4],
                now.slot[i][3], now.slot[i][1], now.slot[i][0], name);
    }
    fputc('\n', s_log);
    fflush(s_log);
    s_prev = now;
    s_have_prev = 1;
}

PSX_MOD_CONSTRUCTOR(register_cb2_loadq_trace) {
    (void)psx_mod_register_vblank_plugin("cb2.loadq-trace", loadq_vblank);
}
