/* Mutation lab.
 *
 * The source buffer is read-only. The destination must be a different
 * event array with count 0. Aliasing is rejected before any write.
 *
 * Order, for each source note, then the geometric passes:
 *   1. Validate. Reject reverse/rotate when rhythm is protected or any
 *      in-range step bit is set.
 *   2. Require out->capacity >= dest_steps. Fail with CAPACITY before writing.
 *   3. Per note: displace, remove, insert, then quantize, octave, pitch,
 *      velocity, gate. Removed notes are not reshaped.
 *   4. Sort, then gen_xform_reverse, then gen_xform_rotate.
 * An inserted pitch is two semitones above the parent, then scale-quantized.
 *
 * A displace that would leave the region, land on an occupied step, or land
 * on a protected step is skipped. An insert with no free unprotected step
 * is skipped. Those skips are the collision policy: notes are not overwritten.
 * Removed notes use the same probability test as gen_xform_remove.
 * Probability 0 with octave, quantize, reverse, and rotate all off copies
 * the source. Reverse and rotate stay unconditional.
 *
 * protect_steps bits 0..63 are the native mask. Bits 64..127 are portable.
 */
#include "genesis.h"

void gen_mutate_protect_step(gen_mutate_req *req, unsigned step, int on)
{
    uint8_t bit;
    if (!req || step >= GEN_MUTATE_MAX_STEPS)
        return;
    bit = (uint8_t)(1u << (step & 7u));
    if (on)
        req->protect_steps[step >> 3] |= bit;
    else
        req->protect_steps[step >> 3] &= (uint8_t)~bit;
}

int gen_mutate_step_protected(const gen_mutate_req *req, unsigned step)
{
    if (!req || step >= GEN_MUTATE_MAX_STEPS)
        return 0;
    return (req->protect_steps[step >> 3] >> (step & 7u)) & 1;
}

static void occ_set(uint8_t *occ, unsigned step, int on)
{
    uint8_t bit = (uint8_t)(1u << (step & 7u));
    if (on)
        occ[step >> 3] |= bit;
    else
        occ[step >> 3] &= (uint8_t)~bit;
}

static int occ_on(const uint8_t *occ, unsigned step)
{
    return (occ[step >> 3] >> (step & 7u)) & 1;
}

static int find_free(const uint8_t *occ, const gen_mutate_req *req, int from)
{
    uint16_t k;
    for (k = 1; k <= req->dest_steps; k++) {
        unsigned s = (unsigned)((from + (int)k) % (int)req->dest_steps);
        if (gen_mutate_step_protected(req, s) || occ_on(occ, s))
            continue;
        return (int)s;
    }
    return -1;
}

static int nonzero_delta(gen_rng *rng, int intensity)
{
    uint32_t k = gen_rng_bound(rng, (uint32_t)intensity * 2u);
    int delta = (int)k - intensity;
    if (delta >= 0)
        delta += 1;
    return delta;
}

static int any_step(const gen_mutate_req *req)
{
    uint16_t s;
    for (s = 0; s < req->dest_steps; s++)
        if (gen_mutate_step_protected(req, s))
            return 1;
    return 0;
}

