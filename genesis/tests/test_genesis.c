/* Deterministic unit + property tests for Genesis. Host only. */
#include "genesis.h"
#include "genesis_smf.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int g_fail;

#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); g_fail++; } \
} while (0)

static void test_rng_kat(void)
{
    gen_rng a, b;
    uint32_t i, xa, xb;
    gen_rng_seed(&a, 0);
    gen_rng_seed(&b, 0xA5A5A5A5u);
    for (i = 0; i < 8; i++) {
        xa = gen_rng_u32(&a);
        xb = gen_rng_u32(&b);
        CHECK(xa == xb, "zero seed remaps to fixed state");
    }
    gen_rng_seed(&a, 1);
    gen_rng_seed(&b, 1);
    for (i = 0; i < 32; i++)
        CHECK(gen_rng_u32(&a) == gen_rng_u32(&b), "seeded reproducibility");
    /* Known answers for seed=1 after remapping none */
    gen_rng_seed(&a, 1);
    CHECK(gen_rng_u32(&a) == 270369u, "KAT1"); /* xorshift32(1) */
}

static void test_scales(void)
{
    CHECK(gen_scale_contains(GEN_SCALE_MAJOR, 60, 60), "C in C major");
    CHECK(gen_scale_contains(GEN_SCALE_MAJOR, 60, 64), "E in C major");
    CHECK(!gen_scale_contains(GEN_SCALE_MAJOR, 60, 61), "C# not in C major");
    CHECK(gen_scale_quantize(GEN_SCALE_MAJOR, 60, 61, 0, 127) == 60, "quantize tie lower");
    CHECK(gen_clamp_midi(200) == 127, "clamp high");
    CHECK(gen_clamp_midi(-3) == 0, "clamp low");
    CHECK(gen_scale_degree_count(GEN_SCALE_MAJOR) == 7, "major degrees");
    CHECK(gen_scale_degree_count(GEN_SCALE_MAJOR_PENT) == 5, "pent degrees");
}

static void test_euclidean(void)
{
    gen_rhythm r;
    uint16_t pulses[] = {0, 1, 5, 8, 16};
    uint16_t steps[] = {8, 16, 16, 16, 16};
    size_t i;
    for (i = 0; i < sizeof pulses / sizeof pulses[0]; i++) {
        CHECK(gen_rhythm_euclidean(&r, steps[i], pulses[i], 0, 120) == GEN_OK, "euclid ok");
        CHECK(gen_rhythm_pulse_count(&r) == pulses[i], "euclid pulse count");
    }
    CHECK(gen_rhythm_euclidean(&r, 16, 5, 3, 120) == GEN_OK, "euclid rotate");
    CHECK(gen_rhythm_pulse_count(&r) == 5, "rotate preserves pulses");
    CHECK(gen_rhythm_euclidean(&r, 16, 17, 0, 120) == GEN_ERR_PARAM, "pulses>steps");
    /* zero and full */
    CHECK(gen_rhythm_euclidean(&r, 16, 0, 0, 60) == GEN_OK && gen_rhythm_pulse_count(&r) == 0, "zero");
    CHECK(gen_rhythm_euclidean(&r, 16, 16, 0, 60) == GEN_OK && gen_rhythm_pulse_count(&r) == 16, "full");
}

static void test_melody_det(void)
{
    gen_event storage[128], storage2[128];
    gen_buf a, b;
    gen_rhythm r;
    gen_melody_cfg cfg;
    memset(&cfg, 0, sizeof cfg);
    gen_buf_init(&a, storage, 128);
    gen_buf_init(&b, storage2, 128);
    CHECK(gen_rhythm_euclidean(&r, 16, 8, 0, 120) == GEN_OK, "rhythm");
    cfg.root = 60;
    cfg.scale = GEN_SCALE_NAT_MINOR;
    cfg.pitch_min = 48;
    cfg.pitch_max = 72;
    cfg.vel_min = 80;
    cfg.vel_max = 100;
    cfg.duration = 100;
    cfg.channel = 0;
    cfg.seed = 42;
    cfg.max_interval = 2;
    cfg.repeat_prob = 10;
    CHECK(gen_melody_uniform(&cfg, &r, &a) == GEN_OK, "melody a");
    CHECK(gen_melody_uniform(&cfg, &r, &b) == GEN_OK, "melody b");
    CHECK(a.count == b.count, "same count");
    CHECK(memcmp(a.events, b.events, a.count * sizeof(gen_event)) == 0, "identical seed");
    CHECK(gen_buf_validate(&a, 1) == GEN_OK, "mono ok for non-overlap rhythm");
}

