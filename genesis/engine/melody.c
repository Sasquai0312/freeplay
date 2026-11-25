#include "genesis.h"

static gen_status check_melody_cfg(const gen_melody_cfg *cfg, const gen_rhythm *rhythm)
{
    if (!cfg || !rhythm || rhythm->step_count == 0)
        return GEN_ERR_PARAM;
    if (cfg->root < 0 || cfg->root > 127)
        return GEN_ERR_PITCH;
    if (cfg->pitch_min < 0 || cfg->pitch_max > 127 || cfg->pitch_min > cfg->pitch_max)
        return GEN_ERR_PITCH;
    if (cfg->vel_min < 1 || cfg->vel_max > 127 || cfg->vel_min > cfg->vel_max)
        return GEN_ERR_PARAM;
    if (cfg->channel > 15)
        return GEN_ERR_PARAM;
    if ((unsigned)cfg->scale >= GEN_SCALE_COUNT)
        return GEN_ERR_PARAM;
    return GEN_OK;
}

static int collect_pitches(const gen_melody_cfg *cfg, int *pitches, int maxn)
{
    int n = 0, p;
    for (p = cfg->pitch_min; p <= cfg->pitch_max && n < maxn; p++)
        if (gen_scale_contains(cfg->scale, cfg->root, p))
            pitches[n++] = p;
    return n;
}

static uint8_t pick_vel(gen_rng *rng, const gen_melody_cfg *cfg)
{
    return (uint8_t)gen_rng_range(rng, cfg->vel_min, cfg->vel_max);
}

static uint32_t note_dur(const gen_melody_cfg *cfg, const gen_rhythm *r)
{
    return cfg->duration ? cfg->duration : r->ticks_per_step;
}

static gen_status emit_from_pitches(const gen_melody_cfg *cfg, const gen_rhythm *rhythm, gen_buf *out,
                                    int (*chooser)(void *ctx, int *pitches, int np, int prev_idx),
                                    void *ctx)
{
    gen_rng rng;
    int pitches[128];
    int np, i, prev = -1;
    uint32_t saved;
    gen_status st;
    st = check_melody_cfg(cfg, rhythm);
    if (st != GEN_OK)
        return st;
    np = collect_pitches(cfg, pitches, 128);
    if (np == 0)
        return GEN_ERR_PITCH;
    gen_rng_seed(&rng, cfg->seed);
    saved = out->count;
    for (i = 0; i < (int)rhythm->step_count; i++) {
        gen_event ev;
        int idx, pitch;
        if (!rhythm->steps[i])
            continue;
        idx = chooser(ctx ? ctx : &rng, pitches, np, prev);
        if (idx < 0 || idx >= np) {
            out->count = saved;
            return GEN_ERR_PARAM;
        }
        pitch = pitches[idx];
        prev = idx;
        ev.tick = (uint32_t)i * rhythm->ticks_per_step;
        ev.note = (uint8_t)pitch;
        ev.velocity = pick_vel(&rng, cfg);
        ev.channel = cfg->channel;
        ev._pad = 0;
        ev.duration = note_dur(cfg, rhythm);
        st = gen_buf_push(out, &ev);
        if (st != GEN_OK) {
            out->count = saved;
            return st;
        }
    }
    return GEN_OK;
}

static int choose_uniform(void *ctx, int *pitches, int np, int prev_idx)
{
    gen_rng *rng = (gen_rng *)ctx;
    (void)pitches;
    (void)prev_idx;
    return (int)gen_rng_bound(rng, (uint32_t)np);
}

gen_status gen_melody_uniform(const gen_melody_cfg *cfg, const gen_rhythm *rhythm, gen_buf *out)
{
    return emit_from_pitches(cfg, rhythm, out, choose_uniform, 0);
}

typedef struct {
    gen_rng *rng;
    const uint8_t *w;
    int nw;
} weighted_ctx;

static int choose_weighted(void *ctx, int *pitches, int np, int prev_idx)
{
    weighted_ctx *w = (weighted_ctx *)ctx;
    uint32_t sum = 0, pick, acc = 0;
    int i, deg, n_deg;
    (void)prev_idx;
    /* Map pitch index to scale degree via pitch class relative to first pitch's degree — simpler:
     * weight by degree within scale for each pitch. */
    n_deg = w->nw;
    for (i = 0; i < np; i++) {
        deg = i % (n_deg > 0 ? n_deg : np);
        sum += w->w && n_deg > 0 ? w->w[deg % n_deg] : 1;
    }
    if (sum == 0)
        return (int)gen_rng_bound(w->rng, (uint32_t)np);
    pick = gen_rng_bound(w->rng, sum);
    for (i = 0; i < np; i++) {
        deg = i % (n_deg > 0 ? n_deg : np);
        acc += w->w && n_deg > 0 ? w->w[deg % n_deg] : 1;
        if (pick < acc)
            return i;
    }
    (void)pitches;
    return np - 1;
}

gen_status gen_melody_weighted(const gen_melody_cfg *cfg, const gen_rhythm *rhythm, gen_buf *out)
{
    gen_rng rng;
    weighted_ctx wc;
    uint8_t ones[16];
    int i, nd;
    gen_status st = check_melody_cfg(cfg, rhythm);
    if (st != GEN_OK)
        return st;
    gen_rng_seed(&rng, cfg->seed);
    nd = gen_scale_degree_count(cfg->scale);
    if (nd > 16)
        nd = 16;
    if (cfg->degree_weights) {
        wc.w = cfg->degree_weights;
        wc.nw = nd;
    } else {
        for (i = 0; i < nd; i++)
            ones[i] = 1;
        wc.w = ones;
        wc.nw = nd;
    }
    wc.rng = &rng;
    return emit_from_pitches(cfg, rhythm, out, choose_weighted, &wc);
}

typedef struct {
    gen_rng *rng;
    const gen_melody_cfg *cfg;
} walk_ctx;

static int choose_walk(void *ctx, int *pitches, int np, int prev_idx)
{
    walk_ctx *w = (walk_ctx *)ctx;
    int max_iv = w->cfg->max_interval ? w->cfg->max_interval : 2;
    int lo, hi, i, best_lo, n_cand, pick;
    int cands[128];
    if (prev_idx < 0)
        return (int)gen_rng_bound(w->rng, (uint32_t)np);
    if (gen_rng_bound(w->rng, 100) < w->cfg->repeat_prob)
        return prev_idx;
    lo = prev_idx - max_iv;
    hi = prev_idx + max_iv;
    if (lo < 0)
        lo = 0;
    if (hi >= np)
        hi = np - 1;
    n_cand = 0;
    for (i = lo; i <= hi; i++)
        if (i != prev_idx)
            cands[n_cand++] = i;
    if (n_cand == 0)
        return prev_idx;
    pick = cands[gen_rng_bound(w->rng, (uint32_t)n_cand)];
    (void)best_lo;
    (void)pitches;
    return pick;
}

gen_status gen_melody_walk(const gen_melody_cfg *cfg, const gen_rhythm *rhythm, gen_buf *out)
{
    gen_rng rng;
    walk_ctx wc;
    gen_status st = check_melody_cfg(cfg, rhythm);
    if (st != GEN_OK)
        return st;
    gen_rng_seed(&rng, cfg->seed);
    wc.rng = &rng;
    wc.cfg = cfg;
    return emit_from_pitches(cfg, rhythm, out, choose_walk, &wc);
}
