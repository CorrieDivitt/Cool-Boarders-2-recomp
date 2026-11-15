/* Which Cool Boarders 2 code module is resident (NTSC-U, SCUS-94358).
 *
 * Every module loads into the same RAM slot at 0x8005AB68, so an address there
 * means different things at different times. The first word of the slot is a
 * small module number (INIT 4, MODE_SEL 5, MAIN_SEL 6, board editor 7, every
 * gameplay module and the ending 8). Numbers repeat, so a module is identified
 * by that word plus a CRC-32 of the first 0x40 bytes of the slot, measured from
 * a stock disc. The CRCs are fingerprints; no game code is stored here.
 */
#ifndef CB2_MODULES_H
#define CB2_MODULES_H

#include <stdint.h>
#include "mod_plugins.h"

#define CB2_OVERLAY_SLOT   0x8005AB68u
#define CB2_MODULE_ID_LEN  0x40u

enum {
    CB2_MOD_UNKNOWN = 0,
    CB2_MOD_INIT, CB2_MOD_MODE_SEL, CB2_MOD_MAIN_SEL, CB2_MOD_BDEDIT, CB2_MOD_ENDING,
    CB2_MOD_FREE, CB2_MOD_VS, CB2_MOD_HP, CB2_MOD_BP, CB2_MOD_TM, CB2_MOD_OMT,
    CB2_MOD_SBC1, CB2_MOD_SBC2, CB2_MOD_COMB,
    CB2_MOD_COUNT
};

static inline uint32_t cb2_guest_crc32(uint32_t addr, uint32_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= psx_mod_read_byte(addr + i);
        for (int b = 0; b < 8; b++)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

/* Returns a CB2_MOD_* value for the module in the slot, or CB2_MOD_UNKNOWN for
 * anything else (another game version, a module still loading, no module). */
static inline int cb2_resident_module(void) {
    static const struct { uint32_t number, crc; int id; } k[] = {
        { 4u, 0x38A4EBD8u, CB2_MOD_INIT     },
        { 5u, 0x145718B0u, CB2_MOD_MODE_SEL },
        { 6u, 0x4E6F8A24u, CB2_MOD_MAIN_SEL },
        { 7u, 0x5A89C5E0u, CB2_MOD_BDEDIT   },
        { 8u, 0x1B08D5B0u, CB2_MOD_ENDING   },
        { 8u, 0x4ED5721Bu, CB2_MOD_FREE     },
        { 8u, 0xCD198E13u, CB2_MOD_VS       },
        { 8u, 0xBD12EEC5u, CB2_MOD_HP       },
        { 8u, 0x673A99A5u, CB2_MOD_BP       },
        { 8u, 0xF4074FD2u, CB2_MOD_TM       },
        { 8u, 0xCD94C228u, CB2_MOD_OMT      },
        { 8u, 0xDDAA5D8Cu, CB2_MOD_SBC1     },
        { 8u, 0xBF634351u, CB2_MOD_SBC2     },
        { 8u, 0x6D308BB8u, CB2_MOD_COMB     },
    };
    uint32_t number = psx_mod_read_word(CB2_OVERLAY_SLOT);
    if (number < 4u || number > 8u) return CB2_MOD_UNKNOWN;
    uint32_t crc = cb2_guest_crc32(CB2_OVERLAY_SLOT, CB2_MODULE_ID_LEN);
    for (unsigned i = 0; i < sizeof k / sizeof k[0]; i++)
        if (k[i].number == number && k[i].crc == crc) return k[i].id;
    return CB2_MOD_UNKNOWN;
}

static inline int cb2_option_on(const char* option_id, int fallback) {
    char v[16];
    if (!psx_mod_current_option_value(option_id, v, sizeof v)) return fallback;
    return v[0] == 't' && v[1] == 'r' && v[2] == 'u' && v[3] == 'e' && v[4] == 0;
}

#endif /* CB2_MODULES_H */
