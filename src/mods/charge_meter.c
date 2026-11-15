/* Trick Charge Meter: show how far a spin and a flip have been wound up.
 *
 * WHAT THE GAME DOES (NTSC-U, SCUS-94358; from static analysis)
 *
 * While the rider crouches (jump held, rider state 2) the game counts frames
 * of held direction, at 30 counts a second:
 *   rider + 0x748  spin wind-up: +1 per frame holding right, -1 holding left
 *   rider + 0x744  flip wind-up: +1 per frame holding up,    -1 holding down
 *   rider + 0x01A  rider state (2 = crouching, winding up)
 * Letting go of the direction resets its count. On take-off a count that is
 * not zero is raised to at least 30. In the air the spin speed follows
 * count / 10 up to a count of 100 and the square root of the count beyond it;
 * the flip speed follows the square root of the count throughout. Both counts
 * are cleared on landing. The offsets are the same in all nine gameplay
 * modules; each module keeps its two rider pointers at its own address.
 *
 * WHAT THIS MOD DOES
 *
 * It only reads. Every VBlank it copies the two counts and the rider state to
 * host variables. The counts are left as they were when a run ends, so the
 * gauge is shown only while the game is still updating that rider (a
 * declining hook on each module's rider update function stamps the time) and,
 * once the crouch ends, for at most HOLD_VBLANKS (a long jump). The title HUD
 * layer then the title HUD layer then draws a cross-shaped gauge over the
 * finished frame: the arm for the direction being held fills up, ten cells an
 * arm, full at a count of 100 (3.3 seconds). A dim fill marks the minimum the
 * take-off will raise the count to, and a white fill shows the slower growth
 * past 100. The gauge is hidden unless the rider is crouching or carrying a
 * wind-up. It is drawn by the host, at window resolution, and is never part of
 * the emulated picture, the game's memory or a save.
 *
 * Drawing uses scissored clears, which need nothing beyond OpenGL 1.0 and
 * leave no state behind once the scissor box, clear colour and colour mask are
 * put back. The HUD layer is OpenGL only; with another renderer the mod does
 * nothing.
 */
#include <stdint.h>
#include <stdatomic.h>
#include <string.h>
#include "mod_plugins.h"
#include "host_overlay.h"
#include "cb2_modules.h"

#define RIDER_STATE       0x01Au
#define RIDER_FLIP        0x744u
#define RIDER_SPIN        0x748u
#define STATE_CROUCH      2
#define COUNT_FULL        100      /* linear range of the spin */
#define COUNT_MIN_LAUNCH  30
#define CELLS             10
#define STALE_VBLANKS     6        /* rider not updated for this long: run over or paused */
#define HOLD_VBLANKS      240      /* show a carried charge this long after the crouch */

/* Address of the two rider pointers in each gameplay module. */
static uint32_t rider_table(int module) {
    switch (module) {
    case CB2_MOD_FREE: return 0x800A39C8u;
    case CB2_MOD_VS:   return 0x800A3260u;
    case CB2_MOD_HP:   return 0x8009B878u;
    case CB2_MOD_BP:   return 0x8009FA4Cu;
    case CB2_MOD_TM:   return 0x8009D978u;
    case CB2_MOD_OMT:  return 0x8009BD38u;
    case CB2_MOD_SBC1: return 0x80090A00u;
    case CB2_MOD_SBC2: return 0x8009B590u;
    case CB2_MOD_COMB: return 0x800A5B10u;
    default:           return 0u;
    }
}

/* Each module's rider update function (called with the rider in a0). */
static uint32_t rider_update_fn(int module) {
    switch (module) {
    case CB2_MOD_FREE: return 0x80066DF0u;
    case CB2_MOD_VS:   return 0x8006624Cu;
    case CB2_MOD_HP:   return 0x80065104u;
    case CB2_MOD_BP:   return 0x80064F38u;
    case CB2_MOD_TM:   return 0x80064FCCu;
    case CB2_MOD_OMT:  return 0x80064FF8u;
    case CB2_MOD_SBC1: return 0x80065290u;
    case CB2_MOD_SBC2: return 0x80066FB0u;
    case CB2_MOD_COMB: return 0x80065F5Cu;
    default:           return 0u;
    }
}

