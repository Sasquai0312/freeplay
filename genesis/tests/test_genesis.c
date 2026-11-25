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

int main(void)
{
    test_rng_kat();
    test_scales();
    test_euclidean();
    test_melody_det();
    test_chords_arp();
    test_capacity_and_errors();
    test_transforms();
    test_smf();
    test_poly_preserve();
    property_fuzz();
    if (g_fail) {
        fprintf(stderr, "%d checks failed\n", g_fail);
        return 1;
    }
    printf("All Genesis tests passed\n");
    return 0;
}
