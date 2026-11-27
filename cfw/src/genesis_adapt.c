/* G3A: portable Genesis events -> verified Play+ MIDI sequencer writes.
 * All native writes go through this adapter. No channel/CC writes. */
#include "genesis_adapt.h"
#include "slice.h"
#include <stddef.h>

/* Thumb/AAPCS layout checks (vdev packs these structs in guest RAM). */
_Static_assert(sizeof(gen_native_step) == 8, "gen_native_step size");
_Static_assert(sizeof(gen_event) == 12, "gen_event size");
_Static_assert(offsetof(gen_fixture_req, ctx) == 0, "fixture.ctx");
_Static_assert(offsetof(gen_fixture_req, fixture_id) == 4, "fixture.id");
_Static_assert(offsetof(gen_fixture_req, track) == 8, "fixture.track");
_Static_assert(offsetof(gen_fixture_req, step0) == 12, "fixture.step0");
_Static_assert(offsetof(gen_fixture_req, fail_at) == 20, "fixture.fail_at");
_Static_assert(offsetof(gen_fixture_req, snap_out) == 24, "fixture.snap");
_Static_assert(offsetof(gen_fixture_req, preview_out) == 28, "fixture.preview");
_Static_assert(sizeof(gen_fixture_req) == 32, "gen_fixture_req size");
_Static_assert(offsetof(gen_gen_req, fixture_id) == 0, "gen.fixture_id");
_Static_assert(offsetof(gen_gen_req, capacity) == 4, "gen.capacity");
_Static_assert(offsetof(gen_gen_req, out) == 8, "gen.out");
_Static_assert(offsetof(gen_gen_req, count_out) == 12, "gen.count_out");
_Static_assert(offsetof(gen_gen_req, seed_out) == 16, "gen.seed_out");
_Static_assert(sizeof(gen_gen_req) == 20, "gen_gen_req size");
_Static_assert(offsetof(gen_preview, valid) == 21, "preview.valid");
_Static_assert(offsetof(gen_preview, event_count) == 16, "preview.event_count");
_Static_assert(sizeof(gen_chord_req) == 40, "gen_chord_req size");
_Static_assert(offsetof(gen_chord_req, chords) == 24, "chord.chords");
_Static_assert(offsetof(gen_chord_req, fail_at) == 32, "chord.fail");
_Static_assert(offsetof(gen_chord_req, snap_out) == 36, "chord.snap");

extern u32 fw_step_toggle(void *sel, int track, int step);

#define BUF_CTX 0x98
#define TRK_VT_LENGTH 0x04
#define PV_VT_TRACK 0x00

#define GEN_FAIL_NONE 0
#define GEN_FAIL_BEFORE_SNAP 1
#define GEN_FAIL_AFTER_SNAP 2
#define GEN_FAIL_BEFORE_WRITE 3
#define GEN_FAIL_AFTER_FIRST_WRITE 4
#define GEN_FAIL_MID_EDIT 5
#define GEN_FAIL_BEFORE_COMMIT 6
#define GEN_FAIL_DURING_RESTORE 7
#define GEN_FAIL_PAGE_EDGE 8 /* diagnostic: after the first write at step >= 16 */
#define GEN_FAIL_NEAR_END 9  /* diagnostic: after the write before the last event */
#define GEN_FAIL_READBACK 10 /* diagnostic: after the writes, before commit */
#define GEN_MIDI_PARAM_CHORD 10
#define STEP_VT_CLEAR_LOCK 0x30
#define CHORD_LOCK 0x80u

#define GEN_TPS 480u /* portable ticks per step used by fixtures */

static void *ctrl_of(void *ctx) { return PTR(ctx, MENU_CTX_CTRL); }
static void *sel_of(void *ctx) { return (char *)ctrl_of(ctx) + CTRL_SEL; }

static void *track_obj(void *ctx, int track)
{
    void *pv = PTR(sel_of(ctx), SEL_PVIEW);
    return VFN(pv, PV_VT_TRACK, void *, int)(pv, track);
}

static void *inst_of(void *ctx, int track)
{
    void *app = PTR(ctx, MENU_CTX_APP);
    void *seq = PTR(app, APP_SEQ_PTR);
    void *buf = VFN(seq, SEQ_VT_PLAY_BUF, void *)(seq);
    return *(void **)((char *)buf + BUF_CTX + track * BUF_TRACK_STRIDE);
}

static u32 udiv32(u32 n, u32 d)
{
    u32 q = 0, bit = 1;
    if (d == 0)
        return 0;
    while (d < n && !(d & 0x80000000u)) {
        d <<= 1;
        bit <<= 1;
    }
    while (bit) {
        if (n >= d) {
            n -= d;
            q |= bit;
        }
        d >>= 1;
        bit >>= 1;
    }
    return q;
}

