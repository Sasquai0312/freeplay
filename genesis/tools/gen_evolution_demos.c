/* G5B musical demonstrations. Host only. Writes SMF plus a parameter manifest. */
#include "genesis.h"
#include "genesis_smf.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static uint32_t file_fnv(const char *path)
{
    FILE *fp = fopen(path, "rb");
    uint8_t buf[256];
    uint32_t h = 2166136261u;
    size_t n;
    if (!fp)
        return 0;
    while ((n = fread(buf, 1, sizeof buf, fp)) > 0) {
        size_t i;
        for (i = 0; i < n; i++) {
            h ^= buf[i];
            h *= 16777619u;
        }
    }
    fclose(fp);
    return h;
}

static void write_manifest(const char *path, const char *generator, uint32_t seed, int root,
                           const char *scale, int steps, const char *settings, const gen_buf *b,
                           uint32_t tps, int native_ok, uint32_t checksum)
{
    FILE *fp = fopen(path, "w");
    uint32_t i;
    if (!fp)
        return;
    fprintf(fp,
            "generator: %s\nseed: %u\nroot: %d\nscale: %s\nlength_steps: %d\n"
            "ticks_per_step: %u\ntempo_bpm: 120\nsettings: %s\nevent_count: %u\n"
            "timing: step grid, gate within one step\nnative_compatible: %s\n"
            "smf_fnv1a: %08x\nevents:\n",
            generator, seed, root, scale, steps, tps, settings, b->count,
            native_ok ? "yes" : "no", checksum);
    for (i = 0; i < b->count; i++) {
        const gen_event *e = &b->events[i];
        fprintf(fp, "  tick=%u note=%u vel=%u dur=%u ch=%u\n",
                e->tick, e->note, e->velocity, e->duration, e->channel);
    }
    fclose(fp);
}

static int emit(const char *dir, int index, const char *name, const gen_buf *b,
                const char *generator, uint32_t seed, int root, const char *scale,
                int steps, const char *settings, uint32_t tps)
{
    char mid[512], txt[512];
    gen_smf_opts opt;
    uint32_t sum;
    int native_ok = 1;
    uint32_t i;
    opt.tpq = 480;
    opt.tempo_us = 500000;
    snprintf(mid, sizeof mid, "%s/%02d_%s.mid", dir, index, name);
    snprintf(txt, sizeof txt, "%s/%02d_%s.txt", dir, index, name);
    if (gen_smf_write_path(mid, b, &opt) != 0)
        return -1;
    for (i = 0; i < b->count; i++) {
        const gen_event *e = &b->events[i];
        if (e->tick % tps || e->tick / tps >= (uint32_t)steps ||
            !gen_native_duration_ok(e->duration, tps) || steps > 64)
            native_ok = 0;
    }
    if (gen_buf_validate(b, 1) != GEN_OK)
        native_ok = 0;
    sum = file_fnv(mid);
    write_manifest(txt, generator, seed, root, scale, steps, settings, b, tps, native_ok, sum);
    printf("%s events=%u native=%s fnv=%08x\n", mid, b->count, native_ok ? "yes" : "no", sum);
    return 0;
}

static gen_turing_cfg tc(uint32_t seed, int root, uint8_t scale, int lo, int hi,
                         uint8_t length, uint16_t steps, uint8_t evolve, uint8_t rest, uint8_t lock)
{
    gen_turing_cfg c;
    memset(&c, 0, sizeof c);
    c.gate = 360;
    c.ticks_per_step = 480;
    c.seed = seed;
    c.root = (int16_t)root;
    c.pitch_min = (int16_t)lo;
    c.pitch_max = (int16_t)hi;
    c.steps = steps;
    c.scale = scale;
    c.length = length;
    c.evolve_prob = evolve;
    c.rest_prob = rest;
    c.velocity = 100;
    c.locked = lock;
    return c;
}

static int turing_demo(const char *dir, int index, const char *name, gen_turing_cfg c,
                       const char *scale, const char *settings, gen_event *storage)
{
    gen_turing st, next;
    gen_buf b;
    gen_buf_init(&b, storage, 128);
    if (gen_turing_init(&st, &c) != GEN_OK)
        return -1;
    if (gen_turing_generate(&st, NULL, &b, &next) != GEN_OK)
        return -1;
    return emit(dir, index, name, &b, "turing", c.seed, c.root, scale, c.steps, settings, c.ticks_per_step);
}

