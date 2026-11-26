/* G5C.1 session tests: preview, commit, discard, native limits. */
#include "genesis_evo.h"
#include <stdio.h>
#include <string.h>

static int g_fail;

static void check(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", msg);
        g_fail++;
    }
}

static gen_turing_cfg tcfg(uint32_t seed, uint8_t length, uint16_t steps, uint8_t evolve, uint8_t rest, uint8_t lock)
{
    gen_turing_cfg c;
    memset(&c, 0, sizeof c);
    c.gate = 360;
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
    c.velocity = 100;
    c.locked = lock;
    return c;
}

static gen_markov_cfg mcfg(uint32_t seed, uint16_t steps, uint8_t rest)
{
    gen_markov_cfg c;
    memset(&c, 0, sizeof c);
    c.gate = 360;
    c.ticks_per_step = 480;
    c.seed = seed;
    c.root = 57;
    c.pitch_min = 48;
    c.pitch_max = 76;
    c.initial_degree = 0;
    c.steps = steps;
    c.scale = GEN_SCALE_NAT_MINOR;
    c.velocity = 96;
    c.rest_prob = rest;
    c.octave_motion = 1;
    return c;
}

static void fill_phrase(gen_event *ev, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        memset(&ev[i], 0, sizeof ev[i]);
        ev[i].tick = (uint32_t)i * 480u;
        ev[i].note = (uint8_t)(60 + (i % 5));
        ev[i].velocity = 100;
        ev[i].duration = 360;
    }
}

static int same_events(const gen_buf *a, const gen_buf *b)
{
    uint32_t i;
    if (a->count != b->count)
        return 0;
    for (i = 0; i < a->count; i++) {
        if (a->events[i].tick != b->events[i].tick || a->events[i].note != b->events[i].note ||
            a->events[i].velocity != b->events[i].velocity || a->events[i].duration != b->events[i].duration)
            return 0;
    }
    return 1;
}

static void test_steps(void)
{
    unsigned ok[] = {1, 16, 32, 64};
    unsigned bad[] = {0, 65, 128};
    unsigned i;
    for (i = 0; i < 4; i++)
        check(gen_evo_native_steps(ok[i]) == GEN_OK, "native length");
    for (i = 0; i < 3; i++)
        check(gen_evo_native_steps(bad[i]) == GEN_ERR_PARAM, "reject 65/128");
}

