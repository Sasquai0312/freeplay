#include "genesis_evo.h"

static void copy_pod(void *dst, const void *src, unsigned bytes)
{
    volatile uint32_t *d = (volatile uint32_t *)dst;
    const volatile uint32_t *s = (const volatile uint32_t *)src;
    unsigned i, n = bytes >> 2;
    for (i = 0; i < n; i++)
        d[i] = s[i];
}

static void zero_bytes(void *p, unsigned n)
{
    unsigned i;
    volatile uint8_t *d = (volatile uint8_t *)p;
    for (i = 0; i < n; i++)
        d[i] = 0;
}

static uint32_t mix(uint32_t h, uint32_t v)
{
    h ^= v;
    h *= 16777619u;
    return h;
}

static uint32_t turing_sig(const gen_turing_cfg *c)
{
    uint32_t h = 2166136261u;
    h = mix(h, c->seed);
    h = mix(h, (uint32_t)c->root);
    h = mix(h, (uint32_t)(uint16_t)c->pitch_min);
    h = mix(h, (uint32_t)(uint16_t)c->pitch_max);
    h = mix(h, c->scale);
    h = mix(h, c->length);
    h = mix(h, c->rest_prob);
    return h;
}

static void fail(gen_evo *e, gen_status st)
{
    e->last_status = (int16_t)st;
    e->phase = GEN_EVO_ERROR;
    e->proposed_ok = 0;
}

static gen_status publish(gen_evo *e, gen_buf *out, unsigned steps, uint32_t tps, uint32_t bind, uint8_t mode)
{
    gen_status st;
    if (!out || out->count == 0) {
        if (out)
            out->count = 0;
        fail(e, GEN_ERR_EMPTY);
        return GEN_ERR_EMPTY;
    }
    st = gen_evo_audit(out, steps, tps);
    if (st != GEN_OK) {
        out->count = 0;
        fail(e, st);
        return st;
    }
    e->mode = mode;
    e->preview_bind = bind;
    e->live_bind = bind;
    e->phase = GEN_EVO_VALID;
    e->last_status = GEN_OK;
    return GEN_OK;
}

void gen_evo_init(gen_evo *e)
{
    if (!e)
        return;
    zero_bytes(e, sizeof *e);
}

void gen_evo_set_mode(gen_evo *e, uint8_t mode)
{
    if (!e)
        return;
    e->mode = mode;
    e->proposed_ok = 0;
    e->pending_ok = 0;
    if (e->phase != GEN_EVO_APPLIED)
        e->phase = GEN_EVO_EMPTY;
}

void gen_evo_set_live(gen_evo *e, uint32_t bind)
{
    if (!e)
        return;
    e->live_bind = bind;
    if (e->phase == GEN_EVO_VALID && bind != e->preview_bind)
        e->phase = GEN_EVO_STALE;
}

void gen_evo_discard(gen_evo *e)
{
    if (!e)
        return;
    e->proposed_ok = 0;
    e->pending_ok = 0;
    if (e->phase != GEN_EVO_APPLIED)
        e->phase = GEN_EVO_EMPTY;
}

