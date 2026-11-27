/* G3C: Menu > Genesis — config, preview, and production Apply (G3A adapter).
 * Mirrors LFO/Slicer panel lifecycle. MERGE deferred. */
#include "genesis_ui.h"
#include "genesis_adapt.h"
#include "genesis_evo.h"
#include "slice.h"
#include "dlw.h"
#include <stddef.h>

#ifdef CFW_MOD

extern void *fw_operator_new(u32 size);
extern volatile u32 fw_millis;
extern void fw_toast_show(void *toast, const char *msg);

#define GN_MAGIC 0x47454e31u /* "GEN1" */
#define BTN_TOP 0
#define BTN_BOT 3
#define BTN_CLICK 6
#define BTN_BACK 7
#define SELECT_COL 19
#define REDRAW_MS 40u
#define GEN_TPS 480u
#define MAX_EV 64
#define PAGE_STEPS 16 /* grid page; not the destination length */
#define MAX_DEST 64   /* verified native Track Length ceiling (knob 10) */
#define PANEL_X 0
#define PANEL_Y 24
#define PANEL_W 320
#define PANEL_H 192

enum { ST_CFG = 0, ST_PREV = 1, ST_ERR = 2, ST_CONFIRM = 3 };
enum { LAB_MEL = 0, LAB_EUC = 1, LAB_ARP = 2, LAB_EVO = 3, LAB_N = 4 };
enum { ALG_UNI = 0, ALG_WALK = 1 };
enum {
    P_LAB = 0, P_TRK, P_LEN, P_SEED, P_ROOT, P_SCALE, P_ALG, P_DENS, P_PMIN, P_PMAX,
    P_VMIN, P_VMAX, P_GATE, P_INTV, P_RPT, P_PULSES, P_ROT, P_MODE, P_OCT, P_PRESET,
    P_EMODE, P_ELEN, P_EVOL, P_LOCK, P_STYLE, P_MOCT, P_MPITCH, P_MDISP, P_MVEL, P_MGATE,
    P_MINS, P_MREM, P_MREV, P_MROT, P_MOCTV, P_MQUANT, P_MINT, P_MPROT, P_MSTEP, P_MCLEAR,
    P_RESET, P_N
};

struct genesis_ui {
    u32 magic;
    u8 st, lab, param, track, len, dens, gate;
    u8 root, scale, alg, vmin, vmax, pmin, pmax;
    u8 max_intv, rpt, pulses, rot, mode, oct, preset;
    u8 valid_trk, dirty, lit, recovery;
    u8 n_ev, applied, confirm_replace, diag_fail_at; /* diag_fail_at: harness poke only */
    u8 page, native_len;
    u16 seed;
    u8 fp[MAX_DEST]; /* occupancy at preview time, one byte per step */
    u32 pattern_id;
    u32 key, drawn_ms;
    void *scr;
    gen_preview prev;
    gen_event events[MAX_EV]; /* exact preview events for Apply (no silent regen) */
    /* Evolution Lab state is appended. Earlier offsets stay put for the harness. */
    gen_event evo_src[MAX_EV];
    u8 evo_mode, reg_len, evolve, lockv, mstyle, moct;
    u8 mpitch, mdisp, mvelp, mgatep, mins, mrem;
    u8 mrev, mrot, moctv, mquant, mint, mprot, mstep;
    u8 pstep[8]; /* bits 0..63: protected native steps */
    u8 src_n;
    gen_evo evo;
};

_Static_assert(offsetof(struct genesis_ui, len) == 8, "genesis len offset");
_Static_assert(offsetof(struct genesis_ui, diag_fail_at) == 32, "genesis fail offset");
_Static_assert(offsetof(struct genesis_ui, page) == 33, "genesis page offset");
_Static_assert(offsetof(struct genesis_ui, native_len) == 34, "genesis native_len offset");
_Static_assert(offsetof(struct genesis_ui, seed) == 36, "genesis seed offset");

