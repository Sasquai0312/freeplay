/* Evolution engine tests: Turing register, Markov melody, mutation lab. */
#include "genesis.h"
#include <stdio.h>
#include <string.h>

static int g_fail;

#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); g_fail++; } \
} while (0)

static gen_turing_cfg tcfg(uint32_t seed, uint8_t length, uint16_t steps,
                           uint8_t evolve, uint8_t rest, uint8_t locked)
{
    gen_turing_cfg c;
    memset(&c, 0, sizeof c);
    c.gate = 480;
    c.ticks_per_step = 480;
    c.seed = seed;
    c.root = 60;
    c.pitch_min = 48;
    c.pitch_max = 72;
    c.steps = steps;
    c.scale = GEN_SCALE_NAT_MINOR;
    c.length = length;
    c.evolve_prob = evolve;
    c.rest_prob = rest;
    c.velocity = 96;
    c.locked = locked;
    c.channel = 0;
    return c;
}

static gen_markov_cfg mcfg(uint32_t seed, uint16_t steps, uint8_t rest)
{
    gen_markov_cfg c;
    memset(&c, 0, sizeof c);
    c.gate = 480;
    c.ticks_per_step = 480;
    c.seed = seed;
    c.root = 60;
    c.pitch_min = 48;
    c.pitch_max = 84;
    c.initial_degree = 0;
    c.steps = steps;
    c.scale = GEN_SCALE_NAT_MINOR;
    c.velocity = 90;
    c.rest_prob = rest;
    c.octave_motion = 1;
    c.channel = 0;
    return c;
}

static gen_mutate_req mreq(uint32_t seed)
{
    gen_mutate_req r;
    memset(&r, 0, sizeof r);
    r.ticks_per_step = 480;
    r.gate_min = 240;
    r.gate_max = 480;
    r.seed = seed;
    r.root = 60;
    r.pitch_min = 36;
    r.pitch_max = 96;
    r.dest_steps = 16;
    r.scale = GEN_SCALE_MAJOR;
    r.intensity = 2;
    r.vel_min = 1;
    r.vel_max = 127;
    return r;
}

static void put_note(gen_event *e, uint32_t step, uint8_t note)
{
    memset(e, 0, sizeof *e);
    e->tick = step * 480u;
    e->note = note;
    e->velocity = 100;
    e->duration = 480;
}

static int events_same(const gen_buf *a, const gen_buf *b)
{
    uint32_t i;
    if (a->count != b->count)
        return 0;
    for (i = 0; i < a->count; i++) {
        const gen_event *x = &a->events[i], *y = &b->events[i];
        if (x->tick != y->tick || x->note != y->note || x->velocity != y->velocity ||
            x->duration != y->duration || x->channel != y->channel)
            return 0;
    }
    return 1;
}

static int mono_scale(const gen_buf *b, uint8_t scale, int root, int lo, int hi, uint32_t tps, uint16_t steps)
{
    uint32_t i;
    if (gen_buf_validate(b, 1) != GEN_OK)
        return 0;
    for (i = 0; i < b->count; i++) {
        const gen_event *e = &b->events[i];
        if (e->note < lo || e->note > hi)
            return 0;
        if (!gen_scale_contains((gen_scale)scale, root, e->note))
            return 0;
        if (tps == 0 || e->tick % tps)
            return 0;
        if (e->tick / tps >= steps)
            return 0;
        if (!gen_native_duration_ok(e->duration, tps))
            return 0;
        if (i > 0 && e->tick <= b->events[i - 1].tick)
            return 0;
    }
    return 1;
}