static void test_chords_arp(void)
{
    gen_chord c, c2;
    gen_event storage[256];
    gen_buf b;
    gen_arp_cfg ac;
    gen_progression p;
    gen_harmony_cfg hc;
    memset(&ac, 0, sizeof ac);
    memset(&hc, 0, sizeof hc);
    gen_buf_init(&b, storage, 256);
    CHECK(gen_chord_build(&c, 60, GEN_CHORD_MAJ, 0, 0) == GEN_OK, "Cmaj");
    CHECK(c.count == 3 && c.notes[0] == 60 && c.notes[1] == 64 && c.notes[2] == 67, "C E G");
    CHECK(gen_chord_build(&c2, 60, GEN_CHORD_MAJ, 1, 0) == GEN_OK, "inv1");
    ac.mode = GEN_ARP_UP;
    ac.rate = 120;
    ac.gate = 100;
    ac.octaves = 1;
    ac.length = 0;
    ac.velocity = 100;
    ac.channel = 0;
    ac.seed = 1;
    CHECK(gen_arp_chord(&ac, &c, &b) == GEN_OK, "arp");
    CHECK(b.count == 3, "arp 3 notes");
    CHECK(b.events[0].note == 60 && b.events[1].note == 64 && b.events[2].note == 67, "arp up");
    gen_buf_clear(&b);
    ac.mode = GEN_ARP_UPDOWN;
    ac.length = 5;
    CHECK(gen_arp_chord(&ac, &c, &b) == GEN_OK && b.count == 5, "updown len");
    gen_prog_I_V_vi_IV(&p);
    hc.root = 60;
    hc.scale = GEN_SCALE_MAJOR;
    hc.quality = GEN_CHORD_TRIAD;
    hc.ticks_per_chord = 480;
    hc.duration = 400;
    hc.channel = 0;
    hc.velocity = 90;
    hc.voice_lead = 1;
    gen_buf_clear(&b);
    CHECK(gen_harmony_progression(&hc, &p, &b) == GEN_OK, "prog");
    CHECK(b.count == 12, "4 chords * 3");
}

static void test_capacity_and_errors(void)
{
    gen_event tiny[2];
    gen_buf b;
    gen_rhythm r;
    gen_melody_cfg cfg;
    memset(&cfg, 0, sizeof cfg);
    gen_buf_init(&b, tiny, 2);
    CHECK(gen_rhythm_euclidean(&r, 16, 8, 0, 60) == GEN_OK, "r");
    cfg.root = 60;
    cfg.scale = GEN_SCALE_MAJOR;
    cfg.pitch_min = 60;
    cfg.pitch_max = 72;
    cfg.vel_min = 64;
    cfg.vel_max = 64;
    cfg.duration = 50;
    cfg.channel = 0;
    cfg.seed = 1;
    CHECK(gen_melody_uniform(&cfg, &r, &b) == GEN_ERR_CAPACITY, "capacity rolls back");
    CHECK(b.count == 0, "no partial success");
    CHECK(gen_arp(&(gen_arp_cfg){0}, (uint8_t[]){60}, 1, &b) == GEN_ERR_PARAM, "bad arp cfg");
}