static gen_status validate(const gen_buf *src, const gen_mutate_req *req, const gen_buf *out)
{
    uint32_t i, region;
    uint8_t step_occ[GEN_MUTATE_MAX_STEPS / 8];
    if (!src || !req || !out || !out->events)
        return GEN_ERR_PARAM;
    if (out->count != 0)
        return GEN_ERR_PARAM;
    if (src->count > 0 && !src->events)
        return GEN_ERR_PARAM;
    if (src->events && out->events == src->events)
        return GEN_ERR_PARAM;
    if (req->dest_steps < 1 || req->dest_steps > GEN_MUTATE_MAX_STEPS)
        return GEN_ERR_PARAM;
    if (req->ticks_per_step == 0 || req->ticks_per_step > 1000000u)
        return GEN_ERR_PARAM;
    if (req->ticks_per_step > UINT32_MAX / req->dest_steps)
        return GEN_ERR_OVERFLOW;
    if (req->pitch_prob > 100 || req->displace_prob > 100 || req->insert_prob > 100 ||
        req->remove_prob > 100 || req->velocity_prob > 100 || req->gate_prob > 100)
        return GEN_ERR_PARAM;
    if (req->quantize > 1 || req->reverse > 1)
        return GEN_ERR_PARAM;
    if (req->octave < -2 || req->octave > 2)
        return GEN_ERR_PARAM;
    /* rotate_steps is int8_t, so every stored value is a legal step count. */
    if (req->intensity > 24)
        return GEN_ERR_PARAM;
    if (req->protect & ~((uint8_t)(GEN_MUT_PROTECT_PITCH | GEN_MUT_PROTECT_RHYTHM |
                                   GEN_MUT_PROTECT_VELOCITY | GEN_MUT_PROTECT_DURATION)))
        return GEN_ERR_PARAM;
    if ((unsigned)req->scale >= GEN_SCALE_COUNT)
        return GEN_ERR_PARAM;
    if (req->root < 0 || req->root > 127)
        return GEN_ERR_PITCH;
    if (req->pitch_min < 0 || req->pitch_max > 127 || req->pitch_min > req->pitch_max)
        return GEN_ERR_PITCH;
    if (req->velocity_prob > 0 &&
        (req->vel_min < 1 || req->vel_max > 127 || req->vel_min > req->vel_max))
        return GEN_ERR_PARAM;
    if (req->gate_prob > 0) {
        if (req->gate_min == 0 || req->gate_max < req->gate_min)
            return GEN_ERR_TIMING;
        if (!gen_native_duration_ok(req->gate_min, req->ticks_per_step) ||
            !gen_native_duration_ok(req->gate_max, req->ticks_per_step))
            return GEN_ERR_TIMING;
    }
    if (src->count > GEN_MUTATE_MAX_STEPS || src->count > req->dest_steps)
        return GEN_ERR_PARAM;
    region = (uint32_t)req->dest_steps * req->ticks_per_step;
    for (i = 0; i < sizeof step_occ; i++)
        step_occ[i] = 0;
    for (i = 0; i < src->count; i++) {
        const gen_event *e = &src->events[i];
        uint32_t step;
        int note;
        uint8_t bit;
        if (e->note > 127 || e->channel > 15)
            return GEN_ERR_PITCH;
        if (e->velocity < 1 || e->velocity > 127 || e->duration == 0)
            return GEN_ERR_TIMING;
        if (e->duration > UINT32_MAX - e->tick)
            return GEN_ERR_OVERFLOW;
        if (e->tick % req->ticks_per_step)
            return GEN_ERR_TIMING;
        if (e->tick + e->duration > region)
            return GEN_ERR_TIMING;
        if (!gen_native_duration_ok(e->duration, req->ticks_per_step))
            return GEN_ERR_TIMING;
        step = e->tick / req->ticks_per_step;
        if (step >= req->dest_steps)
            return GEN_ERR_PARAM;
        bit = (uint8_t)(1u << (step & 7u));
        if (step_occ[step >> 3] & bit)
            return GEN_ERR_MONO;
        step_occ[step >> 3] |= bit;
        note = e->note;
        if (!gen_mutate_step_protected(req, step) && !(req->protect & GEN_MUT_PROTECT_PITCH)) {
            if (req->quantize)
                note = gen_scale_quantize((gen_scale)req->scale, req->root, note,
                                          req->pitch_min, req->pitch_max);
            else if (note < req->pitch_min || note > req->pitch_max)
                return GEN_ERR_PITCH;
            if (req->octave)
                note += (int)req->octave * 12;
            if (note < req->pitch_min || note > req->pitch_max)
                return GEN_ERR_PITCH;
        }
    }
    if ((req->reverse || req->rotate_steps) &&
        ((req->protect & GEN_MUT_PROTECT_RHYTHM) || any_step(req)))
        return GEN_ERR_UNSUPPORTED;
    return GEN_OK;
}

static int rhythm_blocked(const gen_mutate_req *req, uint32_t step)
{
    return (req->protect & GEN_MUT_PROTECT_RHYTHM) || gen_mutate_step_protected(req, step);
}

static void apply_velocity(gen_event *ev, const gen_mutate_req *req, gen_rng *rng);
static void mutate_pitch(gen_event *ev, const gen_mutate_req *req, gen_rng *rng);
static int attr_blocked(const gen_mutate_req *req, unsigned step, unsigned bit);

static void shape_note(gen_event *ev, const gen_mutate_req *req, unsigned origin, gen_rng *rng)
{
    if (req->quantize && !attr_blocked(req, origin, GEN_MUT_PROTECT_PITCH))
        ev->note = (uint8_t)gen_scale_quantize((gen_scale)req->scale, req->root, ev->note,
                                               req->pitch_min, req->pitch_max);
    if (req->octave && !attr_blocked(req, origin, GEN_MUT_PROTECT_PITCH))
        ev->note = (uint8_t)((int)ev->note + (int)req->octave * 12);
    if (req->pitch_prob > 0 && req->intensity > 0 && !attr_blocked(req, origin, GEN_MUT_PROTECT_PITCH))
        mutate_pitch(ev, req, rng);
    if (req->velocity_prob > 0 && req->intensity > 0 &&
        !attr_blocked(req, origin, GEN_MUT_PROTECT_VELOCITY)) {
        if (gen_rng_bound(rng, 100) < req->velocity_prob)
            apply_velocity(ev, req, rng);
    }
    if (req->gate_prob > 0 && !attr_blocked(req, origin, GEN_MUT_PROTECT_DURATION)) {
        if (gen_rng_bound(rng, 100) < req->gate_prob)
            ev->duration = (uint32_t)gen_rng_range(rng, (int32_t)req->gate_min, (int32_t)req->gate_max);
    }
}