u32 genesis_midi_caps(void)
{
    return (1u << 0) | (1u << 2) | (1u << 3) | (1u << 9) | (1u << 10)
         | GEN_MIDI_CAP_LEN16 | GEN_MIDI_CAP_LEN32 | GEN_MIDI_CAP_LEN64;
}

int genesis_midi_validate_track(void *ctx, int track)
{
    void *st;
    u32 typ;
    if (!ctx || track < 0 || track >= NUM_TRACKS)
        return GEN_MIDI_ERR_BAD_TRACK;
    st = cfw_step_at(ctx, track, 0);
    if (!st)
        return GEN_MIDI_ERR_BAD_TRACK;
    typ = VFN(st, STEP_VT_TRACK_TYPE, u32)(st);
    if ((typ & 0xff) != 1)
        return GEN_MIDI_ERR_BAD_TRACK;
    if (inst_of(ctx, track) != 0)
        return GEN_MIDI_ERR_BAD_TRACK;
    return GEN_MIDI_OK;
}

int genesis_midi_pattern_length(void *ctx, int track)
{
    void *trk;
    s32 n;
    if (genesis_midi_validate_track(ctx, track) != GEN_MIDI_OK)
        return GEN_MIDI_ERR_BAD_TRACK;
    trk = track_obj(ctx, track);
    if (!trk)
        return GEN_MIDI_ERR_INTERNAL;
    n = VFN(trk, TRK_VT_LENGTH, s32)(trk);
    if (n < 1 || n > NUM_STEPS)
        return GEN_MIDI_ERR_INTERNAL;
    return (int)n;
}

int genesis_midi_read_step(void *ctx, int track, int step, gen_native_step *out)
{
    void *st;
    s16 v;
    int plen;
    if (!out)
        return GEN_MIDI_ERR_PARAM;
    if (genesis_midi_validate_track(ctx, track) != GEN_MIDI_OK)
        return GEN_MIDI_ERR_BAD_TRACK;
    plen = genesis_midi_pattern_length(ctx, track);
    if (plen < 0)
        return plen;
    if (step < 0 || step >= plen)
        return GEN_MIDI_ERR_BAD_STEP;
    st = cfw_step_at(ctx, track, step);
    if (!st)
        return GEN_MIDI_ERR_INTERNAL;
    out->on = (u8)(VFN(st, STEP_VT_IS_ON, u32)(st) & 0xff);
    out->chord = 0;
    out->note = out->vel = out->dur = -1;
    if (VFN(st, STEP_VT_GET_PARAM, u32, int, s16 *)(st, GEN_MIDI_PARAM_NOTE, &v) & 0xff)
        out->note = v;
    if (VFN(st, STEP_VT_GET_PARAM, u32, int, s16 *)(st, GEN_MIDI_PARAM_VEL, &v) & 0xff)
        out->vel = v;
    if (VFN(st, STEP_VT_GET_PARAM, u32, int, s16 *)(st, GEN_MIDI_PARAM_DUR, &v) & 0xff)
        out->dur = v;
    if (VFN(st, STEP_VT_GET_PARAM, u32, int, s16 *)(st, GEN_MIDI_PARAM_CHORD, &v) & 0xff) {
        if (v < 0 || v > GEN_NID_MAX)
            return GEN_MIDI_ERR_PARAM;
        out->chord = (u8)(CHORD_LOCK | (u8)v);
    }
    return GEN_MIDI_OK;
}

int genesis_midi_ticks_to_step(u32 tick, u32 ticks_per_step, u32 *step_out)
{
    u32 q, rem;
    if (!step_out || ticks_per_step == 0)
        return GEN_MIDI_ERR_PARAM;
    q = udiv32(tick, ticks_per_step);
    rem = tick - q * ticks_per_step;
    if (rem != 0)
        return GEN_MIDI_ERR_TIMING;
    *step_out = q;
    return GEN_MIDI_OK;
}

int genesis_midi_ticks_to_dur(u32 duration_ticks, u32 ticks_per_step, s16 *dur_out)
{
    u32 num, q;
    if (!dur_out || ticks_per_step == 0)
        return GEN_MIDI_ERR_PARAM;
    if (duration_ticks == 0)
        return GEN_MIDI_ERR_TIMING;
    /* native = round(duration_ticks * 60 / ticks_per_step), clamp 0..3840 */
    if (duration_ticks > 0xffffffffu / GEN_MIDI_DUR_PER_STEP)
        return GEN_MIDI_ERR_TIMING;
    num = duration_ticks * GEN_MIDI_DUR_PER_STEP + ticks_per_step / 2u;
    q = udiv32(num, ticks_per_step);
    if (q > (u32)GEN_MIDI_DUR_MAX)
        q = (u32)GEN_MIDI_DUR_MAX;
    *dur_out = (s16)q;
    return GEN_MIDI_OK;
}

