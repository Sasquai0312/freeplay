#include "genesis.h"

/* Stock chord-type ids. Display names live with the UI. This file only
 * chooses an id the firmware already stores in parameter 10. */

int gen_native_id_ok(int id)
{
    return id >= 0 && id <= GEN_NID_MAX;
}

static int degree_midi(gen_scale scale, int root, int degree)
{
    int base, delta, midi;
    if (root < 0 || root > 127)
        return -1;
    base = gen_scale_degree_to_midi(scale, 60, degree, 0);
    if (base < 0)
        return -1;
    delta = base - 60;
    midi = root + delta;
    if (midi < 0 || midi > 127)
        return -1;
    return midi;
}

static int interval_from(gen_scale scale, int degree, int add)
{
    int a, b, d;
    a = degree_midi(scale, 60, degree);
    b = degree_midi(scale, 60, degree + add);
    if (a < 0 || b < 0)
        return -1;
    d = b - a;
    while (d < 0)
        d += 12;
    return d % 12;
}

/* Triad id from the scale, or -1 when the stock list has no such triad.
 * Seventh id, or -1. Half-diminished and diminished sevenths are not stock ids. */
static int triad_id(gen_scale scale, int degree)
{
    int t = interval_from(scale, degree, 2);
    int f = interval_from(scale, degree, 4);
    if (t < 0 || f < 0)
        return -1;
    if (t == 4 && f == 7)
        return GEN_NID_MAJOR;
    if (t == 3 && f == 7)
        return GEN_NID_MINOR;
    if (t == 3 && f == 6)
        return GEN_NID_DIM;
    if (t == 4 && f == 8)
        return GEN_NID_AUG;
    return -1;
}

static int seventh_id(gen_scale scale, int degree)
{
    int tri = triad_id(scale, degree);
    int s = interval_from(scale, degree, 6);
    if (tri < 0 || s < 0)
        return -1;
    if (tri == GEN_NID_MAJOR && s == 11)
        return GEN_NID_MAJ7;
    if (tri == GEN_NID_MAJOR && s == 10)
        return GEN_NID_DOM7;
    if (tri == GEN_NID_MINOR && s == 10)
        return GEN_NID_MIN7;
    if (tri == GEN_NID_MINOR && s == 11)
        return GEN_NID_MINMAJ7;
    return -1;
}

static const uint8_t SEEDED_IDS[] = {
    GEN_NID_MAJOR, GEN_NID_MINOR, GEN_NID_SUS2, GEN_NID_SUS4,
    GEN_NID_MAJ7, GEN_NID_MIN7, GEN_NID_DOM7
};

/* First byte is the degree count. The rest are scale degrees, not qualities. */
static const uint8_t PROGS[7][5] = {
    {4, 0, 4, 5, 3}, /* I V vi IV */
    {4, 0, 5, 3, 4}, /* I vi IV V */
    {3, 1, 4, 0, 0}, /* ii V I */
    {4, 0, 5, 1, 4}, /* I vi ii V */
    {4, 0, 5, 2, 6}, /* i VI III VII */
    {4, 0, 3, 5, 6}, /* i iv VI VII */
    {3, 0, 3, 4, 0}  /* i iv v */
};

static int native_units(uint32_t ticks, uint32_t tps)
{
    uint32_t n;
    if (tps == 0 || ticks == 0 || ticks > 0xffffffffu / 60u)
        return 0;
    n = (ticks * 60u + tps / 2u) / tps;
    if (n < 1 || n > GEN_NATIVE_DUR_MAX)
        return 0;
    return (int)n;
}

static void fail(gen_buf *out)
{
    if (out)
        out->count = 0;
}

