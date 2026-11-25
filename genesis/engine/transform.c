#include "genesis.h"

gen_status gen_xform_transpose(gen_buf *b, int semitones)
{
    uint32_t i;
    if (!b)
        return GEN_ERR_PARAM;
    for (i = 0; i < b->count; i++) {
        int n = (int)b->events[i].note + semitones;
        if (n < 0 || n > 127)
            return GEN_ERR_PITCH;
        b->events[i].note = (uint8_t)n;
    }
    return GEN_OK;
}

gen_status gen_xform_reverse(gen_buf *b)
{
    uint32_t i, max_end = 0;
    if (!b)
        return GEN_ERR_PARAM;
    if (b->count == 0)
        return GEN_OK;
    for (i = 0; i < b->count; i++) {
        uint32_t end = b->events[i].tick + b->events[i].duration;
        if (end > max_end)
            max_end = end;
    }
    for (i = 0; i < b->count; i++) {
        gen_event *e = &b->events[i];
        uint32_t end = e->tick + e->duration;
        e->tick = max_end - end;
    }
    return gen_buf_sort(b);
}

gen_status gen_xform_rotate(gen_buf *b, int steps, uint32_t ticks_per_step)
{
    uint32_t i, span = 0, shift;
    if (!b || ticks_per_step == 0)
        return GEN_ERR_PARAM;
    if (b->count == 0)
        return GEN_OK;
    for (i = 0; i < b->count; i++) {
        uint32_t end = b->events[i].tick + b->events[i].duration;
        if (end > span)
            span = end;
    }
    /* Snap span up to step grid */
    if (span % ticks_per_step)
        span += ticks_per_step - (span % ticks_per_step);
    if (span == 0)
        return GEN_OK;
    if (steps < 0) {
        uint32_t nsteps = span / ticks_per_step;
        uint32_t a = (uint32_t)((-steps) % (int)nsteps);
        shift = (nsteps - a) % nsteps * ticks_per_step;
    } else {
        shift = (uint32_t)steps * ticks_per_step;
    }
    for (i = 0; i < b->count; i++)
        b->events[i].tick = (b->events[i].tick + shift) % span;
    return gen_buf_sort(b);
}

gen_status gen_xform_quantize(gen_buf *b, gen_scale scale, int root, int lo, int hi)
{
    uint32_t i;
    if (!b)
        return GEN_ERR_PARAM;
    for (i = 0; i < b->count; i++)
        b->events[i].note = (uint8_t)gen_scale_quantize(scale, root, b->events[i].note, lo, hi);
    return GEN_OK;
}

gen_status gen_xform_velocity(gen_buf *b, int delta_min, int delta_max, gen_rng *rng)
{
    uint32_t i;
    if (!b || !rng)
        return GEN_ERR_PARAM;
    for (i = 0; i < b->count; i++) {
        int v = (int)b->events[i].velocity + gen_rng_range(rng, delta_min, delta_max);
        if (v < 1)
            v = 1;
        if (v > 127)
            v = 127;
        b->events[i].velocity = (uint8_t)v;
    }
    return GEN_OK;
}

gen_status gen_xform_mutate_pitch(gen_buf *b, gen_scale scale, int root, int lo, int hi,
                                  uint8_t prob, gen_rng *rng)
{
    uint32_t i;
    int pitches[128], np = 0, p;
    if (!b || !rng || prob > 100)
        return GEN_ERR_PARAM;
    for (p = lo; p <= hi; p++)
        if (gen_scale_contains(scale, root, p))
            pitches[np++] = p;
    if (np == 0)
        return GEN_ERR_PITCH;
    for (i = 0; i < b->count; i++) {
        if (gen_rng_bound(rng, 100) >= prob)
            continue;
        b->events[i].note = (uint8_t)pitches[gen_rng_bound(rng, (uint32_t)np)];
    }
    return GEN_OK;
}

gen_status gen_xform_gate(gen_buf *b, int pct_min, int pct_max, gen_rng *rng)
{
    uint32_t i;
    if (!b || !rng || pct_min < 1 || pct_max > 200 || pct_min > pct_max)
        return GEN_ERR_PARAM;
    for (i = 0; i < b->count; i++) {
        int pct = (int)gen_rng_range(rng, pct_min, pct_max);
        uint64_t d = (uint64_t)b->events[i].duration * (uint64_t)pct / 100u;
        if (d == 0)
            d = 1;
        if (d > UINT32_MAX)
            return GEN_ERR_OVERFLOW;
        b->events[i].duration = (uint32_t)d;
    }
    return GEN_OK;
}

gen_status gen_xform_remove(gen_buf *b, uint8_t prob, gen_rng *rng)
{
    uint32_t i, w = 0;
    if (!b || !rng || prob > 100)
        return GEN_ERR_PARAM;
    for (i = 0; i < b->count; i++) {
        if (gen_rng_bound(rng, 100) < prob)
            continue;
        b->events[w++] = b->events[i];
    }
    b->count = w;
    return GEN_OK;
}
