/* First-order Markov melody on scale degrees.
 *
 * Preset styles are integer heuristics, not corpus estimates.
 * A row whose weights are all zero falls back to a uniform draw over the
 * scale degrees. Rests (rest_prob) skip a step and do not change the degree.
 * The next pitch is the in-range pitch of the chosen degree closest to the
 * previous note. octave_motion == 0 tries the current octave first, then
 * the same closest-pitch rule if that degree is missing there.
 * Learned counts replace the model only after the whole phrase is accepted.
 */
#include "genesis.h"

static void copy_pod(void *dst, const void *src, unsigned bytes)
{
    volatile uint32_t *d = (volatile uint32_t *)dst;
    const volatile uint32_t *s = (const volatile uint32_t *)src;
    unsigned i, n = bytes >> 2;
    for (i = 0; i < n; i++)
        d[i] = s[i];
}

static int degree_of(uint8_t scale, int root, int note)
{
    int n, d;
    if (note < 0 || note > 127)
        return -1;
    n = gen_scale_degree_count((gen_scale)scale);
    for (d = 0; d < n; d++) {
        int m = gen_scale_degree_to_midi((gen_scale)scale, root, d, 0);
        int pc_m = (m - root) % 12;
        int pc_n = (note - root) % 12;
        if (pc_m < 0)
            pc_m += 12;
        if (pc_n < 0)
            pc_n += 12;
        if (pc_m == pc_n)
            return d;
    }
    return -1;
}

static int shortest(int from, int to, int n)
{
    int d = to - from;
    if (d > n / 2)
        d -= n;
    if (d < -(n / 2))
        d += n;
    return d;
}

uint8_t gen_markov_style_weight(uint8_t style, uint8_t n_deg, uint8_t from, uint8_t to)
{
    int d, ad, w;
    if (n_deg < 1 || n_deg > GEN_MARKOV_MAX_DEG || from >= n_deg || to >= n_deg)
        return 0;
    if (style > GEN_MARKOV_EXPLORATORY)
        return 0;
    d = shortest((int)from, (int)to, (int)n_deg);
    ad = d < 0 ? -d : d;
    switch (style) {
    case GEN_MARKOV_STEPWISE:
        if (ad == 0)
            return 12;
        if (ad == 1)
            return 40;
        if (ad == 2)
            return 8;
        return 1;
    case GEN_MARKOV_MOTIF:
        if (d == 0)
            return 28;
        if (d == 2 || d == -2)
            return 22;
        if (d == 1 || d == -1)
            return 10;
        return 2;
    case GEN_MARKOV_BASSLINE:
        w = 2;
        if (to == 0)
            w += 36;
        if (n_deg >= 5 && to == 4)
            w += 24;
        if (ad <= 1)
            w += 8;
        if (to <= 2)
            w += 6;
        return (uint8_t)w;
    case GEN_MARKOV_EXPLORATORY:
    default:
        return ad == 0 ? 3 : 10;
    }
}

static void zero_model(gen_markov_model *m)
{
    int i, j;
    for (i = 0; i < GEN_MARKOV_MAX_DEG; i++)
        for (j = 0; j < GEN_MARKOV_MAX_DEG; j++)
            m->w[i][j] = 0;
    m->n_deg = 0;
    m->style = 0;
    m->ready = 0;
    m->origin = 0;
}

gen_status gen_markov_preset(gen_markov_model *m, uint8_t scale, uint8_t style)
{
    int n, i, j;
    if (!m || (unsigned)scale >= GEN_SCALE_COUNT || style > GEN_MARKOV_EXPLORATORY)
        return GEN_ERR_PARAM;
    n = gen_scale_degree_count((gen_scale)scale);
    if (n < 1 || n > GEN_MARKOV_MAX_DEG)
        return GEN_ERR_PARAM;
    zero_model(m);
    m->n_deg = (uint8_t)n;
    m->style = style;
    m->origin = 0;
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            m->w[i][j] = gen_markov_style_weight(style, (uint8_t)n, (uint8_t)i, (uint8_t)j);
    m->ready = 1;
    return GEN_OK;
}