static int degree_of(uint8_t scale, int root, int note)
{
    int n = gen_scale_degree_count((gen_scale)scale);
    int d;
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

static void test_turing(void)
{
    gen_turing a, b, next, poison, saved;
    gen_turing_cfg c, bad;
    gen_event storage[128], storage2[128];
    gen_buf out, out2;
    uint32_t i;
    uint8_t seen[128];
    int unique = 0;

    CHECK(sizeof(gen_event) == 12, "gen_event stays 12 bytes");
    c = tcfg(1, 8, 16, 0, 0, 0);
    CHECK(gen_turing_init(&a, &c) == GEN_OK, "init");
    CHECK(gen_turing_init(&b, &c) == GEN_OK, "init b");
    CHECK(memcmp(&a, &b, sizeof a) == 0, "same seed same state");
    c.seed = 2;
    CHECK(gen_turing_init(&b, &c) == GEN_OK, "seed 2");
    CHECK(memcmp(a.cells, b.cells, a.length) != 0, "different seeds differ");

    memset(seen, 0, sizeof seen);
    for (i = 1; i <= 40; i++) {
        c = tcfg(i, 8, 16, 0, 0, 0);
        CHECK(gen_turing_init(&b, &c) == GEN_OK, "seed sweep");
        if (b.cells[0] < 128 && !seen[b.cells[0]]) {
            seen[b.cells[0]] = 1;
            unique++;
        }
    }
    CHECK(unique >= 4, "seeds vary the register");

    c = tcfg(7, 1, 16, 0, 0, 1);
    CHECK(gen_turing_init(&a, &c) == GEN_OK, "len 1");
    gen_buf_init(&out, storage, 128);
    CHECK(gen_turing_generate(&a, NULL, &out, &next) == GEN_OK, "gen len 1");
    CHECK(out.count == 16, "len 1 fills");
    for (i = 1; i < out.count; i++)
        CHECK(out.events[i].note == out.events[0].note, "len 1 repeats");

    {
        uint8_t lengths[] = {4, 8, 16, 32, 64};
        uint16_t outs[] = {16, 32, 64, 128};
        int li, oi;
        for (li = 0; li < 5; li++) {
            for (oi = 0; oi < 4; oi++) {
                c = tcfg(11, lengths[li], outs[oi], 0, 0, 1);
                CHECK(gen_turing_init(&a, &c) == GEN_OK, "len init");
                gen_buf_clear(&out);
                CHECK(gen_turing_generate(&a, NULL, &out, &next) == GEN_OK, "len gen");
                CHECK(out.count == outs[oi], "full grid");
                CHECK(mono_scale(&out, c.scale, c.root, c.pitch_min, c.pitch_max, 480, outs[oi]), "native phrase");
                for (i = 0; i < out.count; i++) {
                    uint32_t j = i % lengths[li];
                    CHECK(out.events[i].note == a.cells[j], "locked period");
                }
            }
        }
    }

    c = tcfg(3, 8, 24, 100, 0, 1);
    CHECK(gen_turing_init(&a, &c) == GEN_OK, "lock init");
    gen_buf_clear(&out);
    CHECK(gen_turing_generate(&a, NULL, &out, &next) == GEN_OK, "lock gen");
    CHECK(memcmp(a.cells, next.cells, 64) == 0, "lock keeps cells");
    CHECK(next.pos == (a.pos + 24) % a.length, "lock still advances");
    CHECK(a.pos == 0, "src pos unchanged");

    c = tcfg(3, 8, 24, 0, 0, 0);
    CHECK(gen_turing_init(&a, &c) == GEN_OK, "zero evolve");
    gen_buf_clear(&out);
    CHECK(gen_turing_generate(&a, NULL, &out, &next) == GEN_OK, "zero gen");
    CHECK(memcmp(a.cells, next.cells, 64) == 0, "zero evolve cells");
    CHECK(a.rng.state == next.rng.state, "zero evolve rng");

    c = tcfg(9, 8, 8, 100, 0, 0);
    CHECK(gen_turing_init(&a, &c) == GEN_OK, "full evolve");
    saved = a;
    gen_buf_clear(&out);
    CHECK(gen_turing_generate(&a, NULL, &out, &next) == GEN_OK, "full gen");
    CHECK(memcmp(&a, &saved, sizeof a) == 0, "generate leaves src");
    CHECK(next.rng.state != a.rng.state, "100% draws");
    gen_buf_init(&out2, storage2, 128);
    CHECK(gen_turing_generate(&a, NULL, &out2, &b) == GEN_OK, "replay");
    CHECK(events_same(&out, &out2), "same state same events");
    CHECK(memcmp(&next, &b, sizeof next) == 0, "same proposed state");

    memset(&poison, 0x5A, sizeof poison);
    gen_buf_init(&out, storage, 4);
    storage[0].note = 99;
    CHECK(gen_turing_generate(&a, NULL, &out, &poison) == GEN_ERR_CAPACITY, "capacity");
    CHECK(out.count == 0, "capacity count");
    CHECK(storage[0].note == 99, "capacity does not write");
    CHECK(poison.ready == 0x5A, "failed generate leaves next");
    CHECK(memcmp(&a, &saved, sizeof a) == 0, "failed generate leaves src");

    c = tcfg(4, 8, 16, 0, 0, 1);
    CHECK(gen_turing_init(&a, &c) == GEN_OK, "rest 0 init");
    gen_buf_init(&out, storage, 16);
    CHECK(gen_turing_generate(&a, NULL, &out, &next) == GEN_OK, "rest 0");
    CHECK(out.count == 16, "rest 0 count");
    c = tcfg(4, 8, 16, 0, 100, 1);
    CHECK(gen_turing_init(&a, &c) == GEN_OK, "rest 100 init");
    gen_buf_init(&out, storage, 0);
    CHECK(gen_turing_generate(&a, NULL, &out, &next) == GEN_OK, "rest 100");
    CHECK(out.count == 0, "rest 100 emits nothing");

    c = tcfg(15, 8, 16, 40, 10, 0);
    CHECK(gen_turing_init(&a, &c) == GEN_OK, "reset init");
    saved = a;
    gen_buf_init(&out, storage, 128);
    CHECK(gen_turing_generate(&a, NULL, &out, &next) == GEN_OK, "before reset");
    b = next;
    CHECK(gen_turing_reset(&b) == GEN_OK, "reset");
    CHECK(memcmp(&b, &saved, sizeof b) == 0, "reset restores init");

    CHECK(gen_turing_copy(&b, &a) == GEN_OK, "copy");
    b.cells[0] ^= 0x7f;
    CHECK(a.cells[0] != b.cells[0], "copy is independent");

    bad = c;
    bad.length = 0;
    CHECK(gen_turing_init(&b, &bad) == GEN_ERR_PARAM, "len 0");
    bad.length = 65;
    CHECK(gen_turing_init(&b, &bad) == GEN_ERR_PARAM, "len 65");
    bad = c;
    bad.steps = 0;
    CHECK(gen_turing_init(&b, &bad) == GEN_ERR_PARAM, "steps 0");
    bad.steps = 129;
    CHECK(gen_turing_init(&b, &bad) == GEN_ERR_PARAM, "steps 129");
    a.ready = 0;
    CHECK(gen_turing_generate(&a, NULL, &out, &next) == GEN_ERR_PARAM, "not ready");
    CHECK(gen_turing_init(&a, &c) == GEN_OK, "reinit");
    a.cells[0] = 200;
    CHECK(gen_turing_generate(&a, NULL, &out, &next) == GEN_ERR_PITCH, "bad cell");

    c = tcfg(21, 16, 32, 25, 0, 0);
    CHECK(gen_turing_init(&a, &c) == GEN_OK, "long init");
    for (i = 0; i < 100; i++) {
        uint8_t k;
        gen_buf_clear(&out);
        CHECK(gen_turing_generate(&a, NULL, &out, &next) == GEN_OK, "cycle");
        CHECK(mono_scale(&out, c.scale, c.root, c.pitch_min, c.pitch_max, 480, 32), "cycle native");
        for (k = 0; k < next.length; k++)
            CHECK(next.cells[k] == GEN_TURING_REST ||
                  (next.cells[k] <= 127 &&
                   gen_scale_contains((gen_scale)c.scale, c.root, next.cells[k])),
                  "cell stays in scale");
        a = next;
    }
}

static void test_markov(void)
{
    gen_markov_model m, scratch, poisoned;
    gen_markov_cfg c;
    gen_event storage[128], src_ev[8], copy_ev[8];
    gen_buf out, src;
    uint32_t i;
    int from0 = 0, to1 = 0;

    CHECK(gen_markov_style_weight(GEN_MARKOV_STEPWISE, 7, 0, 1) == 40, "step +1");
    CHECK(gen_markov_style_weight(GEN_MARKOV_STEPWISE, 7, 0, 0) == 12, "step 0");
    CHECK(gen_markov_style_weight(GEN_MARKOV_STEPWISE, 7, 0, 6) == 40, "step wrap");
    CHECK(gen_markov_style_weight(GEN_MARKOV_STEPWISE, 7, 0, 3) == 1, "step far");
    CHECK(gen_markov_style_weight(GEN_MARKOV_MOTIF, 7, 0, 2) == 22, "motif third");
    CHECK(gen_markov_style_weight(GEN_MARKOV_BASSLINE, 7, 1, 0) > 30, "bass root");
    CHECK(gen_markov_style_weight(GEN_MARKOV_EXPLORATORY, 7, 0, 0) == 3, "explore stay");
    CHECK(gen_markov_style_weight(GEN_MARKOV_LEARNED, 7, 0, 1) == 0, "learned helper");
    CHECK(gen_markov_preset(&m, GEN_SCALE_MAJOR, GEN_MARKOV_STEPWISE) == GEN_OK, "preset");
    CHECK(m.w[0][1] == 40 && m.w[0][0] == 12, "preset stores weights");

    memset(&m, 0, sizeof m);
    m.n_deg = 7;
    m.ready = 1;
    m.style = GEN_MARKOV_LEARNED;
    m.w[0][1] = 9;
    m.w[0][0] = 1;
    for (i = 1; i < 7; i++)
        m.w[i][0] = 1;
    c = mcfg(12345, 128, 0);
    c.scale = GEN_SCALE_MAJOR;
    c.initial_degree = 0;
    gen_buf_init(&out, storage, 128);
    CHECK(gen_markov_generate(&m, &c, &out) == GEN_OK, "weighted gen");
    CHECK(out.count == 128, "weighted count");
    for (i = 0; i + 1 < out.count; i++) {
        int d0 = degree_of(GEN_SCALE_MAJOR, 60, out.events[i].note);
        int d1 = degree_of(GEN_SCALE_MAJOR, 60, out.events[i + 1].note);
        if (d0 == 0) {
            from0++;
            if (d1 == 1)
                to1++;
            CHECK(d1 == 0 || d1 == 1, "zero weight not selected");
        }
    }
    CHECK(from0 > 40, "enough trials");
    CHECK(to1 * 100 > from0 * 75, "higher weight wins");
    CHECK(to1 < from0, "lower weight still occurs");

    memset(&m, 0, sizeof m);
    m.n_deg = 7;
    m.ready = 1;
    m.style = GEN_MARKOV_LEARNED;
    m.w[0][3] = 5;
    m.w[3][3] = 5;
    c.steps = 24;
    gen_buf_clear(&out);
    CHECK(gen_markov_generate(&m, &c, &out) == GEN_OK, "single edge");
    CHECK(degree_of(GEN_SCALE_MAJOR, 60, out.events[0].note) == 0, "starts at 0");
    for (i = 1; i < out.count; i++)
        CHECK(degree_of(GEN_SCALE_MAJOR, 60, out.events[i].note) == 3, "only allowed edge");

    memset(&m, 0, sizeof m);
    m.n_deg = 7;
    m.ready = 1;
    m.style = GEN_MARKOV_LEARNED;
    c.steps = 32;
    gen_buf_clear(&out);
    CHECK(gen_markov_generate(&m, &c, &out) == GEN_OK, "all zero fallback");
    {
        uint8_t seen[12];
        int nseen = 0;
        memset(seen, 0, sizeof seen);
        for (i = 0; i < out.count; i++) {
            int d = degree_of(GEN_SCALE_MAJOR, 60, out.events[i].note);
            CHECK(d >= 0, "fallback in scale");
            if (!seen[d]) {
                seen[d] = 1;
                nseen++;
            }
        }
        CHECK(nseen >= 2, "uniform fallback moves");
    }

    c = mcfg(5, 16, 0);
    CHECK(gen_markov_preset(&m, GEN_SCALE_NAT_MINOR, GEN_MARKOV_STEPWISE) == GEN_OK, "minor preset");
    gen_buf_init(&out, storage, 15);
    storage[0].note = 77;
    CHECK(gen_markov_generate(&m, &c, &out) == GEN_ERR_CAPACITY, "markov capacity");
    CHECK(out.count == 0 && storage[0].note == 77, "markov capacity is clean");
    {
        uint16_t lens[] = {16, 32, 64, 128};
        int li;
        for (li = 0; li < 4; li++) {
            c.steps = lens[li];
            gen_buf_init(&out, storage, lens[li]);
            CHECK(gen_markov_generate(&m, &c, &out) == GEN_OK, "markov length");
            CHECK(out.count == lens[li], "markov length count");
            CHECK(mono_scale(&out, c.scale, c.root, c.pitch_min, c.pitch_max, 480, lens[li]), "markov native");
        }
    }

    c = mcfg(8, 32, 0);
    c.octave_motion = 0;
    c.pitch_min = 36;
    c.pitch_max = 84;
    c.scale = GEN_SCALE_MAJOR;
    CHECK(gen_markov_preset(&m, GEN_SCALE_MAJOR, GEN_MARKOV_EXPLORATORY) == GEN_OK, "oct preset");
    gen_buf_init(&out, storage, 32);
    CHECK(gen_markov_generate(&m, &c, &out) == GEN_OK, "octave gen");
    for (i = 0; i < out.count; i++) {
        CHECK(out.events[i].note >= 60 && out.events[i].note <= 71, "stays in starting octave");
        CHECK(out.events[i].note >= c.pitch_min && out.events[i].note <= c.pitch_max, "bounds");
    }
    c.pitch_min = 67;
    c.pitch_max = 72;
    c.octave_motion = 1;
    gen_buf_clear(&out);
    CHECK(gen_markov_generate(&m, &c, &out) == GEN_OK, "tight range");
    for (i = 0; i < out.count; i++)
        CHECK(out.events[i].note >= 67 && out.events[i].note <= 72, "tight bounds");

    memset(&poisoned, 0xAB, sizeof poisoned);
    src.count = 0;
    src.events = src_ev;
    src.capacity = 8;
    CHECK(gen_markov_learn(&poisoned, GEN_SCALE_MAJOR, 60, &src, 0, &scratch) == GEN_ERR_EMPTY, "empty learn");
    CHECK(((unsigned char *)&poisoned)[0] == 0xAB, "empty leaves model");

    put_note(&src_ev[0], 0, 60);
    src.count = 1;
    CHECK(gen_markov_learn(&m, GEN_SCALE_MAJOR, 60, &src, 0, &scratch) == GEN_OK, "single learn");
    CHECK(m.ready && m.origin == 0, "single origin");
    for (i = 0; i < 7; i++)
        CHECK(m.w[0][i] == 0, "single has no edges");

    put_note(&src_ev[0], 0, 60);
    put_note(&src_ev[1], 1, 62);
    put_note(&src_ev[2], 2, 64);
    put_note(&src_ev[3], 3, 62);
    put_note(&src_ev[4], 4, 60);
    src.count = 5;
    memcpy(copy_ev, src_ev, sizeof copy_ev);
    CHECK(gen_markov_learn(&m, GEN_SCALE_MAJOR, 60, &src, 0, &scratch) == GEN_OK, "fixture learn");
    CHECK(memcmp(src_ev, copy_ev, sizeof copy_ev) == 0, "learn keeps input");
    CHECK(m.w[0][1] == 1 && m.w[1][2] == 1 && m.w[2][1] == 1 && m.w[1][0] == 1, "fixture counts");

    put_note(&src_ev[0], 0, 60);
    put_note(&src_ev[1], 1, 60);
    put_note(&src_ev[2], 2, 60);
    src.count = 3;
    CHECK(gen_markov_learn(&m, GEN_SCALE_MAJOR, 60, &src, 0, &scratch) == GEN_OK, "repeats");
    CHECK(m.w[0][0] == 2, "repeat count");

    put_note(&src_ev[0], 0, 60);
    put_note(&src_ev[1], 1, 61);
    put_note(&src_ev[2], 2, 62);
    src.count = 3;
    CHECK(gen_markov_learn(&m, GEN_SCALE_MAJOR, 60, &src, 0, &scratch) == GEN_OK, "out of scale");
    CHECK(m.w[0][1] == 0 && m.w[0][2] == 0 && m.origin == 1, "skip breaks the chain");

    put_note(&src_ev[0], 0, 60);
    put_note(&src_ev[1], 4, 64);
    src.count = 2;
    CHECK(gen_markov_learn(&m, GEN_SCALE_MAJOR, 60, &src, 480, &scratch) == GEN_OK, "gap");
    CHECK(m.w[0][2] == 0, "gap breaks");
    CHECK(gen_markov_learn(&m, GEN_SCALE_MAJOR, 60, &src, 0, &scratch) == GEN_OK, "gap off");
    CHECK(m.w[0][2] == 1, "gap 0 counts");

    src_ev[1].tick = 0;
    CHECK(gen_markov_learn(&m, GEN_SCALE_MAJOR, 60, &src, 0, &scratch) == GEN_ERR_TIMING, "unsorted");

    CHECK(gen_markov_preset(&m, GEN_SCALE_MAJOR, GEN_MARKOV_STEPWISE) == GEN_OK, "add base");
    m.w[0][1] = 65535;
    CHECK(gen_markov_add(&m, 0, 1) == GEN_ERR_OVERFLOW, "overflow");
    CHECK(m.w[0][1] == 65535, "overflow does not wrap");

    put_note(&src_ev[0], 0, 60);
    put_note(&src_ev[1], 1, 62);
    put_note(&src_ev[2], 2, 64);
    put_note(&src_ev[3], 3, 65);
    put_note(&src_ev[4], 4, 64);
    put_note(&src_ev[5], 5, 62);
    put_note(&src_ev[6], 6, 60);
    src.count = 7;
    CHECK(gen_markov_learn(&m, GEN_SCALE_MAJOR, 60, &src, 0, &scratch) == GEN_OK, "demo fixture");
    c = mcfg(88, 32, 0);
    c.scale = GEN_SCALE_MAJOR;
    c.initial_degree = 0;
    gen_buf_init(&out, storage, 32);
    CHECK(gen_markov_generate(&m, &c, &out) == GEN_OK, "learned gen");
    CHECK(mono_scale(&out, GEN_SCALE_MAJOR, 60, 48, 84, 480, 32), "learned native");
    gen_buf_clear(&out);
    CHECK(gen_markov_generate(&m, &c, &out) == GEN_OK, "learned replay");
    CHECK(out.count == 32, "learned replay count");
}

static void fill_run(gen_event *ev, int n, int root, uint8_t scale, int lo, int hi)
{
    int p = lo, i = 0;
    while (i < n) {
        if (p > hi)
            p = lo;
        if (gen_scale_contains((gen_scale)scale, root, p)) {
            put_note(&ev[i], (uint32_t)i, (uint8_t)p);
            i++;
        }
        p++;
        if (p == lo)
            break;
    }
}

static void test_mutate(void)
{
    gen_mutate_req r;
    gen_event src_ev[128], dst_ev[128], keep[128], poison;
    gen_buf src, out, out2;
    uint32_t i;

    r = mreq(1);
    put_note(&src_ev[0], 0, 60);
    put_note(&src_ev[1], 1, 64);
    put_note(&src_ev[2], 2, 67);
    put_note(&src_ev[3], 4, 72);
    src.events = src_ev;
    src.count = 4;
    src.capacity = 128;
    memcpy(keep, src_ev, sizeof keep);
    gen_buf_init(&out, dst_ev, 128);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "identity");
    CHECK(out.count == 4, "identity count");
    CHECK(events_same(&src, &out), "zero prob copies");
    CHECK(memcmp(src_ev, keep, sizeof keep) == 0, "source intact");

    gen_buf_init(&out2, keep, 128);
    out2.count = 0;
    CHECK(gen_mutate(&src, &r, &out2) == GEN_OK, "det a");
    CHECK(events_same(&out, &out2), "deterministic");

    r.pitch_prob = 100;
    r.intensity = 2;
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "pitch only");
    CHECK(memcmp(src_ev, keep, sizeof keep) == 0, "pitch source");
    for (i = 0; i < out.count; i++) {
        CHECK(out.events[i].tick == src_ev[i].tick, "pitch keeps tick");
        CHECK(out.events[i].duration == src_ev[i].duration, "pitch keeps dur");
        CHECK(out.events[i].velocity == src_ev[i].velocity, "pitch keeps vel");
        CHECK(gen_scale_contains(GEN_SCALE_MAJOR, 60, out.events[i].note), "pitch scale");
    }

    r = mreq(2);
    r.displace_prob = 100;
    r.intensity = 1;
    r.dest_steps = 8;
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "rhythm only");
    for (i = 0; i < out.count; i++)
        CHECK(out.events[i].note == src_ev[i].note || out.count == src.count, "rhythm keeps a pitch");
    CHECK(out.count == src.count, "displace keeps count");
    for (i = 0; i < out.count; i++) {
        int found = 0;
        uint32_t j;
        for (j = 0; j < src.count; j++)
            if (out.events[i].note == src_ev[j].note && out.events[i].velocity == src_ev[j].velocity)
                found = 1;
        CHECK(found, "rhythm does not invent pitches");
        CHECK(out.events[i].duration == 480, "rhythm keeps duration");
    }

    r = mreq(3);
    r.velocity_prob = 100;
    r.intensity = 8;
    r.vel_min = 40;
    r.vel_max = 40;
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "velocity only");
    for (i = 0; i < out.count; i++) {
        CHECK(out.events[i].note == src_ev[i].note, "vel keeps pitch");
        CHECK(out.events[i].tick == src_ev[i].tick, "vel keeps tick");
        CHECK(out.events[i].duration == src_ev[i].duration, "vel keeps dur");
        CHECK(out.events[i].velocity == 40, "vel clamped");
    }

    r = mreq(4);
    r.gate_prob = 100;
    r.gate_min = 240;
    r.gate_max = 240;
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "gate only");
    for (i = 0; i < out.count; i++) {
        CHECK(out.events[i].note == src_ev[i].note, "gate keeps pitch");
        CHECK(out.events[i].tick == src_ev[i].tick, "gate keeps tick");
        CHECK(out.events[i].velocity == src_ev[i].velocity, "gate keeps vel");
        CHECK(out.events[i].duration == 240, "gate changes duration");
    }

    r = mreq(5);
    r.pitch_prob = 100;
    r.displace_prob = 100;
    r.velocity_prob = 100;
    r.gate_prob = 100;
    r.protect = GEN_MUT_PROTECT_PITCH | GEN_MUT_PROTECT_RHYTHM |
                GEN_MUT_PROTECT_VELOCITY | GEN_MUT_PROTECT_DURATION;
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "protect attrs");
    CHECK(events_same(&src, &out), "protected attrs stay");

    r = mreq(6);
    r.pitch_prob = 100;
    r.intensity = 5;
    r.protect = 0;
    gen_mutate_protect_step(&r, 0, 1);
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "protect step");
    CHECK(out.events[0].note == 60 && out.events[0].tick == 0, "step 0 frozen");

    r = mreq(7);
    r.insert_prob = 100;
    r.dest_steps = 4;
    src.count = 1;
    gen_buf_init(&out, dst_ev, 1);
    poison.note = 99;
    dst_ev[0] = poison;
    CHECK(gen_mutate(&src, &r, &out) == GEN_ERR_CAPACITY, "insert capacity");
    CHECK(out.count == 0 && dst_ev[0].note == 99, "insert capacity clean");
    gen_buf_init(&out, dst_ev, 4);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "insert");
    CHECK(out.count == 2, "insert adds one");
    CHECK(out.events[0].note == 60, "insert keeps source");
    CHECK(out.events[1].note == 62, "insert is the next degree");
    CHECK(out.events[1].tick == 480, "insert takes the next step");

    r = mreq(8);
    r.remove_prob = 100;
    src.count = 4;
    gen_buf_init(&out, dst_ev, 16);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "remove");
    CHECK(out.count == 0, "remove all");
    CHECK(src_ev[0].note == 60, "remove leaves source");

    r = mreq(9);
    src.count = 2;
    put_note(&src_ev[0], 0, 60);
    put_note(&src_ev[1], 1, 64);
    r.reverse = 1;
    r.dest_steps = 2;
    gen_buf_init(&out, dst_ev, 4);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "reverse");
    CHECK(out.events[0].tick == 0 && out.events[0].note == 64, "reverse first");
    CHECK(out.events[1].tick == 480 && out.events[1].note == 60, "reverse second");

    r.reverse = 0;
    r.rotate_steps = 1;
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "rotate");
    CHECK(out.events[0].tick == 0 && out.events[0].note == 64, "rotate wrap");
    CHECK(out.events[1].tick == 480 && out.events[1].note == 60, "rotate other");

    r = mreq(10);
    r.quantize = 1;
    r.dest_steps = 1;
    put_note(&src_ev[0], 0, 61);
    src.count = 1;
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "quantize");
    CHECK(out.events[0].note == 60, "quantize tie prefers lower");

    r = mreq(11);
    r.displace_prob = 100;
    r.intensity = 1;
    r.dest_steps = 2;
    put_note(&src_ev[0], 0, 60);
    put_note(&src_ev[1], 1, 64);
    src.count = 2;
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "collision");
    CHECK(out.events[0].tick == 0 && out.events[1].tick == 480, "collision keeps both");
    CHECK(gen_buf_validate(&out, 1) == GEN_OK, "collision mono");

    src.count = 0;
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "empty");
    CHECK(out.count == 0, "empty count");

    src.count = 1;
    r = mreq(12);
    r.dest_steps = 1;
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "single");
    CHECK(out.count == 1 && out.events[0].note == src_ev[0].note, "single copy");

    fill_run(src_ev, 64, 60, GEN_SCALE_MAJOR, 36, 96);
    src.count = 64;
    r = mreq(13);
    r.dest_steps = 64;
    r.pitch_prob = 70;
    r.intensity = 2;
    memcpy(keep, src_ev, 64 * sizeof(gen_event));
    gen_buf_init(&out, dst_ev, 64);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "dense 64");
    CHECK(out.count == 64, "dense count");
    CHECK(mono_scale(&out, GEN_SCALE_MAJOR, 60, 36, 96, 480, 64), "dense native");
    CHECK(memcmp(src_ev, keep, 64 * sizeof(gen_event)) == 0, "dense source");

    fill_run(src_ev, 128, 60, GEN_SCALE_MAJOR, 36, 96);
    src.count = 128;
    r.dest_steps = 128;
    r.pitch_prob = 40;
    memcpy(keep, src_ev, 128 * sizeof(gen_event));
    gen_buf_init(&out, dst_ev, 128);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "portable 128");
    CHECK(out.count == 128, "128 count");
    CHECK(mono_scale(&out, GEN_SCALE_MAJOR, 60, 36, 96, 480, 128), "128 native shape");
    CHECK(memcmp(src_ev, keep, 128 * sizeof(gen_event)) == 0, "128 source");

    r.pitch_prob = 101;
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_ERR_PARAM, "bad prob");
    r = mreq(14);
    r.dest_steps = 16;
    src.count = 1;
    out.events = src_ev;
    out.count = 0;
    out.capacity = 16;
    CHECK(gen_mutate(&src, &r, &out) == GEN_ERR_PARAM, "alias");
    CHECK(src_ev[0].note == keep[0].note || src.count == 1, "alias source");

    gen_buf_init(&out, dst_ev, 16);
    out.count = 2;
    CHECK(gen_mutate(&src, &r, &out) == GEN_ERR_PARAM, "out not empty");
    CHECK(out.count == 2, "rejected out stays");

    put_note(&src_ev[0], 0, 80);
    src.count = 1;
    src.events = src_ev;
    r.pitch_min = 48;
    r.pitch_max = 72;
    r.dest_steps = 1;
    gen_buf_init(&out, dst_ev, 4);
    CHECK(gen_mutate(&src, &r, &out) == GEN_ERR_PITCH, "note outside bounds");
    r.quantize = 1;
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "quantize pulls it in");
    CHECK(out.events[0].note >= 48 && out.events[0].note <= 72, "pulled");

    put_note(&src_ev[0], 0, 60);
    put_note(&src_ev[1], 0, 64);
    src.count = 2;
    r = mreq(15);
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_ERR_MONO, "duplicate tick");

    put_note(&src_ev[0], 0, 60);
    src.count = 1;
    r.reverse = 1;
    gen_mutate_protect_step(&r, 0, 1);
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_ERR_UNSUPPORTED, "reverse vs protect");
    CHECK(out.count == 0, "conflict writes nothing");

    r = mreq(16);
    r.octave = 1;
    r.dest_steps = 1;
    put_note(&src_ev[0], 0, 60);
    src.count = 1;
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "octave");
    CHECK(out.events[0].note == 72, "up an octave");
    r.octave = 1;
    r.pitch_max = 70;
    gen_buf_clear(&out);
    CHECK(gen_mutate(&src, &r, &out) == GEN_ERR_PITCH, "octave out of range");
    CHECK(src_ev[0].note == 60, "rejected octave keeps source");

    fill_run(src_ev, 80, 60, GEN_SCALE_MAJOR, 36, 84);
    src.count = 80;
    r = mreq(16);
    r.dest_steps = 128;
    gen_mutate_protect_step(&r, 70, 1);
    src_ev[70].note = 60;
    r.pitch_prob = 100;
    r.intensity = 3;
    gen_buf_init(&out, dst_ev, 128);
    CHECK(gen_mutate(&src, &r, &out) == GEN_OK, "protect bit 70");
    CHECK(out.events[70].note == 60 && out.events[70].tick == 70u * 480u, "step 70 frozen");
}