static int channel_policy(const gen_event *ev, u32 n)
{
    u32 i;
    u8 ch;
    if (n == 0)
        return GEN_MIDI_OK;
    ch = ev[0].channel;
    for (i = 1; i < n; i++)
        if (ev[i].channel != ch)
            return GEN_MIDI_ERR_CHANNEL;
    return GEN_MIDI_OK;
}

static int map_event(const gen_event *e, u32 ticks_per_step, int step0, int plen,
                     u32 *abs_step, s16 *dur)
{
    u32 rel;
    int st;
    if (genesis_midi_ticks_to_step(e->tick, ticks_per_step, &rel) != GEN_MIDI_OK)
        return GEN_MIDI_ERR_TIMING;
    if (genesis_midi_ticks_to_dur(e->duration, ticks_per_step, dur) != GEN_MIDI_OK)
        return GEN_MIDI_ERR_TIMING;
    if (e->note > 127 || e->velocity < 1 || e->velocity > 127)
        return GEN_MIDI_ERR_PARAM;
    st = step0 + (int)rel;
    if (st < 0 || st >= plen)
        return GEN_MIDI_ERR_BAD_STEP;
    *abs_step = (u32)st;
    return GEN_MIDI_OK;
}

int genesis_midi_validate_events(void *ctx, int track, int step0, const gen_event *ev, u32 n,
                                 u32 ticks_per_step, gen_apply_policy policy)
{
    gen_buf b;
    gen_event storage[GEN_SNAP_MAX];
    u32 i;
    int plen;
    u8 occupied[GEN_SNAP_MAX];
    (void)policy;
    if (!ev || n == 0 || ticks_per_step == 0)
        return GEN_MIDI_ERR_PARAM;
    if (n > GEN_SNAP_MAX)
        return GEN_MIDI_ERR_CAPACITY;
    if (genesis_midi_validate_track(ctx, track) != GEN_MIDI_OK)
        return GEN_MIDI_ERR_BAD_TRACK;
    plen = genesis_midi_pattern_length(ctx, track);
    if (plen < 0)
        return plen;
    if (step0 < 0 || step0 >= plen)
        return GEN_MIDI_ERR_BAD_STEP;
    if (channel_policy(ev, n) != GEN_MIDI_OK)
        return GEN_MIDI_ERR_CHANNEL;

    for (i = 0; i < GEN_SNAP_MAX; i++)
        occupied[i] = 0;
    for (i = 0; i < n; i++) {
        u32 abs;
        s16 dur;
        int rc = map_event(&ev[i], ticks_per_step, step0, plen, &abs, &dur);
        if (rc != GEN_MIDI_OK)
            return rc;
        if (abs >= GEN_SNAP_MAX)
            return GEN_MIDI_ERR_CAPACITY;
        if (occupied[abs])
            return GEN_MIDI_ERR_MONO;
        occupied[abs] = 1;
        storage[i] = ev[i];
    }
    gen_buf_init(&b, storage, n);
    b.count = n;
    if (gen_buf_validate(&b, 1) != GEN_OK)
        return GEN_MIDI_ERR_MONO;
    return GEN_MIDI_OK;
}

int genesis_midi_begin(void *ctx, const char *undo_label)
{
    if (!ctx)
        return GEN_MIDI_ERR_PARAM;
    cfw_edit_begin(ctrl_of(ctx), undo_label ? undo_label : "Genesis");
    return GEN_MIDI_OK;
}

int genesis_midi_upsert(void *ctx, int track, int step, u8 note, u8 vel, s16 dur)
{
    void *st, *sel;
    int plen;
    if (genesis_midi_validate_track(ctx, track) != GEN_MIDI_OK)
        return GEN_MIDI_ERR_BAD_TRACK;
    plen = genesis_midi_pattern_length(ctx, track);
    if (plen < 0)
        return plen;
    if (step < 0 || step >= plen)
        return GEN_MIDI_ERR_BAD_STEP;
    if (note > 127 || vel > 127 || dur < 0 || dur > GEN_MIDI_DUR_MAX)
        return GEN_MIDI_ERR_PARAM;
    st = cfw_step_at(ctx, track, step);
    sel = sel_of(ctx);
    if (!st)
        return GEN_MIDI_ERR_INTERNAL;
    cfw_edit_lock();
    if (!(VFN(st, STEP_VT_IS_ON, u32)(st) & 0xff))
        fw_step_toggle(sel, track, step);
    VFN(st, STEP_VT_SET_PARAM, void, int, int)(st, GEN_MIDI_PARAM_NOTE, (int)note);
    VFN(st, STEP_VT_SET_PARAM, void, int, int)(st, GEN_MIDI_PARAM_VEL, (int)vel);
    VFN(st, STEP_VT_SET_PARAM, void, int, int)(st, GEN_MIDI_PARAM_DUR, (int)dur);
    cfw_edit_unlock();
    return GEN_MIDI_OK;
}