gen_status gen_native_progression(const gen_nchord_cfg *cfg, gen_buf *out,
                                  uint8_t *ids, uint32_t ids_cap)
{
    gen_rng rng;
    const uint8_t *prog = 0;
    uint8_t plen = 0, span, at;
    int ndeg, i;
    uint32_t need;

    if (!cfg || !out || !out->events || !ids)
        return GEN_ERR_PARAM;
    fail(out);
    if ((unsigned)cfg->scale >= GEN_SCALE_COUNT)
        return GEN_ERR_PARAM;
    if ((unsigned)cfg->prog >= GEN_NPROG_COUNT || (unsigned)cfg->mode >= GEN_NMODE_COUNT)
        return GEN_ERR_PARAM;
    if (cfg->steps < 1 || cfg->steps > 64)
        return GEN_ERR_PARAM;
    if (cfg->interval != 4 && cfg->interval != 8 && cfg->interval != 16 && cfg->interval != 32)
        return GEN_ERR_PARAM;
    if (cfg->interval > cfg->steps)
        return GEN_ERR_PARAM;
    if (cfg->velocity < 1 || cfg->velocity > 127 || cfg->channel > 15)
        return GEN_ERR_PARAM;
    if (cfg->hold > GEN_NHOLD_RETRIG)
        return GEN_ERR_PARAM;
    if (cfg->ticks_per_step == 0 || cfg->gate_ticks == 0 || cfg->gate_ticks > cfg->ticks_per_step)
        return GEN_ERR_TIMING;
    if (cfg->mode == GEN_NMODE_FIXED && !gen_native_id_ok(cfg->fixed_id))
        return GEN_ERR_PARAM;
    ndeg = gen_scale_degree_count(cfg->scale);
    if (ndeg < 1)
        return GEN_ERR_PARAM;
    if (cfg->prog != GEN_NPROG_RANDOM) {
        prog = PROGS[cfg->prog];
        plen = prog[0];
        for (i = 0; i < plen; i++) {
            if (prog[1 + i] >= ndeg)
                return GEN_ERR_UNSUPPORTED;
        }
    }

    need = cfg->hold == GEN_NHOLD_RETRIG ? cfg->steps : (uint32_t)((cfg->steps + cfg->interval - 1) / cfg->interval);
    if (need == 0 || need > ids_cap || need > out->capacity) {
        fail(out);
        return GEN_ERR_CAPACITY;
    }

    gen_rng_seed(&rng, cfg->seed);
    at = 0;
    while (at < cfg->steps) {
        uint8_t degree, id;
        int root, qual;
        uint32_t span_steps, ticks, k;
        span = cfg->interval;
        if ((uint16_t)at + span > cfg->steps)
            span = (uint8_t)(cfg->steps - at);
        if (cfg->prog == GEN_NPROG_RANDOM)
            degree = (uint8_t)gen_rng_bound(&rng, (uint32_t)ndeg);
        else
            degree = prog[1 + ((at / cfg->interval) % plen)];
        root = degree_midi(cfg->scale, cfg->root, degree);
        if (root < 0) {
            fail(out);
            return GEN_ERR_PITCH;
        }
        if (cfg->mode == GEN_NMODE_FIXED)
            qual = cfg->fixed_id;
        else if (cfg->mode == GEN_NMODE_SEEDED)
            qual = SEEDED_IDS[gen_rng_bound(&rng, (uint32_t)(sizeof SEEDED_IDS))];
        else if (cfg->mode == GEN_NMODE_SEVENTH)
            qual = seventh_id(cfg->scale, degree);
        else
            qual = triad_id(cfg->scale, degree);
        if (!gen_native_id_ok(qual)) {
            fail(out);
            return GEN_ERR_UNSUPPORTED;
        }
        id = (uint8_t)qual;
        span_steps = cfg->hold == GEN_NHOLD_RETRIG ? 1u : span;
        ticks = span_steps * cfg->gate_ticks;
        if (!native_units(ticks, cfg->ticks_per_step)) {
            fail(out);
            return GEN_ERR_TIMING;
        }
        if (cfg->hold == GEN_NHOLD_RETRIG) {
            for (k = 0; k < span; k++) {
                gen_event ev;
                ev.tick = (uint32_t)(at + k) * cfg->ticks_per_step;
                ev.note = (uint8_t)root;
                ev.velocity = cfg->velocity;
                ev.channel = cfg->channel;
                ev._pad = 0;
                ev.duration = cfg->gate_ticks;
                if (out->count >= ids_cap || gen_buf_push(out, &ev) != GEN_OK) {
                    fail(out);
                    return GEN_ERR_CAPACITY;
                }
                ids[out->count - 1] = id;
            }
        } else {
            gen_event ev;
            ev.tick = (uint32_t)at * cfg->ticks_per_step;
            ev.note = (uint8_t)root;
            ev.velocity = cfg->velocity;
            ev.channel = cfg->channel;
            ev._pad = 0;
            ev.duration = ticks;
            if (out->count >= ids_cap || gen_buf_push(out, &ev) != GEN_OK) {
                fail(out);
                return GEN_ERR_CAPACITY;
            }
            ids[out->count - 1] = id;
        }
        at = (uint8_t)(at + span);
    }
    if (gen_buf_validate(out, 1) != GEN_OK) {
        fail(out);
        return GEN_ERR_MONO;
    }
    return GEN_OK;
}