static void test_long_lengths(void)
{
    static const int lens[] = {16, 32, 64, 128};
    gen_event storage[128];
    gen_buf b;
    gen_rhythm r;
    gen_melody_cfg cfg;
    gen_arp_cfg ac;
    uint8_t notes[3] = {57, 60, 64}; /* A minor pent-ish triad; A=57 */
    int i, n;
    memset(&cfg, 0, sizeof cfg);
    cfg.root = 60;
    cfg.scale = GEN_SCALE_NAT_MINOR;
    cfg.pitch_min = 48;
    cfg.pitch_max = 72;
    cfg.vel_min = 80;
    cfg.vel_max = 80;
    cfg.duration = 480;
    cfg.seed = 42;
    for (i = 0; i < 4; i++) {
        n = lens[i];
        CHECK(gen_rhythm_euclidean(&r, (uint16_t)n, (uint16_t)(n / 2), 0, 480) == GEN_OK, "euclid len");
        CHECK(r.step_count == (uint16_t)n, "euclid steps");
        gen_buf_init(&b, storage, (uint32_t)n);
        CHECK(gen_melody_uniform(&cfg, &r, &b) == GEN_OK, "melody len");
        CHECK(b.count > 0 && b.events[b.count - 1].tick / 480 < (uint32_t)n, "melody in range");
    }
    CHECK(gen_rhythm_euclidean(&r, 129, 8, 0, 480) == GEN_ERR_PARAM, "129 rejected");
    CHECK(gen_rhythm_probability(&r, 0, 480, 50, &(gen_rng){.state = 1}) == GEN_ERR_PARAM, "zero steps");
    gen_buf_init(&b, storage, 127);
    CHECK(gen_rhythm_euclidean(&r, 128, 128, 0, 480) == GEN_OK, "128 hits");
    CHECK(gen_melody_uniform(&cfg, &r, &b) == GEN_ERR_CAPACITY, "no silent truncate");
    CHECK(b.count == 0, "capacity rollback");
    memset(&ac, 0, sizeof ac);
    ac.mode = GEN_ARP_UP;
    ac.rate = 480;
    ac.gate = 240;
    ac.octaves = 1;
    ac.length = 128;
    ac.velocity = 90;
    ac.seed = 1;
    gen_buf_init(&b, storage, 128);
    CHECK(gen_arp(&ac, notes, 3, &b) == GEN_OK && b.count == 128, "arp 128");
    ac.length = 200;
    gen_buf_init(&b, storage, 128);
    CHECK(gen_arp(&ac, notes, 3, &b) == GEN_ERR_CAPACITY && b.count == 0, "arp over buffer");
}

static void test_transforms(void)
{
    gen_event storage[32];
    gen_buf b;
    gen_rng rng;
    gen_event e = {.tick = 0, .note = 60, .velocity = 100, .channel = 0, .duration = 100};
    gen_buf_init(&b, storage, 32);
    gen_buf_push(&b, &e);
    e.tick = 120;
    e.note = 64;
    gen_buf_push(&b, &e);
    CHECK(gen_xform_transpose(&b, 2) == GEN_OK && b.events[0].note == 62, "transpose");
    CHECK(gen_xform_transpose(&b, 100) == GEN_ERR_PITCH, "transpose overflow");
    gen_rng_seed(&rng, 7);
    b.events[0].note = 60;
    b.events[1].note = 64;
    CHECK(gen_xform_mutate_pitch(&b, GEN_SCALE_MAJOR, 60, 48, 72, 100, &rng) == GEN_OK, "mutate");
}

static void test_smf(void)
{
    gen_event storage[16];
    gen_buf b;
    gen_smf_opts o = {.tpq = 480, .tempo_us = 500000};
    gen_event e = {.tick = 0, .note = 60, .velocity = 100, .channel = 0, .duration = 240};
    char path[] = "/tmp/genesis_test.mid";
    FILE *fp;
    uint8_t hdr[8];
    gen_buf_init(&b, storage, 16);
    gen_buf_push(&b, &e);
    e.tick = 240;
    e.note = 64;
    gen_buf_push(&b, &e);
    CHECK(gen_smf_write_path(path, &b, &o) == 0, "smf write");
    fp = fopen(path, "rb");
    CHECK(fp != NULL, "open mid");
    if (fp) {
        CHECK(fread(hdr, 1, 8, fp) == 8, "read hdr");
        CHECK(memcmp(hdr, "MThd", 4) == 0, "MThd");
        fclose(fp);
    }
}