int genesis_midi_remove(void *ctx, int track, int step)
{
    void *st, *sel;
    int plen;
    if (genesis_midi_validate_track(ctx, track) != GEN_MIDI_OK)
        return GEN_MIDI_ERR_BAD_TRACK;
    plen = genesis_midi_pattern_length(ctx, track);
    if (plen < 0)
        return plen;
    if (step < 0 || step >= plen)
        return GEN_MIDI_ERR_BAD_STEP;
    st = cfw_step_at(ctx, track, step);
    sel = sel_of(ctx);
    if (!st)
        return GEN_MIDI_ERR_INTERNAL;
    cfw_edit_lock();
    if (VFN(st, STEP_VT_IS_ON, u32)(st) & 0xff)
        fw_step_toggle(sel, track, step);
    cfw_edit_unlock();
    return GEN_MIDI_OK;
}

/* locked=1 stores id 0..20 and sets the lock. locked=0 stores 0 and clears
 * the lock, which is the readable unlocked state. The stock setter clamps,
 * so an out-of-range id never reaches it. */
static int chord_store(void *ctx, int track, int step, int locked, int id)
{
    void *st;
    if (locked && (id < 0 || id > GEN_NID_MAX))
        return GEN_MIDI_ERR_PARAM;
    st = cfw_step_at(ctx, track, step);
    if (!st)
        return GEN_MIDI_ERR_INTERNAL;
    cfw_edit_lock();
    if (locked)
        VFN(st, STEP_VT_SET_PARAM, void, int, int)(st, GEN_MIDI_PARAM_CHORD, id);
    else {
        VFN(st, STEP_VT_SET_PARAM, void, int, int)(st, GEN_MIDI_PARAM_CHORD, 0);
        VFN(st, STEP_VT_CLEAR_LOCK, void, int)(st, GEN_MIDI_PARAM_CHORD);
    }
    cfw_edit_unlock();
    return GEN_MIDI_OK;
}

static int chord_is_locked(u8 bits)
{
    return (bits & CHORD_LOCK) != 0;
}

static int chord_id_bits(u8 bits)
{
    return (int)(bits & 0x7fu);
}

int genesis_midi_commit(void *ctx)
{
    if (!ctx)
        return GEN_MIDI_ERR_PARAM;
    cfw_refresh_pads(ctrl_of(ctx));
    return GEN_MIDI_OK;
}

int genesis_midi_cancel(void *ctx)
{
    (void)ctx;
    return GEN_MIDI_OK;
}

int genesis_midi_snapshot(void *ctx, int track, int step0, int count, gen_snap *snap)
{
    int i, plen;
    if (!snap || count < 1 || count > GEN_SNAP_MAX)
        return GEN_MIDI_ERR_CAPACITY;
    if (genesis_midi_validate_track(ctx, track) != GEN_MIDI_OK)
        return GEN_MIDI_ERR_BAD_TRACK;
    plen = genesis_midi_pattern_length(ctx, track);
    if (plen < 0)
        return plen;
    if (step0 < 0 || step0 + count > plen)
        return GEN_MIDI_ERR_BAD_STEP;
    snap->magic = GEN_SNAP_MAGIC;
    snap->track = track;
    snap->step0 = step0;
    snap->count = count;
    for (i = 0; i < count; i++) {
        int rc = genesis_midi_read_step(ctx, track, step0 + i, &snap->steps[i]);
        if (rc != GEN_MIDI_OK)
            return rc;
    }
    return GEN_MIDI_OK;
}

/* setParam clamps velocity to 0..127. A stock toggle stores 7600 in that
 * slot, and writing 7600 back comes out as 127. Callers must not treat that
 * as a finished restore. */
static int vel_round_trips(s16 vel)
{
    return vel >= 0 && vel <= 127;
}

static int param_unlock(void *ctx, int track, int step, int idx)
{
    void *st = cfw_step_at(ctx, track, step);
    if (!st)
        return GEN_MIDI_ERR_INTERNAL;
    cfw_edit_lock();
    VFN(st, STEP_VT_CLEAR_LOCK, void, int)(st, idx);
    cfw_edit_unlock();
    return GEN_MIDI_OK;
}