static void apply_velocity(gen_event *ev, const gen_mutate_req *req, gen_rng *rng)
{
    gen_event tmp = *ev;
    gen_buf b;
    gen_buf_init(&b, &tmp, 1);
    b.count = 1;
    gen_xform_velocity(&b, -(int)req->intensity, (int)req->intensity, rng);
    if (tmp.velocity < req->vel_min)
        tmp.velocity = req->vel_min;
    if (tmp.velocity > req->vel_max)
        tmp.velocity = req->vel_max;
    ev->velocity = tmp.velocity;
}

static void mutate_pitch(gen_event *ev, const gen_mutate_req *req, gen_rng *rng)
{
    int note;
    if (gen_rng_bound(rng, 100) >= req->pitch_prob)
        return;
    note = (int)ev->note + nonzero_delta(rng, req->intensity);
    ev->note = (uint8_t)gen_scale_quantize((gen_scale)req->scale, req->root, note,
                                           req->pitch_min, req->pitch_max);
}

static int attr_blocked(const gen_mutate_req *req, unsigned step, unsigned bit)
{
    return gen_mutate_step_protected(req, step) || (req->protect & bit) != 0;
}

gen_status gen_mutate(const gen_buf *src, const gen_mutate_req *req, gen_buf *out)
{
    gen_rng rng;
    gen_status st;
    uint8_t occ[GEN_MUTATE_MAX_STEPS / 8];
    uint32_t i, w = 0;
    st = validate(src, req, out);
    if (st != GEN_OK)
        return st;
    if (src->count == 0)
        return GEN_OK;
    /* A monophonic pattern never has more notes than steps. */
    if (out->capacity < req->dest_steps)
        return GEN_ERR_CAPACITY;
    for (i = 0; i < sizeof occ; i++)
        occ[i] = 0;
    for (i = 0; i < src->count; i++)
        occ_set(occ, src->events[i].tick / req->ticks_per_step, 1);
    gen_rng_seed(&rng, req->seed);
    for (i = 0; i < src->count; i++) {
        unsigned origin = src->events[i].tick / req->ticks_per_step;
        unsigned cur = origin;
        int removed = 0;
        int slot = -1;
        gen_event ev;
        if (req->displace_prob > 0 && req->intensity > 0 && !rhythm_blocked(req, origin)) {
            if (gen_rng_bound(&rng, 100) < req->displace_prob) {
                int delta = nonzero_delta(&rng, req->intensity);
                int target = (int)cur + delta;
                occ_set(occ, cur, 0);
                if (target >= 0 && target < (int)req->dest_steps &&
                    !gen_mutate_step_protected(req, (unsigned)target) &&
                    !occ_on(occ, (unsigned)target)) {
                    occ_set(occ, (unsigned)target, 1);
                    cur = (unsigned)target;
                } else {
                    occ_set(occ, cur, 1);
                }
            }
        }
        if (req->remove_prob > 0 && !rhythm_blocked(req, origin)) {
            if (gen_rng_bound(&rng, 100) < req->remove_prob) {
                occ_set(occ, cur, 0);
                removed = 1;
            }
        }
        if (!removed && req->insert_prob > 0 && !rhythm_blocked(req, origin)) {
            if (gen_rng_bound(&rng, 100) < req->insert_prob) {
                slot = find_free(occ, req, (int)cur);
                if (slot >= 0)
                    occ_set(occ, (unsigned)slot, 1);
            }
        }
        if (removed)
            continue;
        ev = src->events[i];
        shape_note(&ev, req, origin, &rng);
        ev.tick = cur * req->ticks_per_step;
        out->events[w++] = ev;
        if (slot >= 0) {
            gen_event extra = ev;
            extra.note = (uint8_t)gen_scale_quantize((gen_scale)req->scale, req->root,
                                                     (int)ev.note + 2, req->pitch_min, req->pitch_max);
            extra.tick = (uint32_t)slot * req->ticks_per_step;
            extra._pad = 0;
            out->events[w++] = extra;
        }
    }
    out->count = w;
    if (gen_buf_sort(out) != GEN_OK ||
        (req->reverse && gen_xform_reverse(out) != GEN_OK) ||
        (req->rotate_steps && gen_xform_rotate(out, req->rotate_steps, req->ticks_per_step) != GEN_OK)) {
        out->count = 0;
        return GEN_ERR_PARAM;
    }
    st = gen_buf_validate(out, 1);
    if (st != GEN_OK)
        out->count = 0;
    return st;
}
