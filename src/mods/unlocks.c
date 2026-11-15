/* Unlocks: choose which of Cool Boarders 2's unlockables are available.
 *
 * WHAT THE GAME KEEPS (NTSC-U, SCUS-94358; from static analysis)
 *
 * Progress lives in a block of 16-bit fields in the main program's data:
 *   0x800570FC  Freestyle courses offered (3 at first, 10 after Competition)
 *   0x800570FE  mirror Freestyle courses offered (0, 3 or 10)
 *   0x80057100  rider mask: bits 0-3 the four starting riders,
 *               bit 4 Boss, bit 5 Gray (the alien), bit 6 Snowman
 *   0x80057102  special board mask: bit 2 time records (Alpine board); bits 0
 *               and 1 the other two record tables, taken to be trick points
 *               (Freestyle board) and total score (All-Around board) from
 *               their order, which has not been confirmed in play
 *   0x80057104  mirror mode available (R1 + X on the mode menu)
 *   0x80057106  hard mode available (L1 + X on Competition)
 * The memory card save packs 0x800570F4..0x8005710D (0x80018CC0) and unpacks
 * it again on load (0x80019154), so all six fields are part of the save.
 * The extra outfits are different: a session flag at 0x800570EA that is never
 * saved, set when the mode menu's button-code detector (MODE_SEL 0x8005CC60)
 * sees the outfit code.
 *
 * HOW THIS MOD WORKS
 *
 * The menus are the only code that reads these fields to decide what to offer;
 * the gameplay modules and the ending only write them, when something is
 * earned. So the chosen unlocks are written into the fields while a menu
 * module (MODE_SEL or MAIN_SEL) is resident and the player's real values are
 * put back whenever any other module is, and around every save pack and load
 * unpack. The result:
 *   - the memory card only ever receives what the player really earned, so
 *     switching an unlock off takes it away again;
 *   - the game's own award checks run against the real values, so earning
 *     something for real still works and still shows its message.
 * Outfits are switched on by setting the session flag when the detector runs,
 * which is exactly what entering the code does; the game clears it by itself
 * for the attract demo.
 *
 * STATE
 *
 * The real values and "which field is currently overridden" are kept in guest
 * memory allocated for this mod, not in host variables, so quick saves, quick
 * loads and rewind restore them together with the fields they describe.
 * Allocating that memory changes the quick-save layout: quick saves made with
 * this mod off do not load with it on, and the other way round.
 *
 * Nothing here is taken from the game: only addresses, bit meanings and CRCs.
 */
#include <stdint.h>
#include "mod_plugins.h"
#include "func_override.h"
#include "cb2_modules.h"

#define CB2_OUTFIT_FLAG      0x800570EAu
#define CB2_SAVE_PACK_FN     0x80018CC0u
#define CB2_SAVE_UNPACK_FN   0x80019154u
#define CB2_CODE_DETECTOR_FN 0x8005CC60u     /* MODE_SEL only */
#define CB2_DETECTOR_CRC32   0xD1D83FF1u     /* first 0x40 bytes of the detector */

#define RIDER_BOSS     0x10u
#define RIDER_GRAY     0x20u
#define RIDER_SNOWMAN  0x40u
#define BOARD_FREESTYLE 0x01u
#define BOARD_ALLAROUND 0x02u
#define BOARD_ALPINE    0x04u

enum { F_COURSES, F_MIRROR_COURSES, F_RIDERS, F_BOARDS, F_MIRROR, F_HARD, F_COUNT };

static const uint32_t k_field_addr[F_COUNT] = {
    0x800570FCu, 0x800570FEu, 0x80057100u, 0x80057102u, 0x80057104u, 0x80057106u,
};

/* The player's selection. Host configuration: read at activation and when an
 * option is changed from the in-game menu. */
static uint32_t s_rider_bits, s_board_bits;
static int s_courses, s_mirror, s_hard, s_outfits;