static void test_poly_preserve(void)
{
    gen_event storage[8];
    gen_buf b;
    gen_event e = {.tick = 0, .note = 60, .velocity = 100, .channel = 0, .duration = 480};
    gen_buf_init(&b, storage, 8);
    gen_buf_push(&b, &e);
    e.note = 64;
    gen_buf_push(&b, &e);
    e.note = 67;
    gen_buf_push(&b, &e);
    CHECK(b.count == 3, "poly chord kept");
    CHECK(gen_buf_validate(&b, 0) == GEN_OK, "poly validate");
    CHECK(gen_buf_validate(&b, 1) == GEN_ERR_MONO, "mono conflict detected");
}

static void property_fuzz(void)
{
    uint32_t seed;
    for (seed = 0; seed < 50; seed++) {
        gen_event storage[256];
        gen_buf b;
        gen_rhythm r;
        gen_melody_cfg cfg;
        uint32_t i;
        memset(&cfg, 0, sizeof cfg);
        gen_buf_init(&b, storage, 256);
        CHECK(gen_rhythm_euclidean(&r, 32, (uint16_t)(seed % 33), (uint16_t)(seed % 32), 60) == GEN_OK, "fuzz eu");
        cfg.root = 48 + (int)(seed % 24);
        cfg.scale = (gen_scale)(seed % GEN_SCALE_COUNT);
        cfg.pitch_min = 36;
        cfg.pitch_max = 84;
        cfg.vel_min = 1;
        cfg.vel_max = 127;
        cfg.duration = 50;
        cfg.channel = (uint8_t)(seed % 16);
        cfg.seed = seed;
        cfg.max_interval = 3;
        if (gen_melody_walk(&cfg, &r, &b) != GEN_OK)
            continue;
        for (i = 0; i < b.count; i++) {
            CHECK(b.events[i].note <= 127, "note range");
            CHECK(b.events[i].velocity >= 1 && b.events[i].velocity <= 127, "vel");
            CHECK(b.events[i].duration > 0, "dur");
            CHECK(gen_scale_contains(cfg.scale, cfg.root, b.events[i].note), "in scale");
        }
    }
}

static void fill_nchord(gen_nchord_cfg *c)
{
    memset(c, 0, sizeof *c);
    c->root = 60;
    c->scale = GEN_SCALE_MAJOR;
    c->prog = GEN_NPROG_I_V_vi_IV;
    c->mode = GEN_NMODE_TRIAD;
    c->fixed_id = GEN_NID_MAJOR;
    c->steps = 16;
    c->interval = 4;
    c->velocity = 100;
    c->hold = GEN_NHOLD_SUSTAIN;
    c->gate_ticks = 480;
    c->ticks_per_step = 480;
    c->seed = 1;
}