/* Host-only presentation state (emulation thread). */
static uint32_t s_vblank;              /* VBlanks since activation */
static uint32_t s_updated[2];          /* s_vblank when each rider was last updated */
static uint32_t s_crouched[2];         /* s_vblank when each rider last crouched */

/* One sample per rider, packed so the render thread reads it in one load:
 * bit 0 visible, bit 1 crouching, bits 16-31 spin, bits 32-47 flip. */
static _Atomic uint64_t s_sample[2];
static _Atomic int s_riders;          /* 1, or 2 in the two-player module */
static _Atomic int s_at_top;          /* option: draw near the top instead */
static _Atomic int s_size_pct = 100;  /* option: 75 / 100 / 150 */

static int32_t clamp16(int32_t v) { return v > 32767 ? 32767 : v < -32767 ? -32767 : v; }

static uint64_t sample_rider(int i, uint32_t rider) {
    /* A rider is a heap object in main RAM; anything else is not one. */
    if (rider < 0x80010000u || rider >= 0x80200000u - 0x800u || (rider & 3u)) return 0u;
    if (s_vblank - s_updated[i] > STALE_VBLANKS) return 0u;
    int32_t spin = (int32_t)psx_mod_read_word(rider + RIDER_SPIN);
    int32_t flip = (int32_t)psx_mod_read_word(rider + RIDER_FLIP);
    int crouch = (int16_t)psx_mod_read_half(rider + RIDER_STATE) == STATE_CROUCH;
    if (crouch) s_crouched[i] = s_vblank;
    if (!crouch && (spin == 0 && flip == 0)) return 0u;
    if (!crouch && s_vblank - s_crouched[i] > HOLD_VBLANKS) return 0u;
    if (spin > 100000 || spin < -100000 || flip > 100000 || flip < -100000) return 0u;
    return 1u | (crouch ? 2u : 0u) |
           ((uint64_t)(uint16_t)clamp16(spin) << 16) |
           ((uint64_t)(uint16_t)clamp16(flip) << 32);
}

static void meter_vblank(void) {
    s_vblank++;
    int module = cb2_resident_module();
    uint32_t table = rider_table(module);
    int riders = !table ? 0 : (module == CB2_MOD_VS ? 2 : 1);
    for (int i = 0; i < 2; i++)
        atomic_store(&s_sample[i],
                     i < riders ? sample_rider(i, psx_mod_read_word(table + (uint32_t)i * 4u)) : 0u);
    atomic_store(&s_riders, riders);
}

/* ---- drawing (render thread) -------------------------------------------- */

