#include "genesis.h"

gen_status gen_rhythm_fixed(gen_rhythm *out, const uint8_t *pattern, uint16_t n, uint16_t tps)
{
    uint16_t i;
    if (!out || !pattern || n == 0 || n > GEN_RHYTHM_MAX_STEPS || tps == 0)
        return GEN_ERR_PARAM;
    out->step_count = n;
    out->ticks_per_step = tps;
    for (i = 0; i < n; i++)
        out->steps[i] = pattern[i] ? 1 : 0;
    return GEN_OK;
}

gen_status gen_rhythm_probability(gen_rhythm *out, uint16_t n, uint16_t tps, uint8_t density, gen_rng *rng)
{
    uint16_t i;
    if (!out || !rng || n == 0 || n > GEN_RHYTHM_MAX_STEPS || tps == 0 || density > 100)
        return GEN_ERR_PARAM;
    out->step_count = n;
    out->ticks_per_step = tps;
    for (i = 0; i < n; i++)
        out->steps[i] = (gen_rng_bound(rng, 100) < density) ? 1 : 0;
    return GEN_OK;
}

/* Bjorklund Euclidean: distribute `pulses` into `steps` as evenly as possible.
 * Rotation convention: after building with first pulse at index 0 (when pulses>0),
 * rotate right by `rotate` positions (step i <- step (i - rotate) mod n). */
gen_status gen_rhythm_euclidean(gen_rhythm *out, uint16_t steps, uint16_t pulses, uint16_t rotate, uint16_t tps)
{
    uint16_t i;
    int bucket;
    if (!out || steps == 0 || steps > GEN_RHYTHM_MAX_STEPS || tps == 0 || pulses > steps)
        return GEN_ERR_PARAM;
    out->step_count = steps;
    out->ticks_per_step = tps;
    for (i = 0; i < steps; i++)
        out->steps[i] = 0;
    if (pulses == 0)
        return GEN_OK;
    if (pulses == steps) {
        for (i = 0; i < steps; i++)
            out->steps[i] = 1;
    } else {
        /* Bresenham-style */
        bucket = 0;
        for (i = 0; i < steps; i++) {
            bucket += pulses;
            if (bucket >= (int)steps) {
                bucket -= (int)steps;
                out->steps[i] = 1;
            }
        }
    }
    if (rotate % steps != 0) {
        uint8_t tmp[GEN_RHYTHM_MAX_STEPS];
        uint16_t r = rotate % steps;
        for (i = 0; i < steps; i++)
            tmp[i] = out->steps[i];
        for (i = 0; i < steps; i++)
            out->steps[i] = tmp[(i + steps - r) % steps];
    }
    return GEN_OK;
}

gen_status gen_rhythm_rotate(gen_rhythm *r, int amount)
{
    uint8_t tmp[GEN_RHYTHM_MAX_STEPS];
    uint16_t i, n, a;
    if (!r || r->step_count == 0)
        return GEN_ERR_PARAM;
    n = r->step_count;
    if (amount < 0) {
        a = (uint16_t)(((-amount) % n));
        a = (uint16_t)((n - a) % n);
    } else {
        a = (uint16_t)(amount % n);
    }
    for (i = 0; i < n; i++)
        tmp[i] = r->steps[i];
    for (i = 0; i < n; i++)
        r->steps[i] = tmp[(i + n - a) % n];
    return GEN_OK;
}

uint16_t gen_rhythm_pulse_count(const gen_rhythm *r)
{
    uint16_t i, c = 0;
    if (!r)
        return 0;
    for (i = 0; i < r->step_count; i++)
        if (r->steps[i])
            c++;
    return c;
}

gen_status gen_rhythm_to_events(const gen_rhythm *r, gen_buf *out,
                                uint8_t note, uint8_t vel, uint32_t dur, uint8_t ch)
{
    uint16_t i;
    uint32_t saved;
    gen_status st;
    if (!r || !out)
        return GEN_ERR_PARAM;
    if (note > 127 || ch > 15 || vel < 1 || vel > 127)
        return GEN_ERR_PARAM;
    if (dur == 0)
        dur = r->ticks_per_step;
    saved = out->count;
    for (i = 0; i < r->step_count; i++) {
        gen_event ev;
        if (!r->steps[i])
            continue;
        ev.tick = (uint32_t)i * r->ticks_per_step;
        ev.note = note;
        ev.velocity = vel;
        ev.channel = ch;
        ev._pad = 0;
        ev.duration = dur;
        st = gen_buf_push(out, &ev);
        if (st != GEN_OK) {
            out->count = saved;
            return st;
        }
    }
    return GEN_OK;
}