gen_status gen_markov_add(gen_markov_model *m, uint8_t from, uint8_t to)
{
    if (!m || !m->ready)
        return GEN_ERR_PARAM;
    if (from >= m->n_deg || to >= m->n_deg)
        return GEN_ERR_PARAM;
    if (m->w[from][to] == 65535u)
        return GEN_ERR_OVERFLOW;
    m->w[from][to]++;
    m->style = GEN_MARKOV_LEARNED;
    return GEN_OK;
}

gen_status gen_markov_learn(gen_markov_model *m, uint8_t scale, int root,
                            const gen_buf *src, uint32_t gap_ticks,
                            gen_markov_model *scratch)
{
    int n, prev, i;
    uint32_t prev_tick = 0;
    if (!m || !scratch || scratch == m || !src)
        return GEN_ERR_PARAM;
    if ((unsigned)scale >= GEN_SCALE_COUNT || root < 0 || root > 127)
        return GEN_ERR_PARAM;
    n = gen_scale_degree_count((gen_scale)scale);
    if (n < 1 || n > GEN_MARKOV_MAX_DEG)
        return GEN_ERR_PARAM;
    if (src->count > 0 && !src->events)
        return GEN_ERR_PARAM;
    if (src->count == 0)
        return GEN_ERR_EMPTY;
    for (i = 0; i < (int)src->count; i++) {
        const gen_event *e = &src->events[i];
        if (e->note > 127)
            return GEN_ERR_PITCH;
        if (e->velocity < 1 || e->velocity > 127 || e->duration == 0)
            return GEN_ERR_TIMING;
        if (i > 0 && e->tick <= src->events[i - 1].tick)
            return GEN_ERR_TIMING;
    }
    zero_model(scratch);
    scratch->n_deg = (uint8_t)n;
    scratch->style = GEN_MARKOV_LEARNED;
    scratch->ready = 1;
    prev = -1;
    for (i = 0; i < (int)src->count; i++) {
        const gen_event *e = &src->events[i];
        int deg = degree_of(scale, root, e->note);
        if (deg < 0) {
            prev = -1;
            continue;
        }
        if (prev >= 0) {
            int broken = gap_ticks > 0 && (e->tick - prev_tick) > gap_ticks;
            if (!broken) {
                if (scratch->w[prev][deg] == 65535u)
                    return GEN_ERR_OVERFLOW;
                scratch->w[prev][deg]++;
            }
        }
        prev = deg;
        scratch->origin = (uint8_t)deg;
        prev_tick = e->tick;
    }
    copy_pod(m, scratch, sizeof *m);
    return GEN_OK;
}

static gen_status check_gen(const gen_markov_model *m, const gen_markov_cfg *c)
{
    if (!m || !c || !m->ready)
        return GEN_ERR_PARAM;
    if (m->n_deg < 1 || m->n_deg > GEN_MARKOV_MAX_DEG)
        return GEN_ERR_PARAM;
    if (m->n_deg != (uint8_t)gen_scale_degree_count((gen_scale)c->scale))
        return GEN_ERR_PARAM;
    if ((unsigned)c->scale >= GEN_SCALE_COUNT)
        return GEN_ERR_PARAM;
    if (c->steps < 1 || c->steps > GEN_TURING_MAX_STEPS)
        return GEN_ERR_PARAM;
    if (c->rest_prob > 100 || c->velocity < 1 || c->velocity > 127 || c->channel > 15)
        return GEN_ERR_PARAM;
    if (c->root < 0 || c->root > 127)
        return GEN_ERR_PITCH;
    if (c->pitch_min < 0 || c->pitch_max > 127 || c->pitch_min > c->pitch_max)
        return GEN_ERR_PITCH;
    if (c->initial_degree >= m->n_deg)
        return GEN_ERR_PARAM;
    if (c->ticks_per_step == 0 || c->ticks_per_step > 1000000u)
        return GEN_ERR_PARAM;
    if (c->ticks_per_step > UINT32_MAX / c->steps)
        return GEN_ERR_OVERFLOW;
    if (!gen_native_duration_ok(c->gate, c->ticks_per_step))
        return GEN_ERR_TIMING;
    return GEN_OK;
}