int genesis_midi_restore(void *ctx, const gen_snap *snap)
{
    int i, plen, inexact = 0;
    if (!snap || snap->magic != GEN_SNAP_MAGIC)
        return GEN_MIDI_ERR_RESTORE;
    if (genesis_midi_validate_track(ctx, snap->track) != GEN_MIDI_OK)
        return GEN_MIDI_ERR_BAD_TRACK;
    if (snap->count < 1 || snap->count > GEN_SNAP_MAX)
        return GEN_MIDI_ERR_CAPACITY;
    plen = genesis_midi_pattern_length(ctx, snap->track);
    if (plen < 0)
        return plen;
    if (snap->step0 < 0 || snap->step0 + snap->count > plen)
        return GEN_MIDI_ERR_BAD_STEP;
    genesis_midi_begin(ctx, "GenesisRestore");
    for (i = 0; i < snap->count; i++) {
        const gen_native_step *s = &snap->steps[i];
        int step = snap->step0 + i;
        if (s->on) {
            if (s->note < 0 || s->note > 127 || s->dur < 0 || s->dur > GEN_MIDI_DUR_MAX)
                return GEN_MIDI_ERR_RESTORE;
            if (vel_round_trips(s->vel)) {
                if (genesis_midi_upsert(ctx, snap->track, step, (u8)s->note, (u8)s->vel, s->dur) !=
                    GEN_MIDI_OK)
                    return GEN_MIDI_ERR_RESTORE;
            } else {
                /* Occupancy, note, and duration can come back. Velocity cannot. */
                void *st = cfw_step_at(ctx, snap->track, step);
                void *sel = sel_of(ctx);
                if (!st)
                    return GEN_MIDI_ERR_RESTORE;
                inexact = 1;
                cfw_edit_lock();
                if (!(VFN(st, STEP_VT_IS_ON, u32)(st) & 0xff))
                    fw_step_toggle(sel, snap->track, step);
                VFN(st, STEP_VT_SET_PARAM, void, int, int)(st, GEN_MIDI_PARAM_NOTE, (int)s->note);
                VFN(st, STEP_VT_SET_PARAM, void, int, int)(st, GEN_MIDI_PARAM_DUR, (int)s->dur);
                cfw_edit_unlock();
            }
        } else {
            if (genesis_midi_remove(ctx, snap->track, step) != GEN_MIDI_OK)
                return GEN_MIDI_ERR_RESTORE;
            /* A failed write locks note, velocity, and duration. Clear those
             * locks when the snapshot had them unlocked. The halfword may stay. */
            if (s->note < 0 && param_unlock(ctx, snap->track, step, GEN_MIDI_PARAM_NOTE) != GEN_MIDI_OK)
                return GEN_MIDI_ERR_RESTORE;
            if (s->vel < 0 && param_unlock(ctx, snap->track, step, GEN_MIDI_PARAM_VEL) != GEN_MIDI_OK)
                return GEN_MIDI_ERR_RESTORE;
            if (s->dur < 0 && param_unlock(ctx, snap->track, step, GEN_MIDI_PARAM_DUR) != GEN_MIDI_OK)
                return GEN_MIDI_ERR_RESTORE;
        }
        /* Toggle-on forces chord id 0. Put the saved lock and id back after it,
         * including on a step that stays off (setParam while off does not toggle). */
        if (chord_is_locked(s->chord)) {
            if (chord_store(ctx, snap->track, step, 1, chord_id_bits(s->chord)) != GEN_MIDI_OK)
                return GEN_MIDI_ERR_RESTORE;
        } else if (chord_store(ctx, snap->track, step, 0, 0) != GEN_MIDI_OK) {
            return GEN_MIDI_ERR_RESTORE;
        }
    }
    genesis_midi_commit(ctx);
    /* Velocity (or another field) could not be written back. Chord, note,
     * duration, and occupancy were. The caller keeps the recovery lock. */
    if (inexact)
        return GEN_MIDI_ERR_RESTORE;
    return GEN_MIDI_OK;
}

int genesis_midi_apply_req(const gen_apply_req *req)
{
    if (!req)
        return GEN_MIDI_ERR_PARAM;
    return genesis_midi_apply_ex(req->ctx, req->track, req->step0, req->ev, req->n,
                                 req->ticks_per_step, req->policy, req->snap_out, req->fail_at, 0);
}

int genesis_midi_apply(void *ctx, int track, int step0, const gen_event *ev, u32 n,
                       u32 ticks_per_step, gen_apply_policy policy, gen_snap *snap_out)
{
    return genesis_midi_apply_ex(ctx, track, step0, ev, n, ticks_per_step, policy, snap_out, 0, 0);
}

static int event_step(const gen_event *ev, u32 n, u32 ticks_per_step, int step0, int plen, int step)
{
    u32 i;
    for (i = 0; i < n; i++) {
        u32 abs;
        s16 dur;
        if (map_event(&ev[i], ticks_per_step, step0, plen, &abs, &dur) != GEN_MIDI_OK)
            continue;
        if ((int)abs == step)
            return (int)i;
    }
    return -1;
}