static void test_properties(void)
{
    uint32_t seed;
    for (seed = 1; seed <= 100; seed++) {
        gen_turing st, next, again;
        gen_turing_cfg tc = tcfg(seed, (uint8_t)(1 + (seed % 64)), 32, (uint8_t)(seed % 101),
                                 (uint8_t)((seed * 3) % 101), (uint8_t)(seed & 1));
        gen_event ev[32], ev2[32];
        gen_buf b, b2;
        gen_markov_model model;
        gen_markov_cfg mc;
        gen_mutate_req rq;
        gen_event src_ev[16], dst_ev[32], keep[16];
        gen_buf src, dst;
        uint8_t style = (uint8_t)(seed % 4);
        gen_turing frozen;
        if (gen_turing_init(&st, &tc) != GEN_OK)
            continue;
        frozen = st;
        gen_buf_init(&b, ev, 32);
        gen_buf_init(&b2, ev2, 32);
        CHECK(gen_turing_generate(&st, NULL, &b, &next) == GEN_OK, "prop turing");
        CHECK(memcmp(&st, &frozen, sizeof st) == 0, "prop preview leaves state");
        CHECK(gen_turing_generate(&st, NULL, &b2, &again) == GEN_OK, "prop turing replay");
        CHECK(events_same(&b, &b2), "prop turing det");
        CHECK(mono_scale(&b, tc.scale, tc.root, tc.pitch_min, tc.pitch_max, 480, 32) || b.count == 0,
              "prop turing native");

        CHECK(gen_markov_preset(&model, GEN_SCALE_DORIAN, style) == GEN_OK, "prop preset");
        mc = mcfg(seed, 32, (uint8_t)(seed % 50));
        mc.scale = GEN_SCALE_DORIAN;
        gen_buf_clear(&b);
        gen_buf_clear(&b2);
        CHECK(gen_markov_generate(&model, &mc, &b) == GEN_OK, "prop markov");
        CHECK(gen_markov_generate(&model, &mc, &b2) == GEN_OK, "prop markov replay");
        CHECK(events_same(&b, &b2), "prop markov det");
        CHECK(mono_scale(&b, mc.scale, mc.root, mc.pitch_min, mc.pitch_max, 480, 32) || b.count == 0,
              "prop markov native");

        fill_run(src_ev, 8, 60, GEN_SCALE_MAJOR, 48, 72);
        memcpy(keep, src_ev, 8 * sizeof(gen_event));
        src.events = src_ev;
        src.count = 8;
        src.capacity = 16;
        rq = mreq(seed);
        rq.dest_steps = 16;
        rq.pitch_prob = (uint8_t)(seed % 101);
        rq.displace_prob = (uint8_t)((seed * 5) % 101);
        rq.velocity_prob = (uint8_t)((seed * 7) % 101);
        rq.intensity = (uint8_t)(1 + (seed % 5));
        rq.vel_min = 40;
        rq.vel_max = 110;
        gen_buf_init(&dst, dst_ev, 32);
        CHECK(gen_mutate(&src, &rq, &dst) == GEN_OK, "prop mutate");
        CHECK(memcmp(src_ev, keep, 8 * sizeof(gen_event)) == 0, "prop source");
        CHECK(gen_buf_validate(&dst, 1) == GEN_OK, "prop mono");
        CHECK(dst.count <= 16, "prop bound");
    }
}