static gen_markov_cfg mc(uint32_t seed, int root, uint8_t scale, int lo, int hi, uint16_t steps, uint8_t rest)
{
    gen_markov_cfg c;
    memset(&c, 0, sizeof c);
    c.gate = 360;
    c.ticks_per_step = 480;
    c.seed = seed;
    c.root = (int16_t)root;
    c.pitch_min = (int16_t)lo;
    c.pitch_max = (int16_t)hi;
    c.initial_degree = 0;
    c.steps = steps;
    c.scale = scale;
    c.velocity = 96;
    c.rest_prob = rest;
    c.octave_motion = 1;
    return c;
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : "genesis/examples/evolution";
    gen_event storage[128];
    gen_buf b;
    int n = 0;

    mkdir(dir, 0755);
    gen_buf_init(&b, storage, 128);

    if (turing_demo(dir, ++n, "turing_locked_8",
                    tc(11, 60, GEN_SCALE_NAT_MINOR, 48, 72, 8, 8, 0, 25, 1),
                    "natural_minor",
                    "register 8, evolve 0, rest 25, lock 1, gate 360", storage))
        return 1;
    if (turing_demo(dir, ++n, "turing_slow_16",
                    tc(22, 62, GEN_SCALE_DORIAN, 50, 74, 16, 16, 12, 8, 0),
                    "dorian",
                    "register 16, evolve 12, rest 8, lock 0, gate 360", storage))
        return 1;
    if (turing_demo(dir, ++n, "turing_moderate_32",
                    tc(33, 57, GEN_SCALE_NAT_MINOR, 48, 76, 16, 32, 35, 10, 0),
                    "natural_minor",
                    "register 16, evolve 35, rest 10, lock 0, gate 360", storage))
        return 1;
    if (turing_demo(dir, ++n, "turing_evolve_64",
                    tc(44, 60, GEN_SCALE_DORIAN, 48, 81, 32, 64, 55, 12, 0),
                    "dorian",
                    "register 32, evolve 55, rest 12, lock 0, gate 360", storage))
        return 1;

    {
        gen_markov_model m;
        gen_markov_cfg c = mc(55, 57, GEN_SCALE_NAT_MINOR, 57, 81, 32, 8);
        gen_markov_preset(&m, GEN_SCALE_NAT_MINOR, GEN_MARKOV_STEPWISE);
        gen_buf_clear(&b);
        if (gen_markov_generate(&m, &c, &b) != GEN_OK)
            return 1;
        if (emit(dir, ++n, "markov_stepwise_minor", &b, "markov_stepwise", 55, 57, "natural_minor",
                 32, "style stepwise, rest 8, octave motion 1, initial degree 0, gate 360", 480))
            return 1;
    }
    {
        gen_markov_model m;
        gen_markov_cfg c = mc(66, 36, GEN_SCALE_NAT_MINOR, 36, 60, 16, 5);
        gen_markov_preset(&m, GEN_SCALE_NAT_MINOR, GEN_MARKOV_BASSLINE);
        gen_buf_clear(&b);
        if (gen_markov_generate(&m, &c, &b) != GEN_OK)
            return 1;
        if (emit(dir, ++n, "markov_bassline", &b, "markov_bassline", 66, 36, "natural_minor",
                 16, "style bassline, rest 5, range 36-60, gate 360", 480))
            return 1;
    }
    {
        gen_markov_model m;
        gen_markov_cfg c = mc(77, 62, GEN_SCALE_DORIAN, 50, 86, 32, 6);
        gen_markov_preset(&m, GEN_SCALE_DORIAN, GEN_MARKOV_EXPLORATORY);
        gen_buf_clear(&b);
        if (gen_markov_generate(&m, &c, &b) != GEN_OK)
            return 1;
        if (emit(dir, ++n, "markov_exploratory", &b, "markov_exploratory", 77, 62, "dorian",
                 32, "style exploratory, rest 6, octave motion 1, gate 360", 480))
            return 1;
    }
    {
        /* Documented source: C D E G E D C in C major. */
        gen_event phrase[8];
        gen_buf src;
        gen_markov_model m, scratch;
        gen_markov_cfg c = mc(88, 60, GEN_SCALE_MAJOR, 60, 84, 32, 0);
        uint8_t notes[] = {60, 62, 64, 67, 64, 62, 60};
        uint32_t i;
        memset(phrase, 0, sizeof phrase);
        for (i = 0; i < 7; i++) {
            phrase[i].tick = i * 480u;
            phrase[i].note = notes[i];
            phrase[i].velocity = 100;
            phrase[i].duration = 360;
        }
        src.events = phrase;
        src.count = 7;
        src.capacity = 8;
        if (gen_markov_learn(&m, GEN_SCALE_MAJOR, 60, &src, 0, &scratch) != GEN_OK)
            return 1;
        gen_buf_clear(&b);
        if (gen_markov_generate(&m, &c, &b) != GEN_OK)
            return 1;
        if (emit(dir, ++n, "markov_learned", &b, "markov_learned", 88, 60, "major",
                 32, "learned from C D E G E D C (60 62 64 67 64 62 60), gap 0, rest 0, gate 360", 480))
            return 1;
    }
    {
        gen_event src_ev[16], dst_ev[32];
        gen_buf src, out;
        gen_mutate_req r;
        uint8_t phrase[] = {60, 63, 65, 67, 70, 67, 65, 63, 60, 58, 60, 63, 65, 63, 62, 60};
        uint32_t i;
        const char *scale = "natural_minor";
        memset(&r, 0, sizeof r);
        r.ticks_per_step = 480;
        r.gate_min = 240;
        r.gate_max = 360;
        r.root = 60;
        r.pitch_min = 48;
        r.pitch_max = 84;
        r.dest_steps = 16;
        r.scale = GEN_SCALE_NAT_MINOR;
        r.intensity = 2;
        r.vel_min = 70;
        r.vel_max = 110;
        for (i = 0; i < 16; i++) {
            src_ev[i].tick = i * 480u;
            src_ev[i].note = phrase[i];
            src_ev[i].velocity = 100;
            src_ev[i].channel = 0;
            src_ev[i]._pad = 0;
            src_ev[i].duration = 360;
        }
        src.events = src_ev;
        src.count = 16;
        src.capacity = 16;
        gen_buf_init(&out, dst_ev, 16);
        r.seed = 100;
        if (gen_mutate(&src, &r, &out) != GEN_OK)
            return 1;
        if (emit(dir, ++n, "mutation_source", &out, "mutation_identity", 100, 60, scale,
                 16, "zero probability, C natural minor phrase, gate 360", 480))
            return 1;

        gen_buf_clear(&out);
        r.seed = 101;
        r.pitch_prob = 30;
        if (gen_mutate(&src, &r, &out) != GEN_OK)
            return 1;
        if (emit(dir, ++n, "mutation_pitch", &out, "mutation_pitch", 101, 60, scale,
                 16, "pitch probability 30, intensity 2 semitones, other probs 0", 480))
            return 1;

        gen_buf_init(&out, dst_ev, 32);
        r.seed = 102;
        r.pitch_prob = 0;
        r.displace_prob = 80;
        r.dest_steps = 32;
        r.protect = GEN_MUT_PROTECT_PITCH;
        if (gen_mutate(&src, &r, &out) != GEN_OK)
            return 1;
        if (emit(dir, ++n, "mutation_rhythm_protected_pitch", &out, "mutation_rhythm", 102, 60, scale,
                 32, "displace probability 80, intensity 2 steps, pitch protected, room through step 32", 480))
            return 1;

        gen_buf_clear(&out);
        r.seed = 103;
        r.protect = 0;
        r.pitch_prob = 40;
        r.displace_prob = 35;
        r.velocity_prob = 50;
        if (gen_mutate(&src, &r, &out) != GEN_OK)
            return 1;
        if (emit(dir, ++n, "mutation_combined", &out, "mutation_combined", 103, 60, scale,
                 32, "pitch 40, displace 35, velocity 50 into 70-110, intensity 2, dest 32", 480))
            return 1;
    }
    printf("wrote 12 evolution demos in %s\n", dir);
    return 0;
}
