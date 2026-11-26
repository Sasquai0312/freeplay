/* Turing register phrase generator.
 *
 * A cell is a MIDI note or GEN_TURING_REST (0xFF). Rest is stored in the
 * register, so lock and 0% evolution repeat pitch and rest together.
 * Evolution rewrites the cell that just played. 0% never rewrites. 100%
 * rewrites on every advance (the new draw may match the old one). Lock
 * advances the read position and leaves every cell alone.
 * Generate copies src into next and does not write src. */
#include "genesis.h"

static void copy_pod(void *dst, const void *src, unsigned bytes)
{
    volatile uint32_t *d = (volatile uint32_t *)dst;
    const volatile uint32_t *s = (const volatile uint32_t *)src;
    unsigned i, n = bytes >> 2;
    for (i = 0; i < n; i++)
        d[i] = s[i];
}

static int span_count(uint8_t scale, int root, int lo, int hi)
{
    int n = 0, p;
    for (p = lo; p <= hi; p++)
        if (gen_scale_contains((gen_scale)scale, root, p))
            n++;
    return n;
}

static int span_at(uint8_t scale, int root, int lo, int hi, int index)
{
    int n = 0, p;
    for (p = lo; p <= hi; p++) {
        if (!gen_scale_contains((gen_scale)scale, root, p))
            continue;
        if (n == index)
            return p;
        n++;
    }
    return -1;
}

static gen_status check_cfg(const gen_turing_cfg *c, int need_pitches)
{
    if (!c)
        return GEN_ERR_PARAM;
    if (c->length < 1 || c->length > GEN_TURING_MAX_LEN)
        return GEN_ERR_PARAM;
    if (c->steps < 1 || c->steps > GEN_TURING_MAX_STEPS)
        return GEN_ERR_PARAM;
    if (c->evolve_prob > 100 || c->rest_prob > 100)
        return GEN_ERR_PARAM;
    if (c->velocity < 1 || c->velocity > 127 || c->channel > 15)
        return GEN_ERR_PARAM;
    if ((unsigned)c->scale >= GEN_SCALE_COUNT)
        return GEN_ERR_PARAM;
    if (c->root < 0 || c->root > 127)
        return GEN_ERR_PITCH;
    if (c->pitch_min < 0 || c->pitch_max > 127 || c->pitch_min > c->pitch_max)
        return GEN_ERR_PITCH;
    if (c->ticks_per_step == 0 || c->ticks_per_step > 1000000u)
        return GEN_ERR_PARAM;
    if (c->ticks_per_step > UINT32_MAX / c->steps)
        return GEN_ERR_OVERFLOW;
    if (!gen_native_duration_ok(c->gate, c->ticks_per_step))
        return GEN_ERR_TIMING;
    if (need_pitches && span_count(c->scale, c->root, c->pitch_min, c->pitch_max) == 0)
        return GEN_ERR_PITCH;
    return GEN_OK;
}

static uint8_t draw_cell(gen_turing *st)
{
    const gen_turing_cfg *c = &st->cfg;
    int np, index;
    if (c->rest_prob == 100)
        return GEN_TURING_REST;
    if (c->rest_prob > 0 && gen_rng_bound(&st->rng, 100) < c->rest_prob)
        return GEN_TURING_REST;
    np = span_count(c->scale, c->root, c->pitch_min, c->pitch_max);
    index = (int)gen_rng_bound(&st->rng, (uint32_t)np);
    return (uint8_t)span_at(c->scale, c->root, c->pitch_min, c->pitch_max, index);
}

static gen_status fill(gen_turing *st, const gen_turing_cfg *cfg)
{
    gen_status stc;
    uint8_t i;
    int need = cfg->rest_prob < 100;
    stc = check_cfg(cfg, need);
    if (stc != GEN_OK)
        return stc;
    st->ready = 0;
    copy_pod(&st->cfg, cfg, sizeof st->cfg);
    st->length = cfg->length;
    st->pos = 0;
    st->_pad = 0;
    for (i = 0; i < GEN_TURING_MAX_LEN; i++)
        st->cells[i] = 0;
    gen_rng_seed(&st->rng, cfg->seed);
    for (i = 0; i < cfg->length; i++)
        st->cells[i] = draw_cell(st);
    st->ready = 1;
    return GEN_OK;
}

gen_status gen_turing_init(gen_turing *st, const gen_turing_cfg *cfg)
{
    if (!st)
        return GEN_ERR_PARAM;
    return fill(st, cfg);
}

gen_status gen_turing_seed(gen_turing *st, uint32_t seed)
{
    gen_turing_cfg cfg;
    if (!st || !st->ready)
        return GEN_ERR_PARAM;
    copy_pod(&cfg, &st->cfg, sizeof cfg);
    cfg.seed = seed;
    return fill(st, &cfg);
}