static void test_rotate_and_steps(void)
{
    gen_event ev[4], src[4], dst[8], keep[4];
    gen_buf b, in, out;
    gen_mutate_req r;
    int i;
    memset(ev, 0, sizeof ev);
    for (i = 0; i < 4; i++)
        put_note(&ev[i], (uint32_t)i, (uint8_t)(60 + i));
    gen_buf_init(&b, ev, 4);
    b.count = 4;
    CHECK(gen_xform_rotate(&b, 1, 480) == GEN_OK, "rot +1");
    for (i = 0; i < 4; i++)
        if (ev[i].note == 60)
            CHECK(ev[i].tick == 480, "plus shifts forward");
    for (i = 0; i < 4; i++)
        put_note(&ev[i], (uint32_t)i, (uint8_t)(60 + i));
    b.count = 4;
    CHECK(gen_xform_rotate(&b, -1, 480) == GEN_OK, "rot -1");
    for (i = 0; i < 4; i++)
        if (ev[i].note == 60)
            CHECK(ev[i].tick == 1440, "minus wraps to the last step");
    for (i = 0; i < 4; i++)
        put_note(&ev[i], (uint32_t)i, (uint8_t)(60 + i));
    b.count = 4;
    CHECK(gen_xform_rotate(&b, 4, 480) == GEN_OK, "rot +4");
    for (i = 0; i < 4; i++)
        if (ev[i].note == 60)
            CHECK(ev[i].tick == 0, "full cycle is identity");

    for (i = 0; i < 4; i++)
        put_note(&src[i], (uint32_t)i, (uint8_t)(60 + i));
    memcpy(keep, src, sizeof keep);
    in.events = src;
    in.count = 4;
    in.capacity = 4;
    r = mreq(9);
    r.dest_steps = 4;
    r.pitch_prob = 100;
    r.intensity = 12;
    gen_mutate_protect_step(&r, 0, 1);
    gen_mutate_protect_step(&r, 2, 1);
    gen_buf_init(&out, dst, 8);
    CHECK(gen_mutate(&in, &r, &out) == GEN_OK, "two protected steps");
    CHECK(dst[0].note == 60 && dst[2].note == 62, "protected pitches stay");
    CHECK(memcmp(src, keep, sizeof keep) == 0, "protect source");
}

int main(void)
{
    test_turing();
    test_markov();
    test_mutate();
    test_rotate_and_steps();
    test_properties();
    if (g_fail) {
        fprintf(stderr, "%d evolution checks failed\n", g_fail);
        return 1;
    }
    printf("All evolution tests passed\n");
    printf("sizeof turing_cfg=%u turing=%u markov_cfg=%u markov_model=%u mutate_req=%u event=%u\n",
           (unsigned)sizeof(gen_turing_cfg), (unsigned)sizeof(gen_turing),
           (unsigned)sizeof(gen_markov_cfg), (unsigned)sizeof(gen_markov_model),
           (unsigned)sizeof(gen_mutate_req), (unsigned)sizeof(gen_event));
    return 0;
}
