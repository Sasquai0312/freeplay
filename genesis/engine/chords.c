#include "genesis.h"

/* Semitone offsets from chord root. */
static gen_status fill_intervals(gen_chord_quality q, int *iv, int *n)
{
    switch (q) {
    case GEN_CHORD_TRIAD:
    case GEN_CHORD_MAJ:
        iv[0] = 0; iv[1] = 4; iv[2] = 7; *n = 3; return GEN_OK;
    case GEN_CHORD_MIN:
        iv[0] = 0; iv[1] = 3; iv[2] = 7; *n = 3; return GEN_OK;
    case GEN_CHORD_SUS2:
        iv[0] = 0; iv[1] = 2; iv[2] = 7; *n = 3; return GEN_OK;
    case GEN_CHORD_SUS4:
        iv[0] = 0; iv[1] = 5; iv[2] = 7; *n = 3; return GEN_OK;
    case GEN_CHORD_SEVENTH:
    case GEN_CHORD_MAJ7:
        iv[0] = 0; iv[1] = 4; iv[2] = 7; iv[3] = 11; *n = 4; return GEN_OK;
    case GEN_CHORD_MIN7:
        iv[0] = 0; iv[1] = 3; iv[2] = 7; iv[3] = 10; *n = 4; return GEN_OK;
    case GEN_CHORD_DOM7:
        iv[0] = 0; iv[1] = 4; iv[2] = 7; iv[3] = 10; *n = 4; return GEN_OK;
    case GEN_CHORD_NINTH:
        iv[0] = 0; iv[1] = 4; iv[2] = 7; iv[3] = 10; iv[4] = 14; *n = 5; return GEN_OK;
    default:
        return GEN_ERR_UNSUPPORTED;
    }
}

gen_status gen_chord_build(gen_chord *out, int root, gen_chord_quality q, int inversion, int octave_shift)
{
    int iv[GEN_CHORD_MAX_NOTES], n, i, j, midi;
    gen_status st;
    if (!out || root < 0 || root > 127)
        return GEN_ERR_PARAM;
    st = fill_intervals(q, iv, &n);
    if (st != GEN_OK)
        return st;
    if (inversion < 0 || inversion >= n)
        return GEN_ERR_PARAM;
    out->count = (uint8_t)n;
    for (i = 0; i < n; i++) {
        j = (i + inversion) % n;
        midi = root + iv[j] + octave_shift * 12;
        /* notes rotated by inversion: raise wrapped tones one octave */
        if (j < inversion)
            midi += 12;
        if (midi < 0 || midi > 127)
            return GEN_ERR_PITCH;
        out->notes[i] = (uint8_t)midi;
    }
    return GEN_OK;
}

gen_status gen_chord_diatonic(gen_chord *out, int key_root, gen_scale scale, int degree,
                              gen_chord_quality q, int inversion)
{
    int chord_root;
    if (degree < 0 || degree > 6)
        return GEN_ERR_PARAM;
    if ((unsigned)scale >= GEN_SCALE_COUNT)
        return GEN_ERR_PARAM;
    /* Build from scale degree 0..6 using major/minor triad quality from scale when q==TRIAD */
    chord_root = gen_scale_degree_to_midi(scale, key_root, degree, 0);
    if (chord_root < 0)
        return GEN_ERR_PITCH;
    if (q == GEN_CHORD_TRIAD || q == GEN_CHORD_SEVENTH || q == GEN_CHORD_NINTH) {
        /* Derive quality from scale thirds */
        int third = gen_scale_degree_to_midi(scale, key_root, degree + 2, 0);
        int fifth = gen_scale_degree_to_midi(scale, key_root, degree + 4, 0);
        int seventh = gen_scale_degree_to_midi(scale, key_root, degree + 6, 0);
        int i = 0;
        out->notes[i++] = (uint8_t)chord_root;
        out->notes[i++] = (uint8_t)third;
        out->notes[i++] = (uint8_t)fifth;
        if (q == GEN_CHORD_SEVENTH || q == GEN_CHORD_NINTH)
            out->notes[i++] = (uint8_t)seventh;
        if (q == GEN_CHORD_NINTH)
            out->notes[i++] = (uint8_t)gen_scale_degree_to_midi(scale, key_root, degree + 8, 0);
        out->count = (uint8_t)i;
        if (inversion > 0 && inversion < out->count) {
            gen_chord tmp = *out;
            int k;
            for (k = 0; k < out->count; k++) {
                int src = (k + inversion) % out->count;
                int n = tmp.notes[src];
                if (src < inversion)
                    n = gen_clamp_midi(n + 12);
                out->notes[k] = (uint8_t)n;
            }
        }
        return GEN_OK;
    }
    return gen_chord_build(out, chord_root, q, inversion, 0);
}