static int pick_degree(const gen_markov_model *m, int from, gen_rng *rng)
{
    uint32_t total = 0, acc = 0, r;
    int j;
    for (j = 0; j < m->n_deg; j++)
        total += m->w[from][j];
    if (total == 0)
        return (int)gen_rng_bound(rng, m->n_deg);
    r = gen_rng_bound(rng, total);
    for (j = 0; j < m->n_deg; j++) {
        acc += m->w[from][j];
        if (r < acc)
            return j;
    }
    return (int)m->n_deg - 1;
}

static int pitch_of(const gen_markov_cfg *c, int degree, int prev, int same_oct_only)
{
    int p, best = -1, best_d = 100000, center;
    center = prev >= 0 ? prev : (c->pitch_min + c->pitch_max) / 2;
    for (p = c->pitch_min; p <= c->pitch_max; p++) {
        int dist;
        if (degree_of(c->scale, c->root, p) != degree)
            continue;
        if (same_oct_only && prev >= 0 && (p / 12) != (prev / 12))
            continue;
        dist = p - center;
        if (dist < 0)
            dist = -dist;
        if (best < 0 || dist < best_d || (dist == best_d && p < best)) {
            best = p;
            best_d = dist;
        }
    }
    return best;
}

static int place(const gen_markov_cfg *c, int degree, int prev)
{
    int dist, n;
    n = gen_scale_degree_count((gen_scale)c->scale);
    for (dist = 0; dist < n; dist++) {
        int a = degree + dist;
        int b = degree - dist;
        int pa, pb;
        if (a >= n)
            a -= n;
        if (b < 0)
            b += n;
        pa = pitch_of(c, a, prev, !c->octave_motion);
        if (pa < 0 && !c->octave_motion)
            pa = pitch_of(c, a, prev, 0);
        if (pa >= 0)
            return pa;
        if (dist == 0)
            continue;
        pb = pitch_of(c, b, prev, !c->octave_motion);
        if (pb < 0 && !c->octave_motion)
            pb = pitch_of(c, b, prev, 0);
        if (pb >= 0)
            return pb;
    }
    return -1;
}

gen_status gen_markov_generate(const gen_markov_model *m, const gen_markov_cfg *cfg, gen_buf *out)
{
    gen_rng rng;
    gen_status st;
    uint16_t i, need;
    uint32_t saved;
    int deg, prev = -1;
    if (!out || !out->events)
        return GEN_ERR_PARAM;
    st = check_gen(m, cfg);
    if (st != GEN_OK)
        return st;
    if (place(cfg, cfg->initial_degree >= 0 ? cfg->initial_degree : 0, -1) < 0 &&
        place(cfg, 0, -1) < 0)
        return GEN_ERR_PITCH;
    need = cfg->rest_prob == 100 ? 0 : cfg->steps;
    if (out->count > out->capacity || out->capacity - out->count < need)
        return GEN_ERR_CAPACITY;
    gen_rng_seed(&rng, cfg->seed);
    if (cfg->initial_degree >= 0)
        deg = cfg->initial_degree;
    else if (m->style == GEN_MARKOV_LEARNED)
        deg = m->origin;
    else
        deg = (int)gen_rng_bound(&rng, m->n_deg);
    saved = out->count;
    for (i = 0; i < cfg->steps; i++) {
        int sounding = cfg->rest_prob == 0 ||
                       (cfg->rest_prob < 100 && gen_rng_bound(&rng, 100) >= cfg->rest_prob);
        if (sounding) {
            gen_event ev;
            int note = place(cfg, deg, prev);
            if (note < 0) {
                out->count = saved;
                return GEN_ERR_PITCH;
            }
            ev.tick = (uint32_t)i * cfg->ticks_per_step;
            ev.note = (uint8_t)note;
            ev.velocity = cfg->velocity;
            ev.channel = cfg->channel;
            ev._pad = 0;
            ev.duration = cfg->gate;
            st = gen_buf_push(out, &ev);
            if (st != GEN_OK) {
                out->count = saved;
                return st;
            }
            prev = note;
            deg = pick_degree(m, deg, &rng);
        }
    }
    return GEN_OK;
}