static void test_turing(void)
{
    gen_evo e;
    gen_turing_cfg c = tcfg(11, 8, 4, 0, 0, 0);
    gen_turing frozen, after;
    gen_event a[16], b[16];
    gen_buf ba, bb;
    uint8_t cells[8];
    gen_evo_init(&e);
    gen_buf_init(&ba, a, 16);
    gen_buf_init(&bb, b, 16);
    check(gen_evo_turing_reinit(&e, &c) == GEN_OK, "reset init");
    frozen = e.committed;
    check(gen_evo_turing_reinit(&e, &c) == GEN_OK, "reset again");
    check(memcmp(&e.committed, &frozen, sizeof frozen) == 0, "reset deterministic");
    check(e.phase == GEN_EVO_EMPTY, "reset writes no preview");

    check(gen_evo_turing_preview(&e, &c, &ba, 1) == GEN_OK, "preview");
    frozen = e.committed;
    memcpy(cells, e.committed.cells, 8);
    gen_buf_clear(&bb);
    check(gen_evo_turing_preview(&e, &c, &bb, 1) == GEN_OK, "preview again");
    check(same_events(&ba, &bb), "repeated preview");
    check(memcmp(&e.committed, &frozen, sizeof frozen) == 0, "preview keeps committed");
    check(memcmp(e.committed.cells, cells, 8) == 0, "zero evolve cells");

    gen_evo_discard(&e);
    check(e.phase == GEN_EVO_EMPTY && e.proposed_ok == 0, "cancel");
    check(memcmp(&e.committed, &frozen, sizeof frozen) == 0, "cancel keeps committed");
    check(gen_evo_apply_result(&e, 1) == GEN_ERR_PARAM, "apply without preview");

    gen_buf_clear(&ba);
    check(gen_evo_turing_preview(&e, &c, &ba, 7) == GEN_OK, "preview bind 7");
    check(gen_evo_apply_result(&e, 0) == GEN_OK, "failed apply");
    check(memcmp(&e.committed, &frozen, sizeof frozen) == 0, "failed apply keeps state");
    check(e.phase == GEN_EVO_VALID, "failed apply keeps preview");
    after = e.committed;
    check(gen_evo_apply_result(&e, 1) == GEN_OK, "apply once");
    check(e.phase == GEN_EVO_APPLIED, "applied");
    check(e.committed.pos == 4, "advanced once");
    check(memcmp(e.committed.cells, cells, 8) == 0, "zero evolve still the register");
    after = e.committed;
    check(gen_evo_apply_result(&e, 1) == GEN_ERR_PARAM, "second apply");
    check(memcmp(&e.committed, &after, sizeof after) == 0, "no second advance");

    c.locked = 1;
    c.evolve_prob = 100;
    c.steps = 4;
    gen_evo_init(&e);
    gen_buf_clear(&ba);
    check(gen_evo_turing_preview(&e, &c, &ba, 3) == GEN_OK, "lock preview");
    memcpy(cells, e.committed.cells, 8);
    check(memcmp(e.proposed.cells, cells, 8) == 0, "lock does not rewrite");
    check(gen_evo_apply_result(&e, 1) == GEN_OK, "lock apply");
    check(memcmp(e.committed.cells, cells, 8) == 0, "lock commit keeps cells");

    c = tcfg(11, 8, 4, 100, 0, 0);
    gen_evo_init(&e);
    gen_buf_clear(&ba);
    check(gen_evo_turing_preview(&e, &c, &ba, 9) == GEN_OK, "evolve preview");
    frozen = e.committed;
    gen_evo_set_live(&e, 10);
    check(e.phase == GEN_EVO_STALE, "stale");
    check(gen_evo_apply_result(&e, 1) == GEN_ERR_PARAM, "stale apply");
    check(memcmp(&e.committed, &frozen, sizeof frozen) == 0, "stale keeps committed");

    gen_evo_set_live(&e, 9);
    e.phase = GEN_EVO_VALID;
    e.proposed_ok = 1;
    gen_evo_set_mode(&e, GEN_EVO_MARKOV);
    check(gen_evo_apply_result(&e, 1) == GEN_ERR_PARAM, "mode switch drops preview");
    check(memcmp(&e.committed, &frozen, sizeof frozen) == 0, "mode switch keeps turing");

    c.steps = 65;
    check(gen_evo_turing_preview(&e, &c, &ba, 1) == GEN_ERR_PARAM, "turing 65");
    c.steps = 128;
    check(gen_evo_turing_preview(&e, &c, &ba, 1) == GEN_ERR_PARAM, "turing 128");
}