int genesis_midi_apply_ex(void *ctx, int track, int step0, const gen_event *ev, u32 n,
                          u32 ticks_per_step, gen_apply_policy policy, gen_snap *snap_out,
                          int fail_at, const u8 *chords)
{
    gen_snap local, *snap;
    int plen, r0, rlen, i, wrote = 0;
    int rc;
    u32 ii;

    if (policy != GEN_APPLY_EMPTY_ONLY && policy != GEN_APPLY_REPLACE_REGION)
        return GEN_MIDI_ERR_UNSUPPORTED;

    rc = genesis_midi_validate_events(ctx, track, step0, ev, n, ticks_per_step, policy);
    if (rc != GEN_MIDI_OK)
        return rc;
    if (chords) {
        for (ii = 0; ii < n; ii++)
            if (chords[ii] > GEN_NID_MAX)
                return GEN_MIDI_ERR_PARAM;
    }

    plen = genesis_midi_pattern_length(ctx, track);
    {
        int lo = plen, hi = -1;
        for (ii = 0; ii < n; ii++) {
            u32 abs;
            s16 dur;
            map_event(&ev[ii], ticks_per_step, step0, plen, &abs, &dur);
            if ((int)abs < lo)
                lo = (int)abs;
            if ((int)abs > hi)
                hi = (int)abs;
        }
        r0 = lo;
        rlen = hi - lo + 1;
    }

    if (policy == GEN_APPLY_EMPTY_ONLY) {
        for (i = 0; i < rlen; i++) {
            gen_native_step st;
            if (genesis_midi_read_step(ctx, track, r0 + i, &st) != GEN_MIDI_OK)
                return GEN_MIDI_ERR_INTERNAL;
            if (st.on)
                return GEN_MIDI_ERR_OCCUPIED;
        }
    }

    if (fail_at == GEN_FAIL_BEFORE_SNAP)
        return GEN_MIDI_ERR_INTERNAL;

    snap = snap_out ? snap_out : &local;
    rc = genesis_midi_snapshot(ctx, track, r0, rlen, snap);
    if (rc != GEN_MIDI_OK)
        return rc;

    if (fail_at == GEN_FAIL_AFTER_SNAP)
        return GEN_MIDI_ERR_INTERNAL;

    genesis_midi_begin(ctx, "Genesis");

    if (fail_at == GEN_FAIL_BEFORE_WRITE) {
        genesis_midi_cancel(ctx);
        return GEN_MIDI_ERR_INTERNAL;
    }

    if (policy == GEN_APPLY_REPLACE_REGION) {
        for (i = 0; i < rlen; i++) {
            int step = r0 + i;
            int is_ev = event_step(ev, n, ticks_per_step, step0, plen, step) >= 0;
            const gen_native_step *orig = &snap->steps[i];
            int latent = chord_is_locked(orig->chord) && chord_id_bits(orig->chord) != 0;
            if (orig->on && genesis_midi_remove(ctx, track, step) != GEN_MIDI_OK)
                goto fail_restore;
            /* A cleared hole must not keep a latent chord id. Event steps are
             * rewritten below. An off, unlocked, or locked-None hole stays. */
            if (!is_ev && (orig->on || latent)) {
                if (chord_store(ctx, track, step, 1, 0) != GEN_MIDI_OK)
                    goto fail_restore;
            }
        }
    }

    for (i = 0; i < (int)n; i++) {
        u32 abs;
        s16 dur;
        int id = chords ? (int)chords[i] : 0;
        map_event(&ev[i], ticks_per_step, step0, plen, &abs, &dur);
        rc = genesis_midi_upsert(ctx, track, (int)abs, ev[i].note, ev[i].velocity, dur);
        if (rc != GEN_MIDI_OK)
            goto fail_restore;
        rc = chord_store(ctx, track, (int)abs, 1, id);
        if (rc != GEN_MIDI_OK)
            goto fail_restore;
        wrote++;
        if (fail_at == GEN_FAIL_AFTER_FIRST_WRITE && wrote == 1)
            goto fail_restore;
        if (fail_at == GEN_FAIL_MID_EDIT && wrote == ((int)n + 1) / 2)
            goto fail_restore;
        if (fail_at == GEN_FAIL_PAGE_EDGE && (int)abs >= 16)
            goto fail_restore;
        if (fail_at == GEN_FAIL_NEAR_END && n > 1 && wrote == (int)n - 1)
            goto fail_restore;
    }

    if (fail_at == GEN_FAIL_READBACK)
        goto fail_restore;

    for (i = 0; i < rlen; i++) {
        int step = r0 + i;
        int ei = event_step(ev, n, ticks_per_step, step0, plen, step);
        gen_native_step now;
        if (genesis_midi_read_step(ctx, track, step, &now) != GEN_MIDI_OK)
            goto fail_restore;
        if (ei >= 0) {
            u32 abs;
            s16 dur;
            int id = chords ? (int)chords[ei] : 0;
            map_event(&ev[ei], ticks_per_step, step0, plen, &abs, &dur);
            if (!now.on || now.note != (s16)ev[ei].note || now.vel != (s16)ev[ei].velocity ||
                now.dur != dur)
                goto fail_restore;
            if (!chord_is_locked(now.chord) || chord_id_bits(now.chord) != id)
                goto fail_restore;
        } else if (policy == GEN_APPLY_REPLACE_REGION) {
            const gen_native_step *orig = &snap->steps[i];
            int changed = orig->on || (chord_is_locked(orig->chord) && chord_id_bits(orig->chord) != 0);
            if (changed) {
                if (now.on || !chord_is_locked(now.chord) || chord_id_bits(now.chord) != 0)
                    goto fail_restore;
            } else if (now.on != orig->on || now.chord != orig->chord) {
                goto fail_restore;
            }
        } else if (now.on != snap->steps[i].on || now.chord != snap->steps[i].chord ||
                   now.note != snap->steps[i].note || now.vel != snap->steps[i].vel ||
                   now.dur != snap->steps[i].dur) {
            goto fail_restore;
        }
    }

    if (fail_at == GEN_FAIL_BEFORE_COMMIT)
        goto fail_restore;

    if (fail_at == GEN_FAIL_DURING_RESTORE) {
        /* Writes committed; restore intentionally skipped → recovery-required. */
        genesis_midi_commit(ctx);
        return GEN_MIDI_ERR_RESTORE;
    }

    genesis_midi_commit(ctx);
    return GEN_MIDI_OK;

fail_restore:
    if (genesis_midi_restore(ctx, snap) != GEN_MIDI_OK) {
        genesis_midi_cancel(ctx);
        return GEN_MIDI_ERR_RESTORE;
    }
    return GEN_MIDI_ERR_INTERNAL;
}