static uint32_t s_state;        /* guest address of this mod's state block */
static int s_reset_pending;     /* clear the state block at the first VBlank */
static int s_in_wrap;           /* a save pack/unpack is running */

/* State block: [0] bitmask of overridden fields, then per field the player's
 * real value and the value written in its place. All 16-bit. */
#define ST_APPLIED      0u
#define ST_REAL(i)      (2u + (uint32_t)(i) * 4u)
#define ST_FORCED(i)    (4u + (uint32_t)(i) * 4u)
#define ST_BYTES        (2u + F_COUNT * 4u)

static uint16_t st_get(uint32_t off)             { return psx_mod_read_half(s_state + off); }
static void     st_set(uint32_t off, uint16_t v) { psx_mod_write_half(s_state + off, v); }

static void read_options(void) {
    s_rider_bits = (cb2_option_on("boss", 1)    ? RIDER_BOSS    : 0u) |
                   (cb2_option_on("gray", 1)    ? RIDER_GRAY    : 0u) |
                   (cb2_option_on("snowman", 1) ? RIDER_SNOWMAN : 0u);
    s_board_bits = (cb2_option_on("board-freestyle", 1) ? BOARD_FREESTYLE : 0u) |
                   (cb2_option_on("board-allaround", 1) ? BOARD_ALLAROUND : 0u) |
                   (cb2_option_on("board-alpine", 1)    ? BOARD_ALPINE    : 0u);
    s_courses = cb2_option_on("courses", 1);
    s_mirror  = cb2_option_on("mirror", 1);
    s_hard    = cb2_option_on("hard", 1);
    s_outfits = cb2_option_on("outfits", 1);
}

/* What a field should read as in the menus, given the player's real value. */
static uint16_t forced_value(int field, uint16_t real) {
    switch (field) {
    case F_COURSES:
        return (s_courses && real < 10u) ? 10u : real;
    case F_MIRROR_COURSES: {
        /* The game offers 3 mirror courses once mirror mode exists and all 10
         * once every course is open; follow the same rule. */
        uint16_t want = !s_mirror ? 0u : (s_courses ? 10u : 3u);
        return real < want ? want : real;
    }
    case F_RIDERS: return (uint16_t)(real | s_rider_bits);
    case F_BOARDS: return (uint16_t)(real | s_board_bits);
    case F_MIRROR: return (s_mirror && real == 0u) ? 1u : real;
    case F_HARD:   return (s_hard && real == 0u) ? 1u : real;
    default:       return real;
    }
}

static void apply_field(int i) {
    uint16_t applied = st_get(ST_APPLIED);
    if (applied & (1u << i)) return;
    uint16_t real = psx_mod_read_half(k_field_addr[i]);
    uint16_t forced = forced_value(i, real);
    if (forced == real) return;
    st_set(ST_REAL(i), real);
    st_set(ST_FORCED(i), forced);
    psx_mod_write_half(k_field_addr[i], forced);
    st_set(ST_APPLIED, (uint16_t)(applied | (1u << i)));
}

static void restore_field(int i) {
    uint16_t applied = st_get(ST_APPLIED);
    if (!(applied & (1u << i))) return;
    uint16_t real = st_get(ST_REAL(i)), forced = st_get(ST_FORCED(i));
    uint16_t now = psx_mod_read_half(k_field_addr[i]);
    if (now != forced) {
        /* The game changed the field while it was overridden. */
        if (i == F_BOARDS && now == 0u) {
            /* In game mode 0 (probably Competition) MAIN_SEL hides the special
             * boards by zeroing this field for as long as it runs and writing
             * its own copy back on the way out. Leave it alone; restore once
             * the copy is back. */
            return;
        }
        if (i == F_RIDERS || i == F_BOARDS) {
            uint16_t added = (uint16_t)(forced ^ real);   /* bits this mod set */
            real = (uint16_t)((now & ~added) | (real & added));
        } else {
            real = now;                                    /* an explicit new value */
        }
    }
    psx_mod_write_half(k_field_addr[i], real);
    st_set(ST_APPLIED, (uint16_t)(applied & ~(1u << i)));
}

