/* Skip Intro: boot straight to the Cool Boarders 2 title menu.
 *
 * The first code module (INIT, module number 4, loaded at 0x8005AB68) runs the
 * boot intro: a warning-screen routine at 0x8005AC00 (first boot only) and a
 * logo routine at 0x8005B694 (publisher and UEP Systems screens with jingle),
 * then returns module 6 (MAIN_SEL, the title menu). Neither routine's return
 * value is used and neither writes state that later modules read, so this mod
 * declines to run them.
 *
 * Every module loads into the same RAM slot, so these addresses hold different
 * code in other modules. The override only acts when INIT is resident: the slot's
 * module number must be 4 and a CRC-32 of INIT's entry code must match the
 * NTSC-U (SCUS-94358) value measured from a stock disc. Otherwise it declines
 * and the original code runs. The CRC is a fingerprint; no game code is stored.
 *
 * The CD-audio opening movie is handled separately by the runtime's FMV skip,
 * and the console boot logo by the fast_boot setting.
 */
#include <stdint.h>
#include "mod_plugins.h"

#define CB2_OVERLAY_SLOT      0x8005AB68u
#define CB2_MODULE_INIT       4u
#define CB2_INIT_ENTRY_LEN    0x98u          /* module word + INIT entry function */
#define CB2_INIT_ENTRY_CRC32  0xC9B3EC5Au    /* SCUS-94358, stock INIT.BIN */
#define CB2_INIT_WARNING_FN   0x8005AC00u
#define CB2_INIT_LOGOS_FN     0x8005B694u

static uint32_t guest_crc32(uint32_t addr, uint32_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= psx_mod_read_byte(addr + i);
        for (int b = 0; b < 8; b++)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

static int init_module_resident(void) {
    return psx_mod_read_word(CB2_OVERLAY_SLOT) == CB2_MODULE_INIT &&
           guest_crc32(CB2_OVERLAY_SLOT, CB2_INIT_ENTRY_LEN) == CB2_INIT_ENTRY_CRC32;
}

/* Handled = return to the caller without running the intro screen. */
static int skip_intro_screen(struct CPUState* cpu) {
    (void)cpu;
    return init_module_resident() ? 1 : 0;
}

PSX_MOD_CONSTRUCTOR(register_cb2_skip_intro) {
    /* Credit 0: the skipped screens have no hardware work to account for. */
    (void)psx_mod_register_function_override("cb2.skip-intro:warning",
        CB2_INIT_WARNING_FN, skip_intro_screen, NULL, 0, 0);
    (void)psx_mod_register_function_override("cb2.skip-intro:logos",
        CB2_INIT_LOGOS_FN, skip_intro_screen, NULL, 0, 0);
}