int genesis_midi_preview_req(const gen_preview_req *req)
{
    if (!req)
        return GEN_MIDI_ERR_PARAM;
    return genesis_midi_preview_build(req->ctx, req->track, req->step0, req->ev, req->n,
                                      req->ticks_per_step, req->seed, req->generator,
                                      (gen_apply_policy)req->policy, req->out);
}

int genesis_midi_preview_build(void *ctx, int track, int step0, const gen_event *ev, u32 n,
                               u32 ticks_per_step, u32 seed, u8 generator, gen_apply_policy policy,
                               gen_preview *out)
{
    int plen, i;
    u32 ii;
    if (!out)
        return GEN_MIDI_ERR_PARAM;
    for (i = 0; i < GEN_SNAP_MAX; i++) {
        out->occupancy[i] = 0;
        out->notes[i] = 0;
        out->vels[i] = 0;
        out->durs[i] = 0;
    }
    out->track = track;
    out->step0 = step0;
    out->seed = seed;
    out->generator = generator;
    out->policy = (u8)policy;
    out->event_count = n;
    out->valid = 0;
    out->_pad = 0;
    plen = genesis_midi_pattern_length(ctx, track);
    if (plen < 0) {
        out->length = 0;
        return plen;
    }
    out->length = plen;
    if (genesis_midi_validate_events(ctx, track, step0, ev, n, ticks_per_step, policy) !=
        GEN_MIDI_OK)
        return GEN_MIDI_ERR_MONO;
    for (ii = 0; ii < n; ii++) {
        u32 abs;
        s16 dur;
        map_event(&ev[ii], ticks_per_step, step0, plen, &abs, &dur);
        if (abs < GEN_SNAP_MAX) {
            out->occupancy[abs] = 1;
            out->notes[abs] = ev[ii].note;
            out->vels[abs] = ev[ii].velocity;
            out->durs[abs] = (u16)dur;
        }
    }
    out->valid = 1;
    return GEN_MIDI_OK;
}

/* ---- fixtures (G2 engine) -------------------------------------------------- */