static int menu_resident(void) {
    int m = cb2_resident_module();
    if (m != CB2_MOD_MODE_SEL && m != CB2_MOD_MAIN_SEL) return 0;
    /* The rider mask is never zero once the boot module has set the block up. */
    return psx_mod_read_half(k_field_addr[F_RIDERS]) != 0u;
}

static void sync_fields(int menu) {
    for (int i = 0; i < F_COUNT; i++) {
        if (st_get(ST_APPLIED) & (1u << i)) {
            if (!menu) {
                restore_field(i);
            } else if (st_get(ST_FORCED(i)) != forced_value(i, st_get(ST_REAL(i)))) {
                restore_field(i);                 /* the selection changed */
                apply_field(i);
            }
        } else if (menu) {
            apply_field(i);
        }
    }
}

static void unlocks_vblank(void) {
    if (!s_state || s_in_wrap) return;
    if (s_reset_pending) {
        for (uint32_t o = 0; o < ST_BYTES; o += 2u) st_set(o, 0u);
        s_reset_pending = 0;
    }
    sync_fields(menu_resident());
}

/* Save pack and load unpack: run the original with the player's real values in
 * place, then put the overrides back if a menu is still resident. */
static int wrap_save_block(struct CPUState* cpu) {
    if (!s_state || s_reset_pending) return 0;
    s_in_wrap = 1;
    sync_fields(0);
    func_override_call_original(cpu);
    sync_fields(menu_resident());
    s_in_wrap = 0;
    return 1;
}

/* Outfits: before the detector looks for the button code, set the flag it
 * would set. Declines, so the detector itself still runs. */
static int outfits_before_detector(struct CPUState* cpu) {
    (void)cpu;
    /* Other modules have unrelated code at this address; the cheap module-number
     * test keeps this hook out of their way. */
    if (s_outfits && psx_mod_read_word(CB2_OVERLAY_SLOT) == 5u &&
        cb2_resident_module() == CB2_MOD_MODE_SEL &&
        cb2_guest_crc32(CB2_CODE_DETECTOR_FN, 0x40u) == CB2_DETECTOR_CRC32 &&
        psx_mod_read_half(CB2_OUTFIT_FLAG) == 0u)
        psx_mod_write_half(CB2_OUTFIT_FLAG, 1u);
    return 0;
}

static void unlocks_activate(void) {
    if (!s_state) s_state = psx_mod_alloc_guest_memory(32u, 4u);
    s_reset_pending = 1;
    s_in_wrap = 0;
    read_options();
}

static int unlocks_option_changed(const char* option_id, const char* value) {
    (void)option_id; (void)value;
    read_options();
    return 1;       /* picked up at the next VBlank on a menu */
}

PSX_MOD_CONSTRUCTOR(register_cb2_unlocks) {
    (void)psx_mod_register_activation_plugin("cb2.unlocks", unlocks_activate);
    (void)psx_mod_register_vblank_plugin("cb2.unlocks", unlocks_vblank);
    (void)psx_mod_register_option_changed_plugin("cb2.unlocks", unlocks_option_changed);
    /* Wraps: the original accounts for its own time. */
    (void)psx_mod_register_function_override("cb2.unlocks:save-pack",
        CB2_SAVE_PACK_FN, wrap_save_block, NULL, 0, FO_CREDIT_SELF);
    (void)psx_mod_register_function_override("cb2.unlocks:save-unpack",
        CB2_SAVE_UNPACK_FN, wrap_save_block, NULL, 0, FO_CREDIT_SELF);
    /* Pre-hook that always declines: nothing to account for. */
    (void)psx_mod_register_function_override("cb2.unlocks:outfits",
        CB2_CODE_DETECTOR_FN, outfits_before_detector, NULL, 0, 0);
}