static void test_markov(void)
{
    gen_evo e;
    gen_markov_cfg c;
    gen_event ev[64], phrase[8], copy[8];
    gen_buf out, src;
    gen_markov_model saved;
    uint8_t style;
    uint32_t h0;
    int i;
    uint8_t notes[] = {60, 62, 64, 67, 64, 62, 60};
    gen_evo_init(&e);
    gen_buf_init(&out, ev, 64);
    for (style = 0; style < 4; style++) {
        c = mcfg(20 + style, 16, 0);
        check(gen_evo_markov_preview(&e, &c, style, &out, 1) == GEN_OK, "preset");
        check(gen_evo_audit(&out, 16, 480) == GEN_OK, "preset audit");
        for (i = 0; i < (int)out.count; i++)
            check(gen_scale_contains(GEN_SCALE_NAT_MINOR, 57, ev[i].note), "preset scale");
        gen_buf_clear(&out);
        check(e.model_ready == 0, "preset does not replace learned slot");
    }
    c = mcfg(55, 32, 0);
    check(gen_evo_markov_preview(&e, &c, GEN_MARKOV_LEARNED, &out, 1) == GEN_ERR_EMPTY, "learned before learn");

    memset(phrase, 0, sizeof phrase);
    for (i = 0; i < 7; i++) {
        phrase[i].tick = (uint32_t)i * 480u;
        phrase[i].note = notes[i];
        phrase[i].velocity = 90;
        phrase[i].duration = 360;
    }
    src.events = phrase;
    src.count = 7;
    src.capacity = 8;
    memcpy(copy, phrase, sizeof copy);
    saved = e.model;
    check(gen_evo_markov_learn(&e, GEN_SCALE_MAJOR, 60, &src, 0) == GEN_OK, "learn");
    check(memcmp(phrase, copy, sizeof copy) == 0, "learn source");
    check(memcmp(&e.model, &saved, sizeof saved) == 0, "learn waits for apply");
    c = mcfg(88, 16, 0);
    c.root = 60;
    c.scale = GEN_SCALE_MAJOR;
    c.pitch_min = 60;
    c.pitch_max = 84;
    h0 = gen_evo_hash(2166136261u, &e.pending, sizeof e.pending);
    check(gen_evo_markov_preview(&e, &c, GEN_MARKOV_LEARNED, &out, 4) == GEN_OK, "learned gen");
    check(gen_evo_hash(2166136261u, &e.pending, sizeof e.pending) == h0, "gen keeps model");
    check(memcmp(&e.model, &saved, sizeof saved) == 0, "gen does not publish");
    check(gen_evo_apply_result(&e, 0) == GEN_OK, "learn apply fail");
    check(memcmp(&e.model, &saved, sizeof saved) == 0, "failed apply keeps model");
    check(gen_evo_apply_result(&e, 1) == GEN_OK, "learn apply");
    check(e.model_ready == 1 && e.model.style == GEN_MARKOV_LEARNED, "published");
    check(gen_evo_apply_result(&e, 1) == GEN_ERR_PARAM, "learn apply once");

    gen_evo_init(&e);
    saved = e.model;
    src.count = 0;
    check(gen_evo_markov_learn(&e, GEN_SCALE_MAJOR, 60, &src, 0) == GEN_ERR_EMPTY, "empty learn");
    check(memcmp(&e.model, &saved, sizeof saved) == 0, "empty keeps model");
    check(e.pending_ok == 0, "empty does not publish");

    phrase[1].tick = phrase[0].tick;
    src.count = 2;
    saved = e.model;
    check(gen_evo_markov_learn(&e, GEN_SCALE_MAJOR, 60, &src, 0) == GEN_ERR_TIMING, "unordered learn");
    check(memcmp(&e.model, &saved, sizeof saved) == 0, "failed learn keeps model");

    c.steps = 128;
    check(gen_evo_markov_preview(&e, &c, GEN_MARKOV_STEPWISE, &out, 1) == GEN_ERR_PARAM, "markov 128");
}