static int gen_fixture(int id, gen_event *storage, u32 cap, gen_buf *out, u32 *seed_out)
{
    gen_rhythm rh;
    gen_melody_cfg mc;
    gen_arp_cfg ac;
    gen_status st;
    uint8_t notes[3];
    u32 i;

    gen_buf_init(out, storage, cap);
    for (i = 0; i < sizeof(mc); i++)
        ((u8 *)&mc)[i] = 0;
    for (i = 0; i < sizeof(ac); i++)
        ((u8 *)&ac)[i] = 0;

    switch (id) {
    case 1: /* A: scale-aware random melody, C nat minor, 16 steps, seed 42 */
        *seed_out = 42;
        {
            gen_rng rng;
            gen_rng_seed(&rng, 42);
            st = gen_rhythm_probability(&rh, 16, (uint16_t)GEN_TPS, 70, &rng);
        }
        if (st != GEN_OK)
            return GEN_MIDI_ERR_INTERNAL;
        mc.root = 60;
        mc.scale = GEN_SCALE_NAT_MINOR;
        mc.pitch_min = 48;
        mc.pitch_max = 72;
        mc.vel_min = 64;
        mc.vel_max = 110;
        mc.duration = GEN_TPS; /* full step */
        mc.channel = 0;
        mc.seed = 42;
        st = gen_melody_uniform(&mc, &rh, out);
        break;
    case 2: /* B: Euclidean bassline 5/16, D Dorian, seed 123 */
        *seed_out = 123;
        st = gen_rhythm_euclidean(&rh, 16, 5, 0, (uint16_t)GEN_TPS);
        if (st != GEN_OK)
            return GEN_MIDI_ERR_INTERNAL;
        mc.root = 62; /* D */
        mc.scale = GEN_SCALE_DORIAN;
        mc.pitch_min = 36;
        mc.pitch_max = 55;
        mc.vel_min = 80;
        mc.vel_max = 100;
        mc.duration = GEN_TPS;
        mc.channel = 0;
        mc.seed = 123;
        st = gen_melody_uniform(&mc, &rh, out);
        break;
    case 3: /* C: random-walk A minor pent, 32 steps, seed 2026 */
        *seed_out = 2026;
        {
            gen_rng rng;
            gen_rng_seed(&rng, 2026);
            st = gen_rhythm_probability(&rh, 32, (uint16_t)GEN_TPS, 60, &rng);
        }
        if (st != GEN_OK)
            return GEN_MIDI_ERR_INTERNAL;
        mc.root = 57; /* A */
        mc.scale = GEN_SCALE_MINOR_PENT;
        mc.pitch_min = 45;
        mc.pitch_max = 81;
        mc.vel_min = 70;
        mc.vel_max = 115;
        mc.duration = GEN_TPS / 2; /* half step */
        mc.channel = 0;
        mc.seed = 2026;
        mc.max_interval = 3;
        mc.repeat_prob = 20;
        st = gen_melody_walk(&mc, &rh, out);
        break;
    case 4: /* D: arp 60,64,67 ascending, 16 steps */
        *seed_out = 0;
        notes[0] = 60;
        notes[1] = 64;
        notes[2] = 67;
        ac.mode = GEN_ARP_UP;
        ac.rate = GEN_TPS;
        ac.gate = GEN_TPS;
        ac.octaves = 1;
        ac.length = 16;
        ac.velocity = 96;
        ac.channel = 0;
        ac.seed = 0;
        st = gen_arp(&ac, notes, 3, out);
        break;
    default:
        return GEN_MIDI_ERR_PARAM;
    }
    if (st != GEN_OK)
        return GEN_MIDI_ERR_INTERNAL;
    if (gen_buf_validate(out, 1) != GEN_OK)
        return GEN_MIDI_ERR_MONO;
    return GEN_MIDI_OK;
}

int genesis_diag_fixture(const gen_fixture_req *req)
{
    gen_event storage[GEN_SNAP_MAX];
    gen_buf buf;
    u32 seed = 0;
    int rc;
    if (!req || !req->ctx)
        return GEN_MIDI_ERR_PARAM;
    rc = gen_fixture(req->fixture_id, storage, GEN_SNAP_MAX, &buf, &seed);
    if (rc != GEN_MIDI_OK)
        return rc;
    if (req->preview_out)
        return genesis_midi_preview_build(req->ctx, req->track, req->step0, buf.events, buf.count,
                                          GEN_TPS, seed, (u8)req->fixture_id, req->policy,
                                          req->preview_out);
    return genesis_midi_apply_ex(req->ctx, req->track, req->step0, buf.events, buf.count, GEN_TPS,
                                 req->policy, req->snap_out, req->fail_at, 0);
}

int genesis_diag_chord_apply(const gen_chord_req *req)
{
    if (!req)
        return GEN_MIDI_ERR_PARAM;
    return genesis_midi_apply_ex(req->ctx, req->track, req->step0, req->ev, req->n,
                                 req->ticks_per_step, (gen_apply_policy)req->policy, req->snap_out,
                                 req->fail_at, req->chords);
}

int genesis_diag_generate(const gen_gen_req *req)
{
    gen_buf buf;
    u32 seed = 0;
    int rc;
    if (!req || !req->out || req->capacity == 0)
        return GEN_MIDI_ERR_PARAM;
    if (req->capacity > GEN_SNAP_MAX)
        return GEN_MIDI_ERR_CAPACITY;
    rc = gen_fixture(req->fixture_id, req->out, req->capacity, &buf, &seed);
    if (rc != GEN_MIDI_OK)
        return rc;
    if (req->count_out)
        *req->count_out = buf.count;
    if (req->seed_out)
        *req->seed_out = seed;
    return GEN_MIDI_OK;
}