static void test_native_progression(void)
{
    gen_nchord_cfg c;
    gen_event ev[64], ev2[64];
    uint8_t ids[64], ids2[64];
    gen_buf b, b2;
    gen_status st;
    int i, seed, uniq = 0;
    uint8_t seen[128];

    /* Major I–V–vi–IV triads: maj, maj, min, maj. Not every degree is major. */
    fill_nchord(&c);
    gen_buf_init(&b, ev, 64);
    st = gen_native_progression(&c, &b, ids, 64);
    CHECK(st == GEN_OK && b.count == 4, "major I-V-vi-IV count");
    CHECK(ids[0] == GEN_NID_MAJOR && ids[1] == GEN_NID_MAJOR && ids[2] == GEN_NID_MINOR &&
              ids[3] == GEN_NID_MAJOR,
          "major triad qualities");
    CHECK(ev[0].note == 60 && ev[1].note == 67 && ev[2].note == 69 && ev[3].note == 65,
          "major roots C G A F");
    CHECK(ev[0].duration == 4 * 480u && ev[0].velocity == 100 && ev[0].tick == 0, "sustain span");
    CHECK(ev[3].tick == 12 * 480u, "fourth chord step");

    /* Natural minor keeps the same degrees and changes the qualities. */
    c.scale = GEN_SCALE_NAT_MINOR;
    gen_buf_init(&b, ev, 64);
    st = gen_native_progression(&c, &b, ids, 64);
    CHECK(st == GEN_OK, "minor I-V-vi-IV");
    CHECK(ids[0] == GEN_NID_MINOR && ids[1] == GEN_NID_MINOR && ids[2] == GEN_NID_MAJOR &&
              ids[3] == GEN_NID_MINOR,
          "natural-minor qualities");

    /* Every preset on major triads, including the diminished vii when it is used. */
    for (i = 0; i < GEN_NPROG_COUNT; i++) {
        fill_nchord(&c);
        c.prog = (gen_native_prog)i;
        c.steps = 32;
        c.interval = 8;
        gen_buf_init(&b, ev, 64);
        st = gen_native_progression(&c, &b, ids, 64);
        CHECK(st == GEN_OK && b.count == 4, "preset generates");
        CHECK(ids[0] <= GEN_NID_MAX, "preset id in range");
    }

    /* ii–V–I sevenths: min7, dom7, maj7. */
    fill_nchord(&c);
    c.prog = GEN_NPROG_ii_V_I;
    c.mode = GEN_NMODE_SEVENTH;
    c.steps = 12;
    c.interval = 4;
    gen_buf_init(&b, ev, 64);
    st = gen_native_progression(&c, &b, ids, 64);
    CHECK(st == GEN_OK && b.count == 3, "ii-V-I seventh count");
    CHECK(ids[0] == GEN_NID_MIN7 && ids[1] == GEN_NID_DOM7 && ids[2] == GEN_NID_MAJ7,
          "ii-V-I seventh ids");

    /* vii in major is half-diminished. There is no stock id, so the phrase is rejected. */
    fill_nchord(&c);
    c.prog = GEN_NPROG_i_VI_III_VII;
    c.mode = GEN_NMODE_SEVENTH;
    gen_buf_init(&b, ev, 64);
    b.count = 9;
    st = gen_native_progression(&c, &b, ids, 64);
    CHECK(st == GEN_ERR_UNSUPPORTED && b.count == 0, "half-diminished rejected");

    /* The same degrees in natural minor are representable, and the last is dominant. */
    c.scale = GEN_SCALE_NAT_MINOR;
    gen_buf_init(&b, ev, 64);
    st = gen_native_progression(&c, &b, ids, 64);
    CHECK(st == GEN_OK && ids[3] == GEN_NID_DOM7, "natural-minor VII is dominant");

    /* Harmonic minor III is augmented. i is minor-major 7. */
    fill_nchord(&c);
    c.scale = GEN_SCALE_HAR_MINOR;
    c.prog = GEN_NPROG_i_VI_III_VII;
    gen_buf_init(&b, ev, 64);
    st = gen_native_progression(&c, &b, ids, 64);
    CHECK(st == GEN_OK && ids[2] == GEN_NID_AUG, "harmonic III is aug");
    c.mode = GEN_NMODE_SEVENTH;
    c.prog = GEN_NPROG_i_iv_v;
    c.steps = 12;
    gen_buf_init(&b, ev, 64);
    st = gen_native_progression(&c, &b, ids, 64);
    CHECK(st == GEN_OK && ids[0] == GEN_NID_MINMAJ7 && ids[2] == GEN_NID_DOM7,
          "harmonic i is minmaj7");

    /* A pentatonic scale cannot spell degree 5. Do not substitute. */
    fill_nchord(&c);
    c.scale = GEN_SCALE_MAJOR_PENT;
    gen_buf_init(&b, ev, 64);
    st = gen_native_progression(&c, &b, ids, 64);
    CHECK(st == GEN_ERR_UNSUPPORTED && b.count == 0, "pentatonic degree rejected");

    /* Fixed quality and an id the setter would clamp. */
    fill_nchord(&c);
    c.mode = GEN_NMODE_FIXED;
    c.fixed_id = GEN_NID_SUS4;
    gen_buf_init(&b, ev, 64);
    st = gen_native_progression(&c, &b, ids, 64);
    CHECK(st == GEN_OK && ids[0] == GEN_NID_SUS4 && ids[3] == GEN_NID_SUS4, "fixed sus4");
    c.fixed_id = 21;
    gen_buf_init(&b, ev, 64);
    b.count = 4;
    st = gen_native_progression(&c, &b, ids, 64);
    CHECK(st == GEN_ERR_PARAM && b.count == 0, "id 21 rejected");
    CHECK(!gen_native_id_ok(21) && gen_native_id_ok(0) && gen_native_id_ok(20), "id bounds");

    /* Seeded set and identical seeds. A different seed is allowed to differ. */
    fill_nchord(&c);
    c.mode = GEN_NMODE_SEEDED;
    c.prog = GEN_NPROG_RANDOM;
    c.seed = 42;
    gen_buf_init(&b, ev, 64);
    st = gen_native_progression(&c, &b, ids, 64);
    gen_buf_init(&b2, ev2, 64);
    CHECK(gen_native_progression(&c, &b2, ids2, 64) == GEN_OK, "seed repeat status");
    CHECK(b.count == b2.count && memcmp(ids, ids2, b.count) == 0 &&
              memcmp(ev, ev2, b.count * sizeof ev[0]) == 0,
          "identical seed");
    c.seed = 99;
    gen_buf_init(&b2, ev2, 64);
    CHECK(gen_native_progression(&c, &b2, ids2, 64) == GEN_OK, "changed seed status");
    CHECK(b.count != b2.count || memcmp(ids, ids2, b.count) != 0 || ev[0].note != ev2[0].note,
          "changed seed differs");

    memset(seen, 0, sizeof seen);
    for (seed = 1; seed <= 100; seed++) {
        static const uint8_t allow[] = {
            GEN_NID_MAJOR, GEN_NID_MINOR, GEN_NID_SUS2, GEN_NID_SUS4,
            GEN_NID_MAJ7, GEN_NID_MIN7, GEN_NID_DOM7
        };
        int ok, k;
        c.seed = (uint32_t)seed;
        gen_buf_init(&b, ev, 64);
        st = gen_native_progression(&c, &b, ids, 64);
        CHECK(st == GEN_OK && b.count == 4, "seeded 100");
        for (i = 0; i < (int)b.count; i++) {
            ok = 0;
            for (k = 0; k < (int)sizeof allow; k++)
                if (ids[i] == allow[k])
                    ok = 1;
            CHECK(ok, "seeded id is in the set");
            if (ev[i].note < 128)
                seen[ev[i].note] = 1;
        }
    }
    for (i = 0; i < 128; i++)
        uniq += seen[i];
    CHECK(uniq > 1, "100 seeds are not one pitch");

    /* Random diatonic triads stay inside major, minor, and diminished. */
    memset(seen, 0, sizeof seen);
    fill_nchord(&c);
    c.prog = GEN_NPROG_RANDOM;
    c.mode = GEN_NMODE_TRIAD;
    for (seed = 1; seed <= 100; seed++) {
        c.seed = (uint32_t)seed;
        gen_buf_init(&b, ev, 64);
        st = gen_native_progression(&c, &b, ids, 64);
        CHECK(st == GEN_OK, "random triad");
        for (i = 0; i < (int)b.count; i++) {
            CHECK(ids[i] == GEN_NID_MAJOR || ids[i] == GEN_NID_MINOR || ids[i] == GEN_NID_DIM,
                  "diatonic triad id");
            seen[ids[i]] = 1;
        }
    }
    CHECK(seen[GEN_NID_MAJOR] && seen[GEN_NID_MINOR] && seen[GEN_NID_DIM],
          "random triads use all three qualities");

    /* Phrase lengths and intervals, including a repeated cycle and a short last span. */
    fill_nchord(&c);
    c.steps = 16;
    c.interval = 16;
    gen_buf_init(&b, ev, 64);
    CHECK(gen_native_progression(&c, &b, ids, 64) == GEN_OK && b.count == 1, "len 16");
    c.steps = 32;
    c.interval = 16;
    gen_buf_init(&b, ev, 64);
    CHECK(gen_native_progression(&c, &b, ids, 64) == GEN_OK && b.count == 2, "len 32");
    c.steps = 64;
    c.interval = 16;
    gen_buf_init(&b, ev, 64);
    CHECK(gen_native_progression(&c, &b, ids, 64) == GEN_OK && b.count == 4, "len 64");
    c.interval = 8;
    gen_buf_init(&b, ev, 64);
    CHECK(gen_native_progression(&c, &b, ids, 64) == GEN_OK && b.count == 8, "repeated cycle count");
    CHECK(ids[0] == ids[4] && ev[0].note == ev[4].note, "progression repeats");
    c.steps = 24;
    c.interval = 16;
    gen_buf_init(&b, ev, 64);
    CHECK(gen_native_progression(&c, &b, ids, 64) == GEN_OK && b.count == 2, "partial last span");
    CHECK(ev[1].duration == 8 * 480u, "last span is shorter");
    c.interval = 32;
    c.steps = 16;
    gen_buf_init(&b, ev, 64);
    CHECK(gen_native_progression(&c, &b, ids, 64) == GEN_ERR_PARAM, "interval longer than phrase");

    /* Step retrigger uses one event per step and stays inside a step. */
    fill_nchord(&c);
    c.hold = GEN_NHOLD_RETRIG;
    c.gate_ticks = 240;
    c.steps = 8;
    c.interval = 4;
    gen_buf_init(&b, ev, 64);
    CHECK(gen_native_progression(&c, &b, ids, 64) == GEN_OK && b.count == 8, "retrigger count");
    CHECK(ev[0].duration == 240 && ev[1].tick == 480 && ids[0] == ids[1], "retrigger timing");

    /* Capacity is an error, not a truncated phrase. */
    fill_nchord(&c);
    gen_buf_init(&b, ev, 1);
    b.count = 3;
    CHECK(gen_native_progression(&c, &b, ids, 64) == GEN_ERR_CAPACITY && b.count == 0,
          "event capacity");
    gen_buf_init(&b, ev, 64);
    CHECK(gen_native_progression(&c, &b, ids, 1) == GEN_ERR_CAPACITY && b.count == 0, "id capacity");

    /* Pitch that would leave MIDI, and a duration past the native maximum. */
    fill_nchord(&c);
    c.root = 127;
    c.prog = GEN_NPROG_ii_V_I;
    gen_buf_init(&b, ev, 64);
    b.count = 2;
    CHECK(gen_native_progression(&c, &b, ids, 64) == GEN_ERR_PITCH && b.count == 0, "pitch ceiling");
    fill_nchord(&c);
    c.steps = 32;
    c.interval = 32;
    c.gate_ticks = 480;
    c.ticks_per_step = 60;
    gen_buf_init(&b, ev, 64);
    CHECK(gen_native_progression(&c, &b, ids, 64) == GEN_ERR_TIMING && b.count == 0,
          "native duration ceiling");
    c.ticks_per_step = 480;
    gen_buf_init(&b, ev, 64);
    CHECK(gen_native_progression(&c, &b, ids, 64) == GEN_OK, "duration at the native limit fits");
    CHECK(ev[0].duration == 32 * 480u, "long hold duration");
}

int main(void)
{
    test_rng_kat();
    test_scales();
    test_euclidean();
    test_melody_det();
    test_chords_arp();
    test_capacity_and_errors();
    test_long_lengths();
    test_transforms();
    test_smf();
    test_poly_preserve();
    property_fuzz();
    test_native_progression();
    if (g_fail) {
        fprintf(stderr, "%d checks failed\n", g_fail);
        return 1;
    }
    printf("All Genesis tests passed\n");
    return 0;
}