gen_status gen_turing_reset(gen_turing *st)
{
    gen_turing_cfg cfg;
    if (!st || !st->ready)
        return GEN_ERR_PARAM;
    copy_pod(&cfg, &st->cfg, sizeof cfg);
    return fill(st, &cfg);
}

gen_status gen_turing_copy(gen_turing *dst, const gen_turing *src)
{
    if (!dst || !src)
        return GEN_ERR_PARAM;
    copy_pod(dst, src, sizeof *dst);
    return GEN_OK;
}

static int cell_ok(const gen_turing *st, uint8_t cell)
{
    if (cell == GEN_TURING_REST)
        return 1;
    if (cell > 127)
        return 0;
    if (cell < st->cfg.pitch_min || cell > st->cfg.pitch_max)
        return 0;
    return gen_scale_contains((gen_scale)st->cfg.scale, st->cfg.root, cell);
}

static uint16_t note_budget(const gen_turing *st, const gen_turing_cfg *c)
{
    uint16_t i, notes = 0, limit = c->steps;
    if (!c->locked && c->evolve_prob > 0 &&
        !(c->evolve_prob == 100 && c->rest_prob == 100))
        return c->steps;
    if (!c->locked && c->evolve_prob == 100 && c->steps > st->length)
        limit = st->length;
    for (i = 0; i < limit; i++) {
        uint8_t idx = (uint8_t)((st->pos + i) % st->length);
        if (st->cells[idx] != GEN_TURING_REST)
            notes++;
    }
    return notes;
}

gen_status gen_turing_generate(const gen_turing *src, const gen_turing_cfg *cfg,
                               gen_buf *out, gen_turing *next)
{
    gen_turing work;
    gen_turing_cfg play;
    gen_status stc;
    uint16_t i, need;
    uint32_t saved;
    uint8_t k;
    if (!src || !out || !next || !out->events)
        return GEN_ERR_PARAM;
    if (!src->ready || src->length < 1 || src->length > GEN_TURING_MAX_LEN || src->pos >= src->length)
        return GEN_ERR_PARAM;
    for (k = 0; k < src->length; k++)
        if (!cell_ok(src, src->cells[k]))
            return GEN_ERR_PITCH;
    if (cfg)
        copy_pod(&play, cfg, sizeof play);
    else
        copy_pod(&play, &src->cfg, sizeof play);
    /* Scale and pitch bounds stay on the initialized register. A playback
     * override may change steps, gate, timing, velocity, evolution, rest, and lock. */
    if (cfg && (cfg->scale != src->cfg.scale || cfg->root != src->cfg.root ||
                cfg->pitch_min != src->cfg.pitch_min || cfg->pitch_max != src->cfg.pitch_max ||
                cfg->length != src->length))
        return GEN_ERR_PARAM;
    play.scale = src->cfg.scale;
    play.root = src->cfg.root;
    play.pitch_min = src->cfg.pitch_min;
    play.pitch_max = src->cfg.pitch_max;
    play.length = src->length;
    stc = check_cfg(&play, 0);
    if (stc != GEN_OK)
        return stc;
    if (!play.locked && play.evolve_prob > 0 && play.rest_prob < 100 &&
        span_count(play.scale, play.root, play.pitch_min, play.pitch_max) == 0)
        return GEN_ERR_PITCH;
    copy_pod(&work, src, sizeof work);
    work.cfg.rest_prob = play.rest_prob;
    need = note_budget(src, &play);
    if (out->count > out->capacity || out->capacity - out->count < need)
        return GEN_ERR_CAPACITY;
    saved = out->count;
    for (i = 0; i < play.steps; i++) {
        uint8_t cell = work.cells[work.pos];
        if (cell != GEN_TURING_REST) {
            gen_event ev;
            ev.tick = (uint32_t)i * play.ticks_per_step;
            ev.note = cell;
            ev.velocity = play.velocity;
            ev.channel = play.channel;
            ev._pad = 0;
            ev.duration = play.gate;
            stc = gen_buf_push(out, &ev);
            if (stc != GEN_OK) {
                out->count = saved;
                return stc;
            }
        }
        if (!play.locked && play.evolve_prob > 0) {
            int rewrite = play.evolve_prob == 100 ||
                          gen_rng_bound(&work.rng, 100) < play.evolve_prob;
            if (rewrite)
                work.cells[work.pos] = draw_cell(&work);
        }
        work.pos = (uint8_t)((work.pos + 1u) % work.length);
    }
    copy_pod(&work.cfg, &src->cfg, sizeof work.cfg);
    work.ready = 1;
    work.length = src->length;
    copy_pod(next, &work, sizeof work);
    return GEN_OK;
}