#if defined(_WIN32)
#define CB2_GLAPI __stdcall
#else
#define CB2_GLAPI
#endif
#define CB2_GL_SCISSOR_BOX        0x0C10
#define CB2_GL_SCISSOR_TEST       0x0C11
#define CB2_GL_COLOR_CLEAR_VALUE  0x0C22
#define CB2_GL_COLOR_WRITEMASK    0x0C23
#define CB2_GL_COLOR_BUFFER_BIT   0x00004000
unsigned char CB2_GLAPI glIsEnabled(unsigned int cap);
void CB2_GLAPI glEnable(unsigned int cap);
void CB2_GLAPI glDisable(unsigned int cap);
void CB2_GLAPI glGetIntegerv(unsigned int name, int* out);
void CB2_GLAPI glGetFloatv(unsigned int name, float* out);
void CB2_GLAPI glGetBooleanv(unsigned int name, unsigned char* out);
void CB2_GLAPI glScissor(int x, int y, int w, int h);
void CB2_GLAPI glClearColor(float r, float g, float b, float a);
void CB2_GLAPI glColorMask(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void CB2_GLAPI glClear(unsigned int mask);

typedef struct { float r, g, b; } Rgb;
static int s_fb_h;      /* drawable height, for the bottom-left GL origin */

/* Filled rectangle in window pixels, y down from the top. */
static void rect(float x, float y, float w, float h, Rgb c) {
    int x0 = (int)(x + 0.5f), y0 = (int)(y + 0.5f);
    int x1 = (int)(x + w + 0.5f), y1 = (int)(y + h + 0.5f);
    if (x1 <= x0 || y1 <= y0) return;
    glScissor(x0, s_fb_h - y1, x1 - x0, y1 - y0);
    glClearColor(c.r, c.g, c.b, 1.0f);
    glClear(CB2_GL_COLOR_BUFFER_BIT);
}

static unsigned isqrt(unsigned v) {
    unsigned r = 0;
    while ((r + 1u) * (r + 1u) <= v) r++;
    return r;
}

static Rgb cell_colour(int cell) {            /* 0 nearest the hub */
    static const Rgb k[3] = { {0.20f, 0.85f, 0.35f}, {0.98f, 0.82f, 0.15f}, {0.98f, 0.35f, 0.15f} };
    return k[cell < 3 ? 0 : cell < 7 ? 1 : 2];
}
static Rgb scale(Rgb c, float f) { Rgb o = { c.r * f, c.g * f, c.b * f }; return o; }

/* One arm of the cross. (cx, cy) is the hub centre; (dx, dy) the unit direction
 * the arm grows in. `count` is the wind-up in this direction (0 if the other
 * way), `is_flip` selects the flip's square-root response. */
static void arm(float cx, float cy, int dx, int dy, float cell, float thick, float gap,
                int count, int is_flip) {
    static const Rgb back = {0.03f, 0.03f, 0.05f}, empty = {0.20f, 0.21f, 0.25f},
                     over = {1.0f, 1.0f, 1.0f};
    float len = cell * CELLS;
    float half = thick * 0.5f;
    /* Backing strip, one gap larger than the cells all round. */
    float bx = dx > 0 ? cx + half : dx < 0 ? cx - half - len - gap : cx - half - gap;
    float by = dy > 0 ? cy + half : dy < 0 ? cy - half - len - gap : cy - half - gap;
    rect(bx, by, dx ? len + gap : thick + 2 * gap, dy ? len + gap : thick + 2 * gap, back);

    /* Fill fractions of the whole arm, 0..1. */
    float lit = 0, dim = 0, hot = 0;
    if (count > 0) {
        int floor_count = count < COUNT_MIN_LAUNCH ? COUNT_MIN_LAUNCH : count;
        if (is_flip) {
            lit = (float)isqrt((unsigned)count * 100u) / 100.0f;        /* sqrt(count)/10 */
            dim = (float)isqrt((unsigned)floor_count * 100u) / 100.0f;
        } else {
            lit = (float)count / COUNT_FULL;
            dim = (float)floor_count / COUNT_FULL;
        }
        if (count > COUNT_FULL)                                          /* sqrt growth */
            hot = ((float)isqrt((unsigned)count * 100u) / 10.0f - 10.0f) / 10.0f;
        if (lit > 1) lit = 1;
        if (dim > 1) dim = 1;
        if (hot > 1) hot = 1;
    }
    for (int i = 0; i < CELLS; i++) {
        float a = (float)i / CELLS, b = (float)(i + 1) / CELLS;   /* this cell's span */
        float start = half + cell * (float)i + gap;                     /* distance from hub */
        float size = cell - gap;
        Rgb colour = cell_colour(i);
        for (int pass = 0; pass < 4; pass++) {
            float f = pass == 0 ? 1.0f : pass == 1 ? dim : pass == 2 ? lit : hot;
            Rgb c = pass == 0 ? empty : pass == 1 ? scale(colour, 0.40f) : pass == 2 ? colour : over;
            float part = f >= b ? 1.0f : f <= a ? 0.0f : (f - a) * CELLS;
            if (part <= 0.0f) continue;
            float s = size * part;
            float x = dx > 0 ? cx + start : dx < 0 ? cx - start - s : cx - half;
            float y = dy > 0 ? cy + start : dy < 0 ? cy - start - s : cy - half;
            rect(x, y, dx ? s : thick, dy ? s : thick, c);
        }
    }
}

static void gauge(float cx, float cy, float unit, uint64_t sample) {
    static const Rgb back = {0.03f, 0.03f, 0.05f}, hub_idle = {0.55f, 0.57f, 0.62f},
                     hub_full = {1.0f, 0.95f, 0.60f};
    int spin = (int16_t)(sample >> 16), flip = (int16_t)(sample >> 32);
    float cell = 2.0f * unit, thick = 4.0f * unit;
    float gap = unit * 0.3f < 1.0f ? 1.0f : unit * 0.3f;
    int full = spin >= COUNT_FULL || spin <= -COUNT_FULL || flip >= COUNT_FULL || flip <= -COUNT_FULL;
    arm(cx, cy, +1, 0, cell, thick, gap, spin > 0 ? spin : 0, 0);
    arm(cx, cy, -1, 0, cell, thick, gap, spin < 0 ? -spin : 0, 0);
    arm(cx, cy, 0, -1, cell, thick, gap, flip > 0 ? flip : 0, 1);
    arm(cx, cy, 0, +1, cell, thick, gap, flip < 0 ? -flip : 0, 1);
    rect(cx - thick * 0.5f - gap, cy - thick * 0.5f - gap, thick + 2 * gap, thick + 2 * gap, back);
    rect(cx - thick * 0.5f, cy - thick * 0.5f, thick, thick, full ? hub_full : hub_idle);
}

static void meter_draw(int width, int height, void* ctx) {
    (void)ctx;
    int riders = atomic_load(&s_riders);
    uint64_t sample[2] = { atomic_load(&s_sample[0]), atomic_load(&s_sample[1]) };
    if (riders < 1 || !((sample[0] | sample[1]) & 1u)) return;

    unsigned char was_on = glIsEnabled(CB2_GL_SCISSOR_TEST), mask[4] = {1, 1, 1, 1};
    int box[4] = {0, 0, width, height};
    float clear[4] = {0, 0, 0, 0};
    glGetIntegerv(CB2_GL_SCISSOR_BOX, box);
    glGetFloatv(CB2_GL_COLOR_CLEAR_VALUE, clear);
    glGetBooleanv(CB2_GL_COLOR_WRITEMASK, mask);
    glEnable(CB2_GL_SCISSOR_TEST);
    glColorMask(1, 1, 1, 1);
    s_fb_h = height;

    float size = (float)atomic_load(&s_size_pct) / 100.0f;
    int top = atomic_load(&s_at_top);
    for (int i = 0; i < riders; i++) {
        if (!(sample[i] & 1u)) continue;
        /* Two riders share the screen, one above the other. */
        float view_y = riders == 2 ? (float)height * 0.5f * (float)i : 0.0f;
        float view_h = riders == 2 ? (float)height * 0.5f : (float)height;
        float unit = view_h / 240.0f * size * (riders == 2 ? 1.4f : 1.0f);
        float reach = (CELLS * 2.0f + 2.0f) * unit + 2.0f;     /* hub centre to arm tip */
        float cy = top ? view_y + reach + view_h * 0.04f : view_y + view_h * 0.96f - reach;
        gauge((float)width * 0.5f, cy, unit, sample[i]);
    }

    glColorMask(mask[0], mask[1], mask[2], mask[3]);
    glClearColor(clear[0], clear[1], clear[2], clear[3]);
    glScissor(box[0], box[1], box[2], box[3]);
    if (!was_on) glDisable(CB2_GL_SCISSOR_TEST);
}

/* ---- plugin wiring ------------------------------------------------------ */

/* $a0 on entry. CPUState (runtime cpu_state.h) begins with uint32_t gpr[32];
 * reading it through that prefix avoids pulling the whole CPU header in. */
static uint32_t cpu_rider_arg(struct CPUState* cpu) { return ((const uint32_t*)(const void*)cpu)[4]; }

/* Rider update entry: note which rider the game is updating, then let the
 * original run. Every module has other code at the other modules' addresses,
 * so each hook checks that its own module is the one resident. */
static void note_update(struct CPUState* cpu, int module) {
    if (cb2_resident_module() != module) return;
    uint32_t table = rider_table(module), rider = cpu_rider_arg(cpu);
    for (int i = 0; i < 2; i++)
        if (psx_mod_read_word(table + (uint32_t)i * 4u) == rider) s_updated[i] = s_vblank;
}
#define CB2_UPDATE_HOOK(M) \
    static int hook_##M(struct CPUState* cpu) { note_update(cpu, CB2_MOD_##M); return 0; }
CB2_UPDATE_HOOK(FREE) CB2_UPDATE_HOOK(VS) CB2_UPDATE_HOOK(HP) CB2_UPDATE_HOOK(BP)
CB2_UPDATE_HOOK(TM) CB2_UPDATE_HOOK(OMT) CB2_UPDATE_HOOK(SBC1) CB2_UPDATE_HOOK(SBC2)
CB2_UPDATE_HOOK(COMB)

static void read_options(void) {
    char v[16];
    int top = 0, size = 100;
    if (psx_mod_current_option_value("position", v, sizeof v)) top = strcmp(v, "top") == 0;
    if (psx_mod_current_option_value("size", v, sizeof v))
        size = strcmp(v, "small") == 0 ? 75 : strcmp(v, "large") == 0 ? 150 : 100;
    atomic_store(&s_at_top, top);
    atomic_store(&s_size_pct, size);
}

static void meter_activate(void) {
    atomic_store(&s_sample[0], 0u);
    atomic_store(&s_sample[1], 0u);
    atomic_store(&s_riders, 0);
    s_vblank = 1000u;                       /* nothing counts as recently updated */
    s_updated[0] = s_updated[1] = s_crouched[0] = s_crouched[1] = 0u;
    read_options();
    /* The HUD layer is session-scoped: the runtime clears it before every
     * activation and when the renderer shuts down. */
    psx_host_set_hud_draw_cb(meter_draw, NULL);
}

static int meter_option_changed(const char* option_id, const char* value) {
    (void)option_id; (void)value;
    read_options();
    return 1;
}

PSX_MOD_CONSTRUCTOR(register_cb2_charge_meter) {
    (void)psx_mod_register_activation_plugin("cb2.charge-meter", meter_activate);
    (void)psx_mod_register_vblank_plugin("cb2.charge-meter", meter_vblank);
    (void)psx_mod_register_option_changed_plugin("cb2.charge-meter", meter_option_changed);
    /* Pre-hooks that always decline: the original runs and charges its own time. */
    static const struct { int module; PSXModFunctionOverrideFn fn; } hooks[] = {
        { CB2_MOD_FREE, hook_FREE }, { CB2_MOD_VS, hook_VS },     { CB2_MOD_HP, hook_HP },
        { CB2_MOD_BP, hook_BP },     { CB2_MOD_TM, hook_TM },     { CB2_MOD_OMT, hook_OMT },
        { CB2_MOD_SBC1, hook_SBC1 }, { CB2_MOD_SBC2, hook_SBC2 }, { CB2_MOD_COMB, hook_COMB },
    };
    for (unsigned i = 0; i < sizeof hooks / sizeof hooks[0]; i++)
        (void)psx_mod_register_function_override("cb2.charge-meter:rider-update",
            rider_update_fn(hooks[i].module), hooks[i].fn, NULL, 0, 0);
}