static void test_mutate(void)
{
    gen_evo e;
    gen_event src_ev[16], dst[16], keep[16];
    gen_buf src, out;
    gen_mutate_req r;
    int i;
    gen_evo_init(&e);
    fill_phrase(src_ev, 8);
    memcpy(keep, src_ev, sizeof keep);
    src.events = src_ev;
    src.count = 8;
    src.capacity = 16;
    memset(&r, 0, sizeof r);
    r.ticks_per_step = 480;
    r.gate_min = 240;
    r.gate_max = 360;
    r.seed = 5;
    r.root = 60;
    r.pitch_min = 48;
    r.pitch_max = 84;
    r.dest_steps = 8;
    r.scale = GEN_SCALE_MAJOR;
    r.vel_min = 1;
    r.vel_max = 127;
    r.intensity = 2;
    gen_buf_init(&out, dst, 16);
    check(gen_evo_mutate_preview(&e, &src, &r, &out, 1) == GEN_OK, "identity");
    check(same_events(&src, &out), "zero prob");
    check(memcmp(src_ev, keep, 8 * sizeof(gen_event)) == 0, "source");

    gen_buf_clear(&out);
    r.pitch_prob = 100;
    r.protect = GEN_MUT_PROTECT_PITCH;
    check(gen_evo_mutate_preview(&e, &src, &r, &out, 2) == GEN_OK, "pitch protect");
    for (i = 0; i < 8; i++)
        check(dst[i].note == src_ev[i].note && dst[i].tick == src_ev[i].tick, "pitch held");
    check(memcmp(src_ev, keep, 8 * sizeof(gen_event)) == 0, "source after pitch");

    gen_buf_clear(&out);
    r.protect = 0;
    r.pitch_prob = 0;
    r.displace_prob = 100;
    check(gen_evo_mutate_preview(&e, &src, &r, &out, 3) == GEN_OK, "full displace");
    for (i = 0; i < 8; i++)
        check(dst[i].tick == src_ev[i].tick, "occupied grid does not move");

    gen_buf_clear(&out);
    r.displace_prob = 0;
    r.pitch_prob = 100;
    r.protect = 0;
    gen_mutate_protect_step(&r, 0, 1);
    check(gen_evo_mutate_preview(&e, &src, &r, &out, 4) == GEN_OK, "step protect");
    check(dst[0].note == src_ev[0].note && dst[0].tick == src_ev[0].tick, "step 0 frozen");

    gen_buf_clear(&out);
    r.reverse = 1;
    r.protect = GEN_MUT_PROTECT_RHYTHM;
    check(gen_evo_mutate_preview(&e, &src, &r, &out, 5) == GEN_ERR_UNSUPPORTED, "reverse vs rhythm");
    check(out.count == 0, "unsupported writes nothing");
    check(memcmp(src_ev, keep, 8 * sizeof(gen_event)) == 0, "source after reject");

    r.reverse = 0;
    r.protect = 0;
    gen_buf_init(&out, dst, 0);
    out.count = 0;
    check(gen_evo_mutate_preview(&e, &src, &r, &out, 6) == GEN_ERR_CAPACITY, "capacity");
    check(out.count == 0, "capacity leaves empty");

    gen_buf_init(&out, src_ev, 16);
    out.count = 0;
    check(gen_evo_mutate_preview(&e, &src, &r, &out, 7) == GEN_ERR_PARAM, "alias");
    check(memcmp(src_ev, keep, 8 * sizeof(gen_event)) == 0, "alias source");

    r.dest_steps = 65;
    gen_buf_init(&out, dst, 16);
    check(gen_evo_mutate_preview(&e, &src, &r, &out, 8) == GEN_ERR_PARAM, "mutate 65");
}

static void test_audit(void)
{
    gen_event ev[4];
    gen_buf b;
    fill_phrase(ev, 1);
    gen_buf_init(&b, ev, 4);
    b.count = 1;
    check(gen_evo_audit(&b, 16, 480) == GEN_OK, "audit ok");
    ev[0].note = 200;
    check(gen_evo_audit(&b, 16, 480) == GEN_ERR_PARAM, "bad note");
    ev[0].note = 60;
    ev[0].velocity = 0;
    check(gen_evo_audit(&b, 16, 480) == GEN_ERR_PARAM, "bad vel");
    ev[0].velocity = 100;
    ev[0].duration = 3;
    check(gen_evo_audit(&b, 16, 480) == GEN_ERR_TIMING, "bad dur");
    ev[0].duration = 360;
    ev[0].tick = 100;
    check(gen_evo_audit(&b, 16, 480) == GEN_ERR_TIMING, "off grid");
    ev[0].tick = 0;
    check(gen_evo_audit(&b, 128, 480) == GEN_ERR_PARAM, "audit 128");
}

int main(void)
{
    test_steps();
    test_turing();
    test_markov();
    test_mutate();
    test_audit();
    if (g_fail) {
        fprintf(stderr, "%d session checks failed\n", g_fail);
        return 1;
    }
    printf("All evolution session tests passed\n");
    printf("sizeof gen_evo=%u\n", (unsigned)sizeof(gen_evo));
    return 0;
}