uint32_t gen_evo_hash(uint32_t h, const void *bytes, uint32_t n)
{
    const uint8_t *p = (const uint8_t *)bytes;
    uint32_t i;
    if (!p)
        return h;
    for (i = 0; i < n; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

gen_status gen_evo_native_steps(unsigned steps)
{
    if (steps < 1 || steps > GEN_EVO_NATIVE_MAX)
        return GEN_ERR_PARAM;
    return GEN_OK;
}

gen_status gen_evo_turing_reinit(gen_evo *e, const gen_turing_cfg *cfg)
{
    gen_status st;
    if (!e || !cfg)
        return GEN_ERR_PARAM;
    st = gen_evo_native_steps(cfg->steps);
    if (st != GEN_OK)
        return st;
    st = gen_turing_init(&e->committed, cfg);
    if (st != GEN_OK) {
        e->last_status = (int16_t)st;
        return st;
    }
    e->turing_sig = turing_sig(cfg);
    e->proposed_ok = 0;
    e->mode = GEN_EVO_TURING;
    if (e->phase != GEN_EVO_APPLIED)
        e->phase = GEN_EVO_EMPTY;
    e->last_status = GEN_OK;
    return GEN_OK;
}

gen_status gen_evo_turing_preview(gen_evo *e, const gen_turing_cfg *cfg, gen_buf *out, uint32_t bind)
{
    gen_status st;
    uint32_t sig;
    if (!e || !cfg || !out)
        return GEN_ERR_PARAM;
    st = gen_evo_native_steps(cfg->steps);
    if (st != GEN_OK) {
        fail(e, st);
        return st;
    }
    sig = turing_sig(cfg);
    if (!e->committed.ready || sig != e->turing_sig) {
        /* gen_turing_init rejects a bad cfg before it writes. */
        st = gen_turing_init(&e->committed, cfg);
        if (st != GEN_OK) {
            fail(e, st);
            return st;
        }
        e->turing_sig = sig;
    }
    st = gen_turing_generate(&e->committed, cfg, out, &e->proposed);
    if (st != GEN_OK) {
        fail(e, st);
        return st;
    }
    e->proposed_ok = 1;
    e->pending_ok = 0;
    e->mode = GEN_EVO_TURING;
    return publish(e, out, cfg->steps, cfg->ticks_per_step, bind, GEN_EVO_TURING);
}

gen_status gen_evo_markov_learn(gen_evo *e, uint8_t scale, int root, const gen_buf *src, uint32_t gap_ticks)
{
    gen_status st;
    if (!e || !src)
        return GEN_ERR_PARAM;
    /* Failure returns before the destination model is replaced. */
    st = gen_markov_learn(&e->pending, scale, root, src, gap_ticks, &e->scratch);
    if (st != GEN_OK) {
        e->last_status = (int16_t)st;
        return st;
    }
    e->pending_ok = 1;
    e->last_status = GEN_OK;
    return GEN_OK;
}

gen_status gen_evo_markov_preview(gen_evo *e, const gen_markov_cfg *cfg, uint8_t style,
                                  gen_buf *out, uint32_t bind)
{
    const gen_markov_model *use;
    uint32_t h0;
    gen_status st;
    if (!e || !cfg || !out)
        return GEN_ERR_PARAM;
    st = gen_evo_native_steps(cfg->steps);
    if (st != GEN_OK) {
        fail(e, st);
        return st;
    }
    if (style == GEN_MARKOV_LEARNED) {
        use = e->pending_ok ? &e->pending : (e->model_ready ? &e->model : 0);
        if (!use || use->style != GEN_MARKOV_LEARNED || !use->ready) {
            fail(e, GEN_ERR_EMPTY);
            return GEN_ERR_EMPTY;
        }
    } else if (style > GEN_MARKOV_EXPLORATORY) {
        fail(e, GEN_ERR_PARAM);
        return GEN_ERR_PARAM;
    } else {
        st = gen_markov_preset(&e->scratch, cfg->scale, style);
        if (st != GEN_OK) {
            fail(e, st);
            return st;
        }
        use = &e->scratch;
        e->pending_ok = 0;
    }
    h0 = gen_evo_hash(2166136261u, use, sizeof *use);
    st = gen_markov_generate(use, cfg, out);
    if (st != GEN_OK) {
        fail(e, st);
        return st;
    }
    if (gen_evo_hash(2166136261u, use, sizeof *use) != h0) {
        out->count = 0;
        fail(e, GEN_ERR_PARAM);
        return GEN_ERR_PARAM;
    }
    e->mode = GEN_EVO_MARKOV;
    return publish(e, out, cfg->steps, cfg->ticks_per_step, bind, GEN_EVO_MARKOV);
}

gen_status gen_evo_mutate_preview(gen_evo *e, const gen_buf *src, const gen_mutate_req *req,
                                  gen_buf *out, uint32_t bind)
{
    gen_status st;
    uint32_t before;
    if (!e || !src || !req || !out)
        return GEN_ERR_PARAM;
    st = gen_evo_native_steps(req->dest_steps);
    if (st != GEN_OK) {
        fail(e, st);
        return st;
    }
    if (src->events && out->events == src->events) {
        fail(e, GEN_ERR_PARAM);
        return GEN_ERR_PARAM;
    }
    before = src->events ? gen_evo_hash(2166136261u, src->events, src->count * (uint32_t)sizeof(gen_event)) : 0;
    st = gen_mutate(src, req, out);
    if (st != GEN_OK) {
        fail(e, st);
        return st;
    }
    if (src->events && gen_evo_hash(2166136261u, src->events, src->count * (uint32_t)sizeof(gen_event)) != before) {
        out->count = 0;
        fail(e, GEN_ERR_PARAM);
        return GEN_ERR_PARAM;
    }
    e->mode = GEN_EVO_MUTATE;
    e->proposed_ok = 0;
    e->pending_ok = 0;
    return publish(e, out, req->dest_steps, req->ticks_per_step, bind, GEN_EVO_MUTATE);
}

gen_status gen_evo_apply_result(gen_evo *e, int native_ok)
{
    if (!e)
        return GEN_ERR_PARAM;
    if (e->phase == GEN_EVO_APPLIED) {
        e->last_status = GEN_ERR_PARAM;
        return GEN_ERR_PARAM;
    }
    if (e->phase == GEN_EVO_STALE || (e->phase == GEN_EVO_VALID && e->live_bind != e->preview_bind)) {
        e->phase = GEN_EVO_STALE;
        e->last_status = GEN_ERR_PARAM;
        return GEN_ERR_PARAM;
    }
    if (e->phase != GEN_EVO_VALID) {
        e->last_status = GEN_ERR_PARAM;
        return GEN_ERR_PARAM;
    }
    if (!native_ok) {
        e->last_status = GEN_OK;
        return GEN_OK;
    }
    if (e->mode == GEN_EVO_TURING && e->proposed_ok)
        gen_turing_copy(&e->committed, &e->proposed);
    if (e->mode == GEN_EVO_MARKOV && e->pending_ok) {
        copy_pod(&e->model, &e->pending, sizeof e->model);
        e->model_ready = 1;
    }
    e->proposed_ok = 0;
    e->pending_ok = 0;
    e->phase = GEN_EVO_APPLIED;
    e->last_status = GEN_OK;
    return GEN_OK;
}

gen_status gen_evo_audit(const gen_buf *buf, unsigned steps, uint32_t ticks_per_step)
{
    uint32_t i;
    if (!buf || !buf->events)
        return GEN_ERR_PARAM;
    if (gen_evo_native_steps(steps) != GEN_OK)
        return GEN_ERR_PARAM;
    if (buf->count > steps || buf->count > GEN_EVO_NATIVE_MAX)
        return GEN_ERR_CAPACITY;
    for (i = 0; i < buf->count; i++) {
        const gen_event *ev = &buf->events[i];
        uint32_t step;
        if (ev->note > 127 || ev->velocity < 1 || ev->velocity > 127)
            return GEN_ERR_PARAM;
        if (ticks_per_step == 0 || ev->tick % ticks_per_step)
            return GEN_ERR_TIMING;
        step = ev->tick / ticks_per_step;
        if (step >= steps)
            return GEN_ERR_TIMING;
        if (!gen_native_duration_ok(ev->duration, ticks_per_step))
            return GEN_ERR_TIMING;
        if (ev->tick + ev->duration > steps * ticks_per_step)
            return GEN_ERR_TIMING;
    }
    return gen_buf_validate(buf, 1);
}