gen_status gen_chord_voice_lead(const gen_chord *prev, gen_chord *next)
{
    int shift, best_shift = 0, best_cost = 0x7fffffff, s;
    gen_chord cand;
    if (!prev || !next || next->count == 0)
        return GEN_ERR_PARAM;
    for (shift = -2; shift <= 2; shift++) {
        int cost = 0, i, j, n;
        cand = *next;
        for (i = 0; i < cand.count; i++) {
            n = (int)cand.notes[i] + shift * 12;
            if (n < 0 || n > 127)
                goto skip;
            cand.notes[i] = (uint8_t)n;
        }
        for (i = 0; i < cand.count; i++) {
            int best = 999;
            for (j = 0; j < prev->count; j++) {
                int d = (int)cand.notes[i] - (int)prev->notes[j];
                if (d < 0)
                    d = -d;
                if (d < best)
                    best = d;
            }
            cost += best;
        }
        if (cost < best_cost) {
            best_cost = cost;
            best_shift = shift;
        }
    skip:
        (void)s;
    }
    for (s = 0; s < next->count; s++) {
        int n = (int)next->notes[s] + best_shift * 12;
        if (n < 0 || n > 127)
            return GEN_ERR_PITCH;
        next->notes[s] = (uint8_t)n;
    }
    return GEN_OK;
}

void gen_prog_I_V_vi_IV(gen_progression *p)
{
    static const uint8_t d[] = {0, 4, 5, 3};
    uint8_t i;
    p->count = 4;
    for (i = 0; i < 4; i++)
        p->degrees[i] = d[i];
}
void gen_prog_ii_V_I(gen_progression *p)
{
    static const uint8_t d[] = {1, 4, 0};
    uint8_t i;
    p->count = 3;
    for (i = 0; i < 3; i++)
        p->degrees[i] = d[i];
}
void gen_prog_i_VI_III_VII(gen_progression *p)
{
    static const uint8_t d[] = {0, 5, 2, 6};
    uint8_t i;
    p->count = 4;
    for (i = 0; i < 4; i++)
        p->degrees[i] = d[i];
}
void gen_prog_I_vi_ii_V(gen_progression *p)
{
    static const uint8_t d[] = {0, 5, 1, 4};
    uint8_t i;
    p->count = 4;
    for (i = 0; i < 4; i++)
        p->degrees[i] = d[i];
}
void gen_prog_i_iv_V_i(gen_progression *p)
{
    static const uint8_t d[] = {0, 3, 4, 0};
    uint8_t i;
    p->count = 4;
    for (i = 0; i < 4; i++)
        p->degrees[i] = d[i];
}

static gen_status emit_chord_at(gen_buf *out, const gen_chord *c, uint32_t tick,
                                uint32_t dur, uint8_t vel, uint8_t ch)
{
    uint8_t i;
    for (i = 0; i < c->count; i++) {
        gen_event ev;
        gen_status st;
        ev.tick = tick;
        ev.note = c->notes[i];
        ev.velocity = vel;
        ev.channel = ch;
        ev._pad = 0;
        ev.duration = dur;
        st = gen_buf_push(out, &ev);
        if (st != GEN_OK)
            return st;
    }
    return GEN_OK;
}

gen_status gen_harmony_progression(const gen_harmony_cfg *cfg, const gen_progression *prog, gen_buf *out)
{
    uint8_t i;
    gen_chord prev, cur;
    uint32_t saved;
    gen_status st;
    if (!cfg || !prog || !out || prog->count == 0)
        return GEN_ERR_PARAM;
    if (cfg->ticks_per_chord == 0 || cfg->duration == 0 || cfg->velocity < 1)
        return GEN_ERR_PARAM;
    saved = out->count;
    prev.count = 0;
    for (i = 0; i < prog->count; i++) {
        st = gen_chord_diatonic(&cur, cfg->root, cfg->scale, prog->degrees[i], cfg->quality, 0);
        if (st != GEN_OK) {
            out->count = saved;
            return st;
        }
        if (cfg->voice_lead && prev.count)
            gen_chord_voice_lead(&prev, &cur);
        st = emit_chord_at(out, &cur, (uint32_t)i * cfg->ticks_per_chord, cfg->duration,
                           cfg->velocity, cfg->channel);
        if (st != GEN_OK) {
            out->count = saved;
            return st;
        }
        prev = cur;
    }
    return GEN_OK;
}

gen_status gen_harmony_seeded(const gen_harmony_cfg *cfg, uint8_t bars, gen_buf *out)
{
    gen_progression prog;
    gen_rng rng;
    uint8_t i;
    if (!cfg || bars == 0 || bars > 16)
        return GEN_ERR_PARAM;
    gen_rng_seed(&rng, cfg->seed);
    prog.count = bars;
    for (i = 0; i < bars; i++)
        prog.degrees[i] = (uint8_t)gen_rng_bound(&rng, 7);
    return gen_harmony_progression(cfg, &prog, out);
}