static const char *const lab_name[LAB_N] = {"Melody", "Euclid", "Arp", "Evo"};
static const char *const evo_name[] = {"Turing", "Markov", "Mutate"};
static const char *const style_name[] = {"Step", "Motif", "Bass", "Expl", "Learn"};
static const char *const scale_name[] = {
    "Chrom", "Major", "N.min", "H.min", "M.min", "Dor", "Phr", "Lyd",
    "Mixo", "Loc", "MajP", "MinP", "Blues", "Whole"
};
static const char *const note_name[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
static const char *const arp_mode_name[] = {
    "Up", "Down", "UpDn", "DnUp", "Order", "Rand", "Conv", "Div"
};
static const char *const preset_name[] = {"Maj3", "Min3", "7th", "Scale"};
static const u8 led_on[3] = {0x00, 0x40, 0xc0}, led_off[3] = {0, 0, 0};

static int step_prot(const struct genesis_ui *g, unsigned step)
{
    if (step >= 64)
        return 0;
    return (g->pstep[step >> 3] >> (step & 7u)) & 1;
}

static void step_prot_set(struct genesis_ui *g, unsigned step, int on)
{
    u8 bit;
    if (step >= 64)
        return;
    bit = (u8)(1u << (step & 7u));
    if (on)
        g->pstep[step >> 3] |= bit;
    else
        g->pstep[step >> 3] &= (u8)~bit;
}

static void clamp_rot(struct genesis_ui *g)
{
    int lim = g->len > 1 ? (int)g->len - 1 : 0;
    int cur = (int)(int8_t)g->mrot;
    if (cur > lim)
        cur = lim;
    if (cur < -lim)
        cur = -lim;
    g->mrot = (u8)(int8_t)cur;
    if (g->len && g->mstep >= g->len)
        g->mstep = (u8)(g->len - 1);
}

static struct genesis_ui *peek(void)
{
    void **slot = cfw_pages_genesis(0);
    struct genesis_ui *g = slot ? *slot : 0;
    return g && g->magic == GN_MAGIC ? g : 0;
}

static struct genesis_ui *state(void)
{
    void **slot = cfw_pages_genesis(1);
    struct genesis_ui *g;
    u32 i;
    if (!slot)
        return 0;
    g = *slot;
    if (!g || g->magic != GN_MAGIC) {
        g = fw_operator_new(sizeof *g);
        if (!g)
            return 0;
        for (i = 0; i < sizeof *g; i++)
            ((volatile u8 *)g)[i] = 0;
        g->lab = LAB_MEL;
        g->len = 16;
        g->dens = 70;
        g->gate = 100;
        g->root = 60;
        g->scale = (u8)GEN_SCALE_NAT_MINOR;
        g->alg = ALG_UNI;
        g->vmin = 64;
        g->vmax = 110;
        g->pmin = 48;
        g->pmax = 72;
        g->max_intv = 3;
        g->rpt = 20;
        g->pulses = 5;
        g->rot = 0;
        g->mode = (u8)GEN_ARP_UP;
        g->oct = 1;
        g->preset = 0;
        g->seed = 42;
        g->track = 8;
        g->evo_mode = GEN_EVO_TURING;
        g->reg_len = 8;
        g->evolve = 0;
        g->lockv = 0;
        g->mstyle = GEN_MARKOV_STEPWISE;
        g->moct = 1;
        g->mint = 2;
        g->moctv = 2; /* octave shift 0 */
        g->mstep = 0;
        gen_evo_init(&g->evo);
        g->st = ST_CFG;
        g->dirty = 1;
        g->magic = GN_MAGIC;
        *slot = g;
    }
    return g;
}

static void *ctx_of(const struct genesis_ui *g)
{
    return g->scr ? PTR(g->scr, SCR_CTX) : 0;
}

static void say(void *ctx, const char *a, const char *b)
{
    char msg[32], *p = msg;
    if (!ctx)
        return;
    while (a && *a && p < msg + 30)
        *p++ = *a++;
    while (b && *b && p < msg + 30)
        *p++ = *b++;
    *p = 0;
    fw_toast_show((char *)PTR(PTR(ctx, MENU_CTX_CTRL), CTRL_UI) + UI_TOAST, msg);
}

static s32 clampi(s32 v, s32 lo, s32 hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static void invalidate(struct genesis_ui *g)
{
    gen_evo_discard(&g->evo);
    g->prev.valid = 0;
    g->n_ev = 0;
    g->applied = 0;
    g->confirm_replace = 0;
    g->page = 0;
    if (g->st == ST_PREV || g->st == ST_CONFIRM)
        g->st = ST_CFG;
    g->dirty = 1;
}

static u32 pattern_now(void *ctx)
{
    void *app, *pm;
    if (!ctx)
        return 0;
    app = PTR(ctx, MENU_CTX_APP);
    if (!cfw_ptr_ok(app))
        return 0;
    pm = (char *)app + APP_PATMGR;
    return U32(pm, PM_PLAYING);
}

static void capture_fp(void *ctx, int track, int plen, u8 *fp)
{
    gen_native_step st;
    int i;
    if (plen < 0)
        plen = 0;
    if (plen > MAX_DEST)
        plen = MAX_DEST;
    for (i = 0; i < MAX_DEST; i++)
        fp[i] = 0;
    for (i = 0; i < plen; i++) {
        if (genesis_midi_read_step(ctx, track, i, &st) != GEN_MIDI_OK)
            continue;
        fp[i] = st.on ? 1 : 0;
    }
}

static int fp_same(void *ctx, int track, int plen, const u8 *fp)
{
    gen_native_step st;
    int i;
    if (plen < 0 || plen > MAX_DEST)
        return 0;
    for (i = 0; i < plen; i++) {
        if (genesis_midi_read_step(ctx, track, i, &st) != GEN_MIDI_OK)
            return 0;
        if ((st.on ? 1 : 0) != fp[i])
            return 0;
    }
    return 1;
}

static int refresh_track(struct genesis_ui *g)
{
    void *ctx = ctx_of(g);
    int plen, ok;
    if (!ctx)
        return 0;
    ok = genesis_midi_validate_track(ctx, g->track) == GEN_MIDI_OK;
    g->valid_trk = ok ? 1 : 0;
    if (!ok)
        return 0;
    plen = genesis_midi_pattern_length(ctx, g->track);
    if (plen > 0 && plen <= NUM_STEPS && plen <= MAX_DEST) {
        if (g->native_len != (u8)plen) {
            g->native_len = (u8)plen;
            invalidate(g);
        }
        if (g->len > (u8)plen) {
            g->len = (u8)plen;
            invalidate(g);
        }
    } else if (plen > MAX_DEST) {
        g->valid_trk = 0;
        return 0;
    }
    return 1;
}

int cfw_genesis_recovery_active(void)
{
    struct genesis_ui *g = peek();
    return g && g->recovery ? 1 : 0;
}

void cfw_genesis_set_recovery(int on)
{
    struct genesis_ui *g = state();
    if (!g)
        return;
    g->recovery = on ? 1 : 0;
    if (on)
        invalidate(g);
}

/* Preflight before first native write. Returns GEN_MIDI_* / OK. */
static int preflight_apply(struct genesis_ui *g, gen_apply_policy policy)
{
    void *ctx = ctx_of(g);
    int plen, i;
    if (!ctx)
        return GEN_MIDI_ERR_INTERNAL;
    if (g->recovery)
        return GEN_MIDI_ERR_RESTORE;
    if (!g->prev.valid || g->n_ev == 0 || g->n_ev != (u8)g->prev.event_count)
        return GEN_MIDI_ERR_PARAM;
    if (g->applied)
        return GEN_MIDI_ERR_BUSY;
    if ((int)g->track != g->prev.track)
        return GEN_MIDI_ERR_PARAM;
    if (genesis_midi_validate_track(ctx, g->track) != GEN_MIDI_OK)
        return GEN_MIDI_ERR_BAD_TRACK;
    plen = genesis_midi_pattern_length(ctx, g->track);
    if (plen < 1 || plen > MAX_DEST || plen != g->prev.length)
        return GEN_MIDI_ERR_CAPACITY;
    if (g->len > (u8)plen)
        return GEN_MIDI_ERR_CAPACITY;
    if (pattern_now(ctx) != g->pattern_id)
        return GEN_MIDI_ERR_BUSY;
    if (!fp_same(ctx, g->track, plen, g->fp))
        return GEN_MIDI_ERR_BUSY; /* destination mutated since preview */
    for (i = 0; i < (int)g->n_ev; i++) {
        if (g->events[i].note > 127 || g->events[i].velocity < 1 || g->events[i].velocity > 127)
            return GEN_MIDI_ERR_PARAM;
    }
    return genesis_midi_validate_events(ctx, g->track, 0, g->events, g->n_ev, GEN_TPS, policy);
}

static u8 param_max(const struct genesis_ui *g, u8 p)
{
    switch (p) {
    case P_LAB: return LAB_N - 1;
    case P_TRK: return NUM_TRACKS - 1;
    case P_LEN: return g->native_len ? g->native_len : (u8)MAX_DEST;
    case P_SEED: return 255; /* encoder adjusts low byte; click bumps high via regen */
    case P_ROOT: return 127;
    case P_SCALE: return (u8)GEN_SCALE_COUNT - 1;
    case P_ALG: return 1;
    case P_DENS: return 100;
    case P_PMIN:
    case P_PMAX: return 127;
    case P_VMIN:
    case P_VMAX: return 127;
    case P_GATE: return 100;
    case P_INTV: return 12;
    case P_RPT: return 100;
    case P_PULSES: return g->len ? g->len : 16;
    case P_ROT: return g->len ? (u8)(g->len - 1) : 15;
    case P_MODE: return (u8)GEN_ARP_DIVERGE;
    case P_OCT: return 4;
    case P_PRESET: return 3;
    case P_EMODE: return 2;
    case P_ELEN: return 64;
    case P_EVOL: return 100;
    case P_LOCK: return 1;
    case P_STYLE: return 4;
    case P_MOCT: return 1;
    case P_MPITCH:
    case P_MDISP:
    case P_MVEL:
    case P_MGATE:
    case P_MINS:
    case P_MREM: return 100;
    case P_MREV:
    case P_MQUANT:
    case P_RESET: return 1;
    case P_MROT: return g->len > 1 ? (u8)(g->len - 1) : 0;
    case P_MOCTV: return 4;
    case P_MINT: return 24;
    case P_MPROT: return 15;
    case P_MSTEP: return g->len ? (u8)(g->len - 1) : 0;
    case P_MCLEAR: return 1;
    default: return 0;
    }
}

static int param_visible(const struct genesis_ui *g, u8 p)
{
    if (p == P_LAB || p == P_TRK || p == P_LEN || p == P_SEED || p == P_ROOT || p == P_SCALE ||
        p == P_VMIN || p == P_VMAX || p == P_GATE)
        return 1;
    if (g->lab == LAB_MEL)
        return p == P_ALG || p == P_DENS || p == P_PMIN || p == P_PMAX ||
               (g->alg == ALG_WALK && (p == P_INTV || p == P_RPT));
    if (g->lab == LAB_EUC)
        return p == P_PULSES || p == P_ROT || p == P_PMIN || p == P_PMAX || p == P_DENS;
    if (g->lab == LAB_ARP)
        return p == P_MODE || p == P_OCT || p == P_PRESET;
    if (g->lab == LAB_EVO) {
        if (p == P_DENS || p == P_PMIN || p == P_PMAX || p == P_EMODE)
            return g->evo_mode != GEN_EVO_MUTATE || p != P_DENS;
        if (g->evo_mode == GEN_EVO_TURING)
            return p == P_ELEN || p == P_EVOL || p == P_LOCK || p == P_RESET;
        if (g->evo_mode == GEN_EVO_MARKOV)
            return p == P_STYLE || p == P_MOCT;
        return p == P_MPITCH || p == P_MDISP || p == P_MVEL || p == P_MGATE || p == P_MINS ||
               p == P_MREM || p == P_MREV || p == P_MROT || p == P_MOCTV || p == P_MQUANT ||
               p == P_MINT || p == P_MPROT || p == P_MSTEP || p == P_MCLEAR;
    }
    return 0;
}

static u8 *param_ptr(struct genesis_ui *g, u8 p)
{
    switch (p) {
    case P_LAB: return &g->lab;
    case P_TRK: return &g->track;
    case P_LEN: return &g->len;
    case P_SEED: return (u8 *)&g->seed; /* low byte; careful */
    case P_ROOT: return &g->root;
    case P_SCALE: return &g->scale;
    case P_ALG: return &g->alg;
    case P_DENS: return &g->dens;
    case P_PMIN: return &g->pmin;
    case P_PMAX: return &g->pmax;
    case P_VMIN: return &g->vmin;
    case P_VMAX: return &g->vmax;
    case P_GATE: return &g->gate;
    case P_INTV: return &g->max_intv;
    case P_RPT: return &g->rpt;
    case P_PULSES: return &g->pulses;
    case P_ROT: return &g->rot;
    case P_MODE: return &g->mode;
    case P_OCT: return &g->oct;
    case P_PRESET: return &g->preset;
    case P_EMODE: return &g->evo_mode;
    case P_ELEN: return &g->reg_len;
    case P_EVOL: return &g->evolve;
    case P_LOCK: return &g->lockv;
    case P_STYLE: return &g->mstyle;
    case P_MOCT: return &g->moct;
    case P_MPITCH: return &g->mpitch;
    case P_MDISP: return &g->mdisp;
    case P_MVEL: return &g->mvelp;
    case P_MGATE: return &g->mgatep;
    case P_MINS: return &g->mins;
    case P_MREM: return &g->mrem;
    case P_MREV: return &g->mrev;
    case P_MROT: return &g->mrot;
    case P_MOCTV: return &g->moctv;
    case P_MQUANT: return &g->mquant;
    case P_MINT: return &g->mint;
    case P_MPROT: return &g->mprot;
    case P_MSTEP: return &g->mstep;
    default: return 0;
    }
}

static void next_param(struct genesis_ui *g, s32 dir)
{
    u8 p = g->param;
    u32 guard = 0;
    do {
        if (dir >= 0)
            p = (u8)((p + 1) % P_N);
        else
            p = (u8)(p == 0 ? P_N - 1 : p - 1);
        guard++;
    } while (!param_visible(g, p) && guard < P_N + 2);
    g->param = p;
}

static u32 gate_ticks(const struct genesis_ui *g);

static u32 evo_gate(const struct genesis_ui *g)
{
    u32 t = gate_ticks(g);
    if (t < 4)
        t = 4;
    if (t > GEN_TPS)
        t = GEN_TPS;
    return t;
}

static u8 rest_of(const struct genesis_ui *g)
{
    return g->dens >= 100 ? 0 : (u8)(100u - g->dens);
}

static void fill_turing(const struct genesis_ui *g, gen_turing_cfg *c)
{
    u32 i;
    for (i = 0; i < sizeof *c; i++)
        ((u8 *)c)[i] = 0;
    c->gate = evo_gate(g);
    c->ticks_per_step = GEN_TPS;
    c->seed = g->seed ? g->seed : 1;
    c->root = g->root;
    c->pitch_min = g->pmin;
    c->pitch_max = g->pmax;
    c->steps = g->len < 1 ? 1 : g->len;
    c->scale = g->scale;
    c->length = g->reg_len < 1 ? 1 : g->reg_len;
    c->evolve_prob = g->evolve > 100 ? 100 : g->evolve;
    c->rest_prob = rest_of(g);
    c->velocity = g->vmin < 1 ? 1 : g->vmin;
    c->locked = g->lockv ? 1 : 0;
}

static uint32_t evo_bind(struct genesis_ui *g)
{
    void *ctx = ctx_of(g);
    uint32_t h = 2166136261u;
    uint32_t v;
    int i;
    v = g->evo_mode;
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->seed;
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->len;
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->native_len;
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->track;
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->root;
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->scale;
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->pmin;
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->pmax;
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->dens;
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->gate;
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->vmin;
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->reg_len | ((uint32_t)g->evolve << 8) | ((uint32_t)g->lockv << 16);
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->mstyle | ((uint32_t)g->moct << 8);
    h = gen_evo_hash(h, &v, sizeof v);
    v = (uint32_t)(int8_t)g->mrot;
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->mprot | ((uint32_t)g->mrev << 8) | ((uint32_t)g->mquant << 16) | ((uint32_t)g->moctv << 24);
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->mpitch | ((uint32_t)g->mdisp << 8) | ((uint32_t)g->mvelp << 16) | ((uint32_t)g->mgatep << 24);
    h = gen_evo_hash(h, &v, sizeof v);
    v = g->mins | ((uint32_t)g->mrem << 8) | ((uint32_t)g->mint << 16);
    h = gen_evo_hash(h, &v, sizeof v);
    h = gen_evo_hash(h, g->pstep, sizeof g->pstep);
    v = pattern_now(ctx);
    h = gen_evo_hash(h, &v, sizeof v);
    if (!ctx || !g->valid_trk)
        return h;
    for (i = 0; i < (int)g->len && i < MAX_DEST; i++) {
        gen_native_step st;
        if (genesis_midi_read_step(ctx, g->track, i, &st) != GEN_MIDI_OK) {
            v = 0xffffffffu;
            h = gen_evo_hash(h, &v, sizeof v);
            continue;
        }
        h = gen_evo_hash(h, &st, sizeof st);
    }
    return h;
}

static int read_source(struct genesis_ui *g, gen_event *ev, u32 cap, u32 *n_out)
{
    void *ctx = ctx_of(g);
    u32 n = 0, i;
    if (!ctx || !ev || !n_out)
        return GEN_ERR_PARAM;
    for (i = 0; i < g->len && i < MAX_EV; i++) {
        gen_native_step st;
        u32 ticks;
        if (genesis_midi_read_step(ctx, g->track, (int)i, &st) != GEN_MIDI_OK)
            return GEN_ERR_PARAM;
        if (!st.on)
            continue;
        if (st.note < 0 || st.note > 127 || st.vel < 1 || st.vel > 127)
            return GEN_ERR_PARAM;
        if (st.dur < 1)
            return GEN_ERR_TIMING;
        ticks = (u32)st.dur * GEN_TPS / 60u;
        if (!gen_native_duration_ok(ticks, GEN_TPS))
            return GEN_ERR_TIMING;
        if (n >= cap)
            return GEN_ERR_CAPACITY;
        ev[n].tick = i * GEN_TPS;
        ev[n].note = (u8)st.note;
        ev[n].velocity = (u8)st.vel;
        ev[n].channel = 0;
        ev[n]._pad = 0;
        ev[n].duration = ticks;
        n++;
    }
    *n_out = n;
    return GEN_OK;
}

static void adjust(struct genesis_ui *g, s32 d)
{
    u8 p = g->param, *ptr, mx;
    u16 seed;
    if (p == P_RESET) {
        gen_turing_cfg c;
        /* Writes the committed register now. Does not write native notes. */
        fill_turing(g, &c);
        if (gen_evo_turing_reinit(&g->evo, &c) != GEN_OK)
            say(ctx_of(g), "Reset failed", "");
        else
            say(ctx_of(g), "Register reset", "");
        invalidate(g);
        return;
    }
    if (p == P_MCLEAR) {
        u32 i;
        for (i = 0; i < sizeof g->pstep; i++)
            g->pstep[i] = 0;
        say(ctx_of(g), "Steps clear", "");
        invalidate(g);
        return;
    }
    if (p == P_MROT) {
        int lim = g->len > 1 ? (int)g->len - 1 : 0;
        int cur = (int)(int8_t)g->mrot + d;
        if (cur > lim)
            cur = lim;
        if (cur < -lim)
            cur = -lim;
        g->mrot = (u8)(int8_t)cur;
        invalidate(g);
        return;
    }
    if (p == P_MSTEP) {
        int lim = g->len ? (int)g->len - 1 : 0;
        u8 pg;
        g->mstep = (u8)clampi((int)g->mstep + d, 0, lim);
        pg = (u8)(g->mstep / PAGE_STEPS);
        invalidate(g);
        g->page = pg;
        return;
    }
    if (p == P_SEED) {
        seed = (u16)clampi((s32)g->seed + d, 0, 65535);
        if (seed != g->seed) {
            g->seed = seed;
            invalidate(g);
        }
        return;
    }
    if (p == P_LEN)
        refresh_track(g);
    ptr = param_ptr(g, p);
    if (!ptr)
        return;
    mx = param_max(g, p);
    if (p == P_LEN || p == P_ELEN)
        *ptr = (u8)clampi((s32)*ptr + d * 1, 1, mx < 1 ? 1 : mx);
    else if (p == P_PULSES)
        *ptr = (u8)clampi((s32)*ptr + d, 0, g->len);
    else
        *ptr = (u8)clampi((s32)*ptr + d, 0, mx);
    if (p == P_PMIN && g->pmin > g->pmax)
        g->pmax = g->pmin;
    if (p == P_PMAX && g->pmax < g->pmin)
        g->pmin = g->pmax;
    if (p == P_VMIN && g->vmin > g->vmax)
        g->vmax = g->vmin;
    if (p == P_VMAX && g->vmax < g->vmin)
        g->vmin = g->vmax;
    if (p == P_VMIN && g->vmin < 1)
        g->vmin = 1;
    if (p == P_LEN)
        clamp_rot(g);
    if (p == P_TRK || p == P_LAB || p == P_LEN)
        refresh_track(g);
    if (p == P_EMODE)
        gen_evo_set_mode(&g->evo, g->evo_mode);
    invalidate(g);
}

static u32 gate_ticks(const struct genesis_ui *g)
{
    u32 t = (GEN_TPS * (u32)g->gate + 50u) / 100u;
    return t < 1 ? 1 : t;
}

static void build_arp_notes(const struct genesis_ui *g, u8 *notes, u8 *n_out)
{
    u8 n = 0, i, deg, nd;
    int midi;
    if (g->preset == 0) { /* major triad */
        notes[n++] = g->root;
        notes[n++] = (u8)gen_clamp_midi(g->root + 4);
        notes[n++] = (u8)gen_clamp_midi(g->root + 7);
    } else if (g->preset == 1) {
        notes[n++] = g->root;
        notes[n++] = (u8)gen_clamp_midi(g->root + 3);
        notes[n++] = (u8)gen_clamp_midi(g->root + 7);
    } else if (g->preset == 2) {
        notes[n++] = g->root;
        notes[n++] = (u8)gen_clamp_midi(g->root + 4);
        notes[n++] = (u8)gen_clamp_midi(g->root + 7);
        notes[n++] = (u8)gen_clamp_midi(g->root + 10);
    } else {
        nd = (u8)gen_scale_degree_count((gen_scale)g->scale);
        if (nd < 1)
            nd = 1;
        for (i = 0; i < nd && n < 8; i++) {
            midi = gen_scale_degree_to_midi((gen_scale)g->scale, g->root, i, 0);
            notes[n++] = (u8)gen_clamp_midi(midi);
        }
    }
    *n_out = n;
    (void)deg;
}

static void say_evo(void *ctx, gen_status st)
{
    if (st == GEN_ERR_EMPTY)
        say(ctx, "No source notes", "");
    else if (st == GEN_ERR_UNSUPPORTED)
        say(ctx, "Rev/rot blocked", "");
    else if (st == GEN_ERR_CAPACITY)
        say(ctx, "Too long", "");
    else if (st == GEN_ERR_TIMING)
        say(ctx, "Bad timing", "");
    else if (st == GEN_ERR_PITCH)
        say(ctx, "Bad pitch", "");
    else if (st == GEN_ERR_MONO)
        say(ctx, "Not mono", "");
    else if (st == GEN_ERR_PARAM)
        say(ctx, "Bad param", "");
    else
        say(ctx, "Generate failed", "");
}

static gen_status generate_evo(struct genesis_ui *g, gen_buf *buf)
{
    gen_turing_cfg tc;
    gen_markov_cfg mc;
    gen_mutate_req rq;
    gen_buf src;
    uint32_t n = 0, bind, i;
    gen_status st;
    u32 gmax, gmin;
    gen_evo_discard(&g->evo);
    g->src_n = 0;
    bind = evo_bind(g);
    if (g->len < 1 || g->len > GEN_EVO_NATIVE_MAX)
        return GEN_ERR_PARAM;
    if (g->evo_mode == GEN_EVO_TURING) {
        fill_turing(g, &tc);
        return gen_evo_turing_preview(&g->evo, &tc, buf, bind);
    }
    if (g->evo_mode == GEN_EVO_MARKOV) {
        if (g->mstyle == GEN_MARKOV_LEARNED) {
            st = (gen_status)read_source(g, g->evo_src, MAX_EV, &n);
            if (st != GEN_OK)
                return st;
            src.events = g->evo_src;
            src.count = n;
            src.capacity = MAX_EV;
            st = gen_evo_markov_learn(&g->evo, g->scale, g->root, &src, 0);
            if (st != GEN_OK)
                return st;
        }
        for (i = 0; i < sizeof mc; i++)
            ((u8 *)&mc)[i] = 0;
        mc.gate = evo_gate(g);
        mc.ticks_per_step = GEN_TPS;
        mc.seed = g->seed ? g->seed : 1;
        mc.root = g->root;
        mc.pitch_min = g->pmin;
        mc.pitch_max = g->pmax;
        mc.initial_degree = 0;
        mc.steps = g->len;
        mc.scale = g->scale;
        mc.velocity = g->vmin < 1 ? 1 : g->vmin;
        mc.rest_prob = rest_of(g);
        mc.octave_motion = g->moct ? 1 : 0;
        return gen_evo_markov_preview(&g->evo, &mc, g->mstyle, buf, bind);
    }
    st = (gen_status)read_source(g, g->evo_src, MAX_EV, &n);
    if (st != GEN_OK)
        return st;
    g->src_n = (u8)n;
    src.events = g->evo_src;
    src.count = n;
    src.capacity = MAX_EV;
    for (i = 0; i < sizeof rq; i++)
        ((u8 *)&rq)[i] = 0;
    gmax = evo_gate(g);
    gmin = gmax / 2u;
    if (gmin < 4)
        gmin = 4;
    if (gmin > gmax)
        gmin = gmax;
    rq.ticks_per_step = GEN_TPS;
    rq.gate_min = gmin;
    rq.gate_max = gmax;
    rq.seed = g->seed ? g->seed : 1;
    rq.root = g->root;
    rq.pitch_min = g->pmin;
    rq.pitch_max = g->pmax;
    rq.dest_steps = g->len;
    rq.octave = (int8_t)((int)g->moctv - 2);
    clamp_rot(g);
    rq.rotate_steps = (int8_t)g->mrot;
    rq.scale = g->scale;
    rq.pitch_prob = g->mpitch;
    rq.quantize = g->mquant ? 1 : 0;
    rq.displace_prob = g->mdisp;
    rq.insert_prob = g->mins;
    rq.remove_prob = g->mrem;
    rq.velocity_prob = g->mvelp;
    rq.gate_prob = g->mgatep;
    rq.reverse = g->mrev ? 1 : 0;
    rq.intensity = g->mint > 24 ? 24 : g->mint;
    rq.protect = (uint8_t)(g->mprot & 0x0f);
    rq.vel_min = g->vmin < 1 ? 1 : g->vmin;
    rq.vel_max = g->vmax < rq.vel_min ? rq.vel_min : g->vmax;
    for (i = 0; i < g->len && i < 64u; i++) {
        if (step_prot(g, i))
            gen_mutate_protect_step(&rq, i, 1);
    }
    return gen_evo_mutate_preview(&g->evo, &src, &rq, buf, bind);
}

static int do_generate(struct genesis_ui *g)
{
    void *ctx = ctx_of(g);
    gen_event storage[MAX_EV];
    gen_buf buf;
    gen_rhythm rh;
    gen_melody_cfg mc;
    gen_arp_cfg ac;
    gen_rng rng;
    gen_status st = GEN_ERR_PARAM;
    u8 notes[8], nn = 0;
    u32 i;

    if (!ctx)
        return GEN_MIDI_ERR_INTERNAL;
    if (!refresh_track(g)) {
        g->st = ST_ERR;
        say(ctx, "Need MIDI track", "");
        return GEN_MIDI_ERR_BAD_TRACK;
    }
    if (g->len < 1)
        g->len = 16;
    if (g->pmin > g->pmax || g->vmin > g->vmax || g->vmin < 1) {
        g->st = ST_ERR;
        say(ctx, "Bad pitch/vel", "");
        return GEN_MIDI_ERR_PARAM;
    }

    gen_buf_init(&buf, storage, MAX_EV);
    for (i = 0; i < sizeof mc; i++)
        ((u8 *)&mc)[i] = 0;
    for (i = 0; i < sizeof ac; i++)
        ((u8 *)&ac)[i] = 0;

    if (g->lab == LAB_MEL) {
        gen_rng_seed(&rng, g->seed);
        st = gen_rhythm_probability(&rh, g->len, (uint16_t)GEN_TPS, g->dens, &rng);
        if (st != GEN_OK)
            goto fail;
        mc.root = g->root;
        mc.scale = (gen_scale)g->scale;
        mc.pitch_min = g->pmin;
        mc.pitch_max = g->pmax;
        mc.vel_min = g->vmin;
        mc.vel_max = g->vmax;
        mc.duration = gate_ticks(g);
        mc.channel = 0;
        mc.seed = g->seed;
        mc.max_interval = g->max_intv;
        mc.repeat_prob = g->rpt;
        st = g->alg == ALG_WALK ? gen_melody_walk(&mc, &rh, &buf)
                                : gen_melody_uniform(&mc, &rh, &buf);
    } else if (g->lab == LAB_EUC) {
        if (g->pulses > g->len)
            g->pulses = g->len;
        st = gen_rhythm_euclidean(&rh, g->len, g->pulses, g->rot, (uint16_t)GEN_TPS);
        if (st != GEN_OK)
            goto fail;
        mc.root = g->root;
        mc.scale = (gen_scale)g->scale;
        mc.pitch_min = g->pmin;
        mc.pitch_max = g->pmax;
        mc.vel_min = g->vmin;
        mc.vel_max = g->vmax;
        mc.duration = gate_ticks(g);
        mc.channel = 0;
        mc.seed = g->seed;
        st = gen_melody_uniform(&mc, &rh, &buf);
    } else if (g->lab == LAB_ARP) {
        build_arp_notes(g, notes, &nn);
        if (nn == 0)
            goto fail;
        ac.mode = (gen_arp_mode)g->mode;
        ac.rate = GEN_TPS;
        ac.gate = gate_ticks(g);
        ac.octaves = g->oct < 1 ? 1 : g->oct;
        ac.length = g->len;
        ac.velocity = g->vmin;
        ac.channel = 0;
        ac.seed = g->seed;
        st = gen_arp(&ac, notes, nn, &buf);
    } else {
        st = generate_evo(g, &buf);
    }
    if (st != GEN_OK || gen_buf_validate(&buf, 1) != GEN_OK)
        goto fail;
    if (buf.count == 0 || buf.count > MAX_EV)
        goto fail;

    if (genesis_midi_preview_build(ctx, g->track, 0, buf.events, buf.count, GEN_TPS, g->seed,
                                   (u8)(g->lab + 1), GEN_APPLY_EMPTY_ONLY, &g->prev) != GEN_MIDI_OK)
        goto fail;
    if (g->prev.length < 1 || g->prev.length > MAX_DEST)
        goto fail;
    /* Keep exact events for Apply — never silently regenerate or drop. */
    g->n_ev = 0;
    for (i = 0; i < buf.count; i++) {
        g->events[i] = buf.events[i];
        g->n_ev++;
    }
    if (g->n_ev != buf.count)
        goto fail;
    g->pattern_id = pattern_now(ctx);
    capture_fp(ctx, g->track, g->prev.length, g->fp);
    g->page = 0;
    g->applied = 0;
    g->confirm_replace = 0;
    g->st = ST_PREV;
    g->dirty = 1;
    if (g->lab == LAB_EVO && g->evo_mode == GEN_EVO_MUTATE && g->n_ev == g->src_n && g->src_n) {
        u32 same = 1;
        for (i = 0; i < g->n_ev; i++) {
            if (g->events[i].tick != g->evo_src[i].tick || g->events[i].note != g->evo_src[i].note ||
                g->events[i].velocity != g->evo_src[i].velocity ||
                g->events[i].duration != g->evo_src[i].duration)
                same = 0;
        }
        if (same) {
            say(ctx, "No note change", "");
            return GEN_MIDI_OK;
        }
    }
    say(ctx, "Preview ", lab_name[g->lab]);
    return GEN_MIDI_OK;
fail:
    gen_evo_discard(&g->evo);
    g->prev.valid = 0;
    g->n_ev = 0;
    g->st = ST_ERR;
    if (g->lab == LAB_EVO)
        say_evo(ctx, st);
    else
        say(ctx, "Generate failed", "");
    return GEN_MIDI_ERR_INTERNAL;
}

static void regen_seed(struct genesis_ui *g)
{
    /* Mix displayed seed with millis — documented Regenerate policy. */
    g->seed = (u16)(g->seed * 33u + (u16)fw_millis + 0x9e37u);
    if (g->seed == 0)
        g->seed = 1;
    invalidate(g);
}

static void toast_apply_err(void *ctx, int rc)
{
    if (rc == GEN_MIDI_ERR_OCCUPIED)
        say(ctx, "Region occupied", "");
    else if (rc == GEN_MIDI_ERR_RESTORE)
        say(ctx, "Recovery required", "");
    else if (rc == GEN_MIDI_ERR_BAD_TRACK)
        say(ctx, "Need MIDI track", "");
    else if (rc == GEN_MIDI_ERR_BUSY)
        say(ctx, "Stale preview", "");
    else if (rc == GEN_MIDI_ERR_CAPACITY)
        say(ctx, "Bad length", "");
    else if (rc == GEN_MIDI_ERR_PARAM)
        say(ctx, "No preview", "");
    else
        say(ctx, "Apply failed", "");
}

static int do_apply(struct genesis_ui *g, gen_apply_policy policy)
{
    void *ctx = ctx_of(g);
    gen_snap snap;
    char msg[28], *p;
    int rc, n;
    u32 i;

    if (!ctx)
        return GEN_MIDI_ERR_INTERNAL;
    if (g->lab == LAB_EVO) {
        gen_evo_set_live(&g->evo, evo_bind(g));
        if (g->evo.phase != GEN_EVO_VALID) {
            say(ctx, "Stale preview", "");
            invalidate(g);
            return GEN_MIDI_ERR_BUSY;
        }
    }
    if (g->recovery) {
        say(ctx, "Recovery required", "");
        return GEN_MIDI_ERR_RESTORE;
    }
    rc = preflight_apply(g, policy);
    if (rc != GEN_MIDI_OK) {
        toast_apply_err(ctx, rc);
        if (rc == GEN_MIDI_ERR_BUSY || rc == GEN_MIDI_ERR_PARAM)
            invalidate(g);
        return rc;
    }

    for (i = 0; i < sizeof snap; i++)
        ((u8 *)&snap)[i] = 0;
    rc = genesis_midi_apply_ex(ctx, g->track, 0, g->events, g->n_ev, GEN_TPS, policy, &snap,
                               (int)g->diag_fail_at);
    g->diag_fail_at = 0; /* one-shot harness poke */

    if (rc == GEN_MIDI_ERR_OCCUPIED && policy == GEN_APPLY_EMPTY_ONLY) {
        /* Offer explicit REPLACE confirmation (G3A-verified restore path). */
        g->confirm_replace = 1;
        g->st = ST_CONFIRM;
        g->dirty = 1;
        say(ctx, "Replace? Apply again", "");
        return GEN_MIDI_ERR_OCCUPIED;
    }
    if (rc == GEN_MIDI_ERR_RESTORE) {
        /* The writer may already have committed. Do not advance Evolution state.
         * invalidate drops the preview and the pending learn. recovery blocks
         * another Apply until Shift+Back re-reads the track. */
        g->recovery = 1;
        invalidate(g);
        g->st = ST_ERR;
        say(ctx, "Pattern may differ", "");
        return rc;
    }
    if (rc != GEN_MIDI_OK) {
        toast_apply_err(ctx, rc);
        g->confirm_replace = 0;
        if (g->st == ST_CONFIRM)
            g->st = ST_PREV;
        g->dirty = 1;
        return rc;
    }

    if (g->lab == LAB_EVO && gen_evo_apply_result(&g->evo, 1) != GEN_OK) {
        say(ctx, "State not saved", "");
        g->applied = 1;
        g->prev.valid = 0;
        g->n_ev = 0;
        g->st = ST_ERR;
        g->dirty = 1;
        return GEN_MIDI_ERR_INTERNAL;
    }
    n = (int)g->n_ev;
    p = msg;
    *p++ = 'A';
    *p++ = 'p';
    *p++ = 'p';
    *p++ = 'l';
    *p++ = 'i';
    *p++ = 'e';
    *p++ = 'd';
    *p++ = ' ';
    if (n >= 10)
        *p++ = (char)('0' + (n / 10) % 10);
    *p++ = (char)('0' + n % 10);
    *p++ = ' ';
    *p++ = 't';
    *p++ = (char)('0' + ((g->track + 1) / 10) % 10);
    *p++ = (char)('0' + (g->track + 1) % 10);
    *p = 0;
    fw_toast_show((char *)PTR(PTR(ctx, MENU_CTX_CTRL), CTRL_UI) + UI_TOAST, msg);

    g->applied = 1;
    g->confirm_replace = 0;
    g->prev.valid = 0;
    g->n_ev = 0;
    g->st = ST_CFG;
    g->dirty = 1;
    return GEN_MIDI_OK;
}

void cfw_menu_build_genesis(void *ctx, void *menu)
{
    (void)ctx;
    (void)menu;
    __asm__ volatile("" : : : "memory");
}

void cfw_genesis_enter(void *scr)
{
    struct genesis_ui *g = state();
    void *ctx;
    u32 mask, t;
    if (!g)
        return;
    ctx = PTR(scr, SCR_CTX);
    if (!g->lit) {
        g->st = ST_CFG;
        g->param = P_LAB;
        g->prev.valid = 0;
        mask = U32((char *)PTR(ctx, MENU_CTX_CTRL) + CTRL_SEL, 0x0c) & 0xffffu;
        g->track = 8; /* prefer verified default MIDI track */
        for (t = 0; t < NUM_TRACKS; t++)
            if ((mask & (1u << t))) {
                g->track = (u8)t;
                if (refresh_track(g))
                    break;
                g->track = 8;
            }
        refresh_track(g);
        if (!g->valid_trk) {
            for (t = 0; t < NUM_TRACKS; t++) {
                g->track = (u8)t;
                if (refresh_track(g))
                    break;
            }
        }
        g->lit = 1;
        invalidate(g);
    }
    g->scr = scr;
}

void cfw_genesis_tick(void *app)
{
    struct genesis_ui *g = peek();
    struct cfw_view *v = &cfw_ram.view;
    void *ctrl;
    if (!g)
        return;
    ctrl = (char *)app + APP_CTRL;
    if (v->magic != VIEW_MAGIC || v->wmode != WMODE_GENESIS || !cfw_view_alive(v)) {
        g->lit = 0;
        g->scr = 0;
        return;
    }
    cfw_lfo_close_if_left(g->scr);
    if (v->wmode != WMODE_GENESIS)
        return;
    if (g->dirty && fw_millis - g->drawn_ms >= REDRAW_MS) {
        g->dirty = 0;
        g->drawn_ms = fw_millis;
        VFN(v->disp, DISP_REFRESH, void, u32)(v->disp, (u32)v->slot);
    }
    (void)ctrl;
}

int cfw_genesis_input(u32 kind, u32 id, u32 val)
{
    struct genesis_ui *g = peek();
    struct cfw_view *v = &cfw_ram.view;
    void *ctx, *mode;
    u32 row, col, track;
    if (!g || v->magic != VIEW_MAGIC || v->wmode != WMODE_GENESIS || !cfw_view_alive(v))
        return 0;
    ctx = ctx_of(g);
    if (!ctx)
        return 0;

    if (kind == 0) { /* pad */
        col = id % LED_COLS;
        row = id / LED_COLS;
        mode = PTR(PTR(ctx, MENU_CTX_CTRL), CTRL_MODE);
        track = row + (cfw_ptr_ok(mode) && U8(mode, MODE_BANK) ? 8u : 0u);
        if (col == SELECT_COL && val == 0) {
            if (track < NUM_TRACKS) {
                g->track = (u8)track;
                invalidate(g);
                if (!refresh_track(g))
                    say(ctx, "Not MIDI track", "");
                else
                    say(ctx, "Track ", "");
                g->dirty = 1;
            }
            return 1;
        }
        /* Step pads are swallowed while Genesis is open. On Mutation config they
         * toggle protection for the current 16-step page. Select column stays track select. */
        if (col < PAD_COLS && val == 0 && !g->recovery && g->st == ST_CFG &&
            g->lab == LAB_EVO && g->evo_mode == GEN_EVO_MUTATE && track == g->track) {
            u32 step = (u32)g->page * PAGE_STEPS + col;
            u8 pg = g->page;
            if (step < g->len) {
                step_prot_set(g, step, !step_prot(g, step));
                invalidate(g);
                g->page = pg;
            }
            g->dirty = 1;
        }
        return 1; /* swallow pads while open */
    }
    if (kind == 1) { /* button */
        if (id == BTN_BACK && val == 0) {
            if (g->recovery && v->shift) {
                /* A track re-read does not restore the pattern, release a native
                 * edit, or prove the step data. Apply stays blocked until the
                 * heap object is gone, which is the next power-on. */
                say(ctx, "Recovery required", "");
                g->dirty = 1;
                return 1;
            }
            if (g->st == ST_CONFIRM) {
                g->confirm_replace = 0;
                g->st = ST_PREV;
                g->dirty = 1;
                say(ctx, "Replace cancelled", "");
                return 1;
            }
            if (g->st == ST_ERR) {
                invalidate(g);
                g->st = ST_CFG;
                g->dirty = 1;
                return 1;
            }
            if (g->st == ST_PREV) {
                invalidate(g);
                return 1;
            }
            return 0; /* stock leave */
        }
        if (id == BTN_TOP && val == 0) {
            if (g->recovery) {
                say(ctx, "Recovery required", "");
                return 1;
            }
            g->lab = (u8)((g->lab + 1) % LAB_N);
            g->param = P_LAB;
            invalidate(g);
            say(ctx, lab_name[g->lab], "");
            g->dirty = 1;
            return 1;
        }
        if (id == BTN_BOT && val == 0) {
            /* Bottom = Apply when preview/confirm; Edit return only via Back. */
            if (g->recovery) {
                say(ctx, "Recovery required", "");
                g->st = ST_ERR;
                g->dirty = 1;
                return 1;
            }
            if (g->st == ST_CONFIRM && g->confirm_replace) {
                do_apply(g, GEN_APPLY_REPLACE_REGION);
                return 1;
            }
            if (g->st == ST_PREV && g->prev.valid && g->n_ev) {
                do_apply(g, GEN_APPLY_EMPTY_ONLY);
                return 1;
            }
            if (g->st == ST_CFG)
                say(ctx, "Generate first", "");
            return 1;
        }
        if (id == BTN_CLICK && val == 0) {
            if (g->recovery) {
                say(ctx, "Recovery required", "");
                return 1;
            }
            if (v->shift) {
                regen_seed(g);
                do_generate(g);
            } else {
                do_generate(g);
            }
            g->dirty = 1;
            return 1;
        }
        return 1;
    }
    if (kind == 4 && (g->st == ST_PREV || g->st == ST_CONFIRM)) {
        s32 d = (s32)val;
        int pages;
        if (v->shift) {
            next_param(g, d > 0 ? 1 : -1);
            invalidate(g);
            g->dirty = 1;
            return 1;
        }
        pages = (int)((g->len + PAGE_STEPS - 1) / PAGE_STEPS);
        if (pages < 1)
            pages = 1;
        if (d > 0 && (int)g->page + 1 < pages)
            g->page++;
        else if (d < 0 && g->page)
            g->page--;
        g->dirty = 1;
        return 1;
    }
    if (kind == 4 && g->st == ST_CFG) { /* encoder: turn=value, Shift+turn=param */
        s32 d = (s32)val;
        if (d == 0)
            return 1;
        if (v->shift)
            next_param(g, d > 0 ? 1 : -1);
        else
            adjust(g, d > 0 ? 1 : -1);
        g->dirty = 1;
        return 1;
    }
    if (kind == 2 || kind == 3)
        return 1; /* swallow knobs */
    return 0;
}

const u8 *cfw_genesis_led(u32 led)
{
    struct genesis_ui *g = peek();
    struct cfw_view *v = &cfw_ram.view;
    void *ctx, *mode;
    u32 row, col, track, step;
    if (!g || v->magic != VIEW_MAGIC || v->wmode != WMODE_GENESIS)
        return 0;
    col = led % LED_COLS;
    row = led / LED_COLS;
    ctx = ctx_of(g);
    mode = ctx ? PTR(PTR(ctx, MENU_CTX_CTRL), CTRL_MODE) : 0;
    track = row + (cfw_ptr_ok(mode) && U8(mode, MODE_BANK) ? 8u : 0u);
    if (col == SELECT_COL) {
        if (track == g->track)
            return led_on;
        return led_off;
    }
    if (col < PAD_COLS && track == g->track && g->prev.valid) {
        step = (u32)g->page * PAGE_STEPS + col;
        if (step < g->len && step < GEN_SNAP_MAX && g->prev.occupancy[step])
            return led_on;
        return led_off;
    }
    if (col < PAD_COLS && track == g->track && g->st == ST_CFG &&
        g->lab == LAB_EVO && g->evo_mode == GEN_EVO_MUTATE) {
        step = (u32)g->page * PAGE_STEPS + col;
        if (step < g->len && step_prot(g, step))
            return led_on;
        return led_off;
    }
    return 0;
}

static void put_u(char *buf, u32 v)
{
    char tmp[8];
    int n = 0, i;
    do {
        tmp[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v && n < 8);
    for (i = 0; i < n; i++)
        buf[i] = tmp[n - 1 - i];
    buf[n] = 0;
}

static void fmt_note(char *buf, u8 midi)
{
    const char *nm = note_name[midi % 12];
    int oct = (int)(midi / 12) - 1, i = 0;
    while (nm[i]) {
        buf[i] = nm[i];
        i++;
    }
    if (oct < 0) {
        buf[i++] = '-';
        buf[i++] = (char)('0' + (-oct));
    } else {
        buf[i++] = (char)('0' + oct);
    }
    buf[i] = 0;
}

static void param_label(const struct genesis_ui *g, u8 p, char *out)
{
    static const char *const names[] = {
        "Lab", "Track", "Length", "Seed", "Root", "Scale", "Algo", "Dens", "P.min", "P.max",
        "V.min", "V.max", "Gate%", "Intv", "Repeat", "Pulses", "Rotate", "Mode", "Oct", "Notes",
        "Engine", "RegLen", "Evolve", "Lock", "Style", "OctMv", "Pit%", "Rhy%", "Vel%", "GtVar",
        "Ins%", "Rem%", "Rev", "RotBy", "OctSh", "Quant", "Amt", "Attr", "Step", "Clr", "Reset"
    };
    const char *s = p < P_N ? names[p] : "?";
    while (*s)
        *out++ = *s++;
    *out = 0;
    (void)g;
}

static void value_str(const struct genesis_ui *g, u8 p, char *out)
{
    const char *s;
    char *o = out;
    u8 v;
    out[0] = 0;
    switch (p) {
    case P_LAB:
        s = lab_name[g->lab];
        while (*s)
            *o++ = *s++;
        *o = 0;
        return;
    case P_SCALE:
        if (g->scale < GEN_SCALE_COUNT) {
            s = scale_name[g->scale];
            while (*s)
                *o++ = *s++;
            *o = 0;
            return;
        }
        break;
    case P_ALG:
        s = g->alg == ALG_WALK ? "Walk" : "Uniform";
        while (*s)
            *o++ = *s++;
        *o = 0;
        return;
    case P_ROOT:
        fmt_note(out, g->root);
        return;
    case P_MODE:
        if (g->mode <= GEN_ARP_DIVERGE) {
            s = arp_mode_name[g->mode];
            while (*s)
                *o++ = *s++;
            *o = 0;
            return;
        }
        break;
    case P_PRESET:
        s = preset_name[g->preset < 4 ? g->preset : 0];
        while (*s)
            *o++ = *s++;
        *o = 0;
        return;
    case P_SEED:
        put_u(out, g->seed);
        return;
    case P_EMODE:
        s = evo_name[g->evo_mode < 3 ? g->evo_mode : 0];
        while (*s)
            *o++ = *s++;
        *o = 0;
        return;
    case P_STYLE:
        s = style_name[g->mstyle < 5 ? g->mstyle : 0];
        while (*s)
            *o++ = *s++;
        *o = 0;
        return;
    case P_LOCK:
    case P_MREV:
    case P_MQUANT:
    case P_MOCT:
        s = (*param_ptr((struct genesis_ui *)g, p)) ? "On" : "Off";
        while (*s)
            *o++ = *s++;
        *o = 0;
        return;
    case P_RESET:
    case P_MCLEAR:
        s = "Now";
        while (*s)
            *o++ = *s++;
        *o = 0;
        return;
    case P_MSTEP:
        put_u(out, g->mstep);
        if (step_prot(g, g->mstep)) {
            while (*o)
                o++;
            *o++ = ' ';
            *o++ = 'P';
            *o = 0;
        }
        return;
    case P_MROT: {
        int cur = (int)(int8_t)g->mrot;
        if (cur < 0) {
            out[0] = '-';
            put_u(out + 1, (u32)(-cur));
        } else {
            put_u(out, (u32)cur);
        }
        return;
    }
    case P_MPROT: {
        static const char bits[] = {'P', 'R', 'V', 'D'};
        int k, any = 0;
        for (k = 0; k < 4; k++) {
            if (g->mprot & (1u << k)) {
                if (any)
                    *o++ = '+';
                *o++ = bits[k];
                any = 1;
            }
        }
        if (!any) {
            s = "None";
            while (*s)
                *o++ = *s++;
        }
        *o = 0;
        return;
    }
    case P_MOCTV:
        {
            int sh = (int)g->moctv - 2;
            if (sh < 0) {
                out[0] = '-';
                put_u(out + 1, (u32)(-sh));
            } else {
                put_u(out, (u32)sh);
            }
        }
        return;
    default:
        break;
    }
    v = *param_ptr((struct genesis_ui *)g, p);
    if (p == P_TRK)
        put_u(out, (u32)v + 1);
    else
        put_u(out, v);
}

static const char *learn_line(const struct genesis_ui *g)
{
    if (g->mstyle != GEN_MARKOV_LEARNED)
        return style_name[g->mstyle < 5 ? g->mstyle : 0];
    if (g->evo.pending_ok)
        return "Pending";
    if (g->evo.last_status != GEN_OK)
        return "Error";
    if (g->evo.model_ready && g->evo.model.style == GEN_MARKOV_LEARNED)
        return "Learned";
    return "No model";
}

int cfw_genesis_render(void *w)
{
    struct genesis_ui *g = peek();
    struct dlw d = {0};
    char a[24], b[24];
    u32 i, x, occupied;
    const u32 C_TEXT = 0xffffff, C_DIM = 0x7a7a7a, C_ACC = 0x3a7bff, C_STEP = 0x2a2a2a,
              C_ON = 0x2bd45a, C_BG = 0x000000, C_ERR = 0xff5540;
    (void)w;
    if (!g)
        return 0;

    fw_eve_begin();
    fw_eve_dlstart();
    dl(&d, DL_SAVE_CONTEXT);
    dl(&d, DL_VERTEX_FORMAT(4));
    dl(&d, DL_COLOR_RGB(C_BG));
    dl(&d, DL_BEGIN(PRIM_RECTS));
    dl(&d, DL_VERTEX2F(PANEL_X * SUB, PANEL_Y * SUB));
    dl(&d, DL_VERTEX2F((PANEL_X + PANEL_W) * SUB, (PANEL_Y + PANEL_H) * SUB));
    dl(&d, DL_END);

    text(&d, TILE_TEXT_X, 26, C_TEXT, 0, "Genesis");
    text(&d, 200, 26, g->valid_trk ? C_ACC : C_ERR, 0, g->valid_trk ? "MIDI ok" : "need MIDI");

    if (g->recovery)
        text(&d, 180, 26, C_ERR, 0, "MAY DIFFER");

    if ((g->st == ST_PREV || g->st == ST_CONFIRM) && g->prev.valid) {
        const char *title = "Preview";
        if (g->st == ST_CONFIRM)
            title = "Replace?";
        else if (g->lab == LAB_EVO)
            title = evo_name[g->evo_mode < 3 ? g->evo_mode : 0];
        text(&d, TILE_TEXT_X, 48, C_DIM, 0, title);
        if (g->lab == LAB_EVO && g->evo_mode == GEN_EVO_TURING && g->lockv)
            text(&d, 250, 48, C_ERR, 0, "LOCK");
        if (g->lab == LAB_EVO && g->evo_mode == GEN_EVO_MARKOV)
            text(&d, 250, 66, C_ACC, 0, learn_line(g));
        put_u(a, g->prev.event_count);
        text(&d, 120, 48, C_TEXT, 0, a);
        text(&d, 160, 48, C_DIM, 0, "seed");
        put_u(a, g->seed);
        text(&d, 200, 48, C_TEXT, 0, a);
        put_u(a, (u32)g->track + 1);
        text(&d, TILE_TEXT_X, 66, C_DIM, 0, "trk");
        text(&d, 40, 66, C_TEXT, 0, a);
        put_u(a, g->native_len);
        text(&d, 70, 66, C_DIM, 0, "pat");
        text(&d, 100, 66, C_TEXT, 0, a);
        {
            int pages = (int)((g->len + PAGE_STEPS - 1) / PAGE_STEPS);
            int base, k;
            if (pages < 1)
                pages = 1;
            if ((int)g->page >= pages)
                g->page = (u8)(pages - 1);
            put_u(a, (u32)g->page + 1);
            text(&d, 140, 66, C_DIM, 0, "pg");
            text(&d, 164, 66, C_TEXT, 0, a);
            text(&d, 190, 66, C_DIM, 0, "/");
            put_u(a, (u32)pages);
            text(&d, 200, 66, C_TEXT, 0, a);

            occupied = 0;
            for (i = 0; i < g->len && i < GEN_SNAP_MAX; i++) {
                if (g->prev.occupancy[i])
                    occupied++;
            }
            base = (int)g->page * PAGE_STEPS;
            for (k = 0; k < PAGE_STEPS; k++) {
                i = base + k;
                if (i >= g->len || i >= GEN_SNAP_MAX)
                    break;
                x = TILE_TEXT_X + k * 14;
                dl(&d, DL_COLOR_RGB(g->prev.occupancy[i] ? C_ON : C_STEP));
                dl(&d, DL_BEGIN(PRIM_RECTS));
                dl(&d, DL_VERTEX2F(x * SUB, 90 * SUB));
                dl(&d, DL_VERTEX2F((x + 12) * SUB, 118 * SUB));
                dl(&d, DL_END);
                if (g->lab == LAB_EVO && g->evo_mode == GEN_EVO_MUTATE && step_prot(g, i)) {
                    dl(&d, DL_COLOR_RGB(C_ACC));
                    dl(&d, DL_BEGIN(PRIM_RECTS));
                    dl(&d, DL_VERTEX2F(x * SUB, 86 * SUB));
                    dl(&d, DL_VERTEX2F((x + 12) * SUB, 90 * SUB));
                    dl(&d, DL_END);
                }
                if (g->prev.occupancy[i]) {
                    fmt_note(a, g->prev.notes[i]);
                    text_f(&d, (int)(x + 6), 122, FONT_SMALL, C_DIM, OPT_CENTERX, a);
                }
            }
        }
        put_u(a, occupied);
        if (g->lab == LAB_EVO && g->evo_mode == GEN_EVO_TURING && g->evo.committed.ready) {
            int rk, rbase = (int)g->page * PAGE_STEPS;
            text(&d, 70, 48, C_DIM, 0, "reg");
            put_u(a, g->evo.committed.length);
            text(&d, 96, 48, C_TEXT, 0, a);
            for (rk = 0; rk < PAGE_STEPS; rk++) {
                int ri = rbase + rk;
                if (ri >= g->evo.committed.length)
                    break;
                x = TILE_TEXT_X + (u32)rk * 14;
                dl(&d, DL_COLOR_RGB(g->evo.committed.cells[ri] ? C_ACC : C_STEP));
                dl(&d, DL_BEGIN(PRIM_RECTS));
                dl(&d, DL_VERTEX2F(x * SUB, 78 * SUB));
                dl(&d, DL_VERTEX2F((x + 10) * SUB, 84 * SUB));
                dl(&d, DL_END);
            }
        }
        text(&d, TILE_TEXT_X, 150, C_DIM, 0, "hits");
        text(&d, 50, 150, C_TEXT, 0, a);
        if (g->st == ST_CONFIRM) {
            text(&d, TILE_TEXT_X, 170, C_ERR, 0, "Apply=replace  Back=cancel");
            cfw_key_tile(&d, 0, "Lab", 0);
            cfw_key_tile(&d, 1, "Replace", 1);
        } else if (g->recovery) {
            text(&d, TILE_TEXT_X, 170, C_ERR, 0, "Apply locked");
            cfw_key_tile(&d, 0, "Lab", 0);
            cfw_key_tile(&d, 1, "Locked", 0);
        } else {
            text(&d, TILE_TEXT_X, 170, C_ACC, 0, "Bottom: Apply");
            cfw_key_tile(&d, 0, "Lab", 0);
            cfw_key_tile(&d, 1, "Apply", 1);
        }
    } else {
        text(&d, TILE_TEXT_X, 48, C_DIM, 0,
             g->lab == LAB_EVO ? evo_name[g->evo_mode < 3 ? g->evo_mode : 0] : lab_name[g->lab]);
        if (g->lab == LAB_EVO && g->evo_mode == GEN_EVO_TURING) {
            text(&d, TILE_TEXT_X, 66, C_DIM, 0, "reg");
            put_u(a, g->reg_len);
            text(&d, 58, 66, C_TEXT, 0, a);
            if (g->lockv)
                text(&d, 90, 66, C_ERR, 0, "LOCK");
        }
        if (g->lab == LAB_EVO && g->evo_mode == GEN_EVO_MARKOV)
            text(&d, TILE_TEXT_X, 66, C_ACC, 0, learn_line(g));
        if (g->lab == LAB_EVO && g->evo_mode == GEN_EVO_MUTATE) {
            int k, base = (int)g->page * PAGE_STEPS;
            for (k = 0; k < PAGE_STEPS; k++) {
                int si = base + k;
                u32 colr = C_STEP;
                if (si < (int)g->len && step_prot(g, (unsigned)si))
                    colr = C_ACC;
                if (si == (int)g->mstep)
                    colr = C_TEXT;
                x = TILE_TEXT_X + (u32)k * 14;
                dl(&d, DL_COLOR_RGB(colr));
                dl(&d, DL_BEGIN(PRIM_RECTS));
                dl(&d, DL_VERTEX2F(x * SUB, 188 * SUB));
                dl(&d, DL_VERTEX2F((x + 10) * SUB, 196 * SUB));
                dl(&d, DL_END);
            }
        }
        put_u(a, (u32)g->track + 1);
        text(&d, 120, 48, C_DIM, 0, "trk");
        text(&d, 150, 48, C_TEXT, 0, a);
        put_u(a, g->len);
        text(&d, 188, 48, C_DIM, 0, "len");
        text(&d, 214, 48, C_TEXT, 0, a);
        put_u(a, g->native_len);
        text(&d, 250, 48, C_DIM, 0, "pat");
        text(&d, 276, 48, C_TEXT, 0, a);

        param_label(g, g->param, a);
        value_str(g, g->param, b);
        text(&d, TILE_TEXT_X, 80, C_ACC, 0, a);
        text_f(&d, TILE_TEXT_X, 104, FONT_LARGE, C_TEXT, 0, b);

        text(&d, TILE_TEXT_X, 140, C_DIM, 0, "Turn: value  Shift+Turn: param");
        text(&d, TILE_TEXT_X, 158, C_DIM, 0, "Click: Generate");
        text(&d, TILE_TEXT_X, 176, C_DIM, 0, "Shift+Click: Regen");

        if (g->st == ST_ERR || g->recovery)
            text(&d, 200, 80, C_ERR, 0, g->recovery ? "LOCK" : "ERR");

        cfw_key_tile(&d, 0, "Lab+", 1);
        cfw_key_tile(&d, 1, "Apply", 0);
    }

    dl(&d, DL_RESTORE_CONTEXT);
    fw_eve_end();
    return 0;
}

#endif /* CFW_MOD */
