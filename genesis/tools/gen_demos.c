/* Generate Genesis v0.1 demonstration MIDI files + sidecar config text. */
#include "genesis.h"
#include "genesis_smf.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static void dump_events(FILE *fp, const gen_buf *b)
{
    uint32_t i;
    for (i = 0; i < b->count; i++) {
        const gen_event *e = &b->events[i];
        fprintf(fp, "  tick=%u note=%u vel=%u dur=%u ch=%u\n",
                e->tick, e->note, e->velocity, e->duration, e->channel);
    }
}

static void write_meta(const char *path, const char *name, uint32_t seed, int root,
                       const char *scale, int tempo, int len, const char *settings,
                       uint32_t events, const gen_buf *b)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
        return;
    fprintf(fp, "generator: %s\nseed: %u\nroot: %d\nscale: %s\ntempo_bpm: %d\n"
                "pattern_length: %d\nsettings: %s\nevent_count: %u\nevents:\n",
            name, seed, root, scale, tempo, len, settings, events);
    dump_events(fp, b);
    fclose(fp);
}

static uint32_t bpm_to_us(int bpm)
{
    return (uint32_t)(60000000u / (uint32_t)bpm);
}

int main(int argc, char **argv)
{
    const char *outdir = argc > 1 ? argv[1] : "genesis/examples";
    gen_event storage[512];
    gen_buf b;
    gen_smf_opts opt;
    char mid[512], txt[512];
    int n = 0;

    mkdir(outdir, 0755);
    gen_buf_init(&b, storage, 512);
    opt.tpq = 480;
    opt.tempo_us = bpm_to_us(120);

#define OUT(name) do { \
    snprintf(mid, sizeof mid, "%s/%02d_%s.mid", outdir, ++n, name); \
    snprintf(txt, sizeof txt, "%s/%02d_%s.txt", outdir, n, name); \
    gen_smf_write_path(mid, &b, &opt); \
} while (0)

    /* 1. C minor random melody */
    {
        gen_rhythm r;
        gen_melody_cfg c;
        memset(&c, 0, sizeof c);
        gen_rhythm_euclidean(&r, 16, 12, 0, 120);
        c.root = 60; c.scale = GEN_SCALE_NAT_MINOR;
        c.pitch_min = 48; c.pitch_max = 72;
        c.vel_min = 70; c.vel_max = 110; c.duration = 100; c.seed = 1001;
        gen_buf_clear(&b);
        gen_melody_uniform(&c, &r, &b);
        OUT("c_minor_random");
        write_meta(txt, "melody_uniform", 1001, 60, "natural_minor", 120, 16,
                   "euclidean E(12,16) tps=120", b.count, &b);
    }
    /* 2. D Dorian walk */
    {
        gen_rhythm r;
        gen_melody_cfg c;
        memset(&c, 0, sizeof c);
        gen_rhythm_euclidean(&r, 16, 10, 2, 120);
        c.root = 62; c.scale = GEN_SCALE_DORIAN;
        c.pitch_min = 50; c.pitch_max = 74;
        c.vel_min = 80; c.vel_max = 100; c.duration = 110;
        c.seed = 2002; c.max_interval = 2; c.repeat_prob = 15;
        gen_buf_clear(&b);
        gen_melody_walk(&c, &r, &b);
        OUT("d_dorian_walk");
        write_meta(txt, "melody_walk", 2002, 62, "dorian", 120, 16,
                   "max_interval=2 repeat=15%", b.count, &b);
    }
    /* 3. Euclidean E(5,16) as mid tom hits */
    {
        gen_rhythm r;
        gen_rhythm_euclidean(&r, 16, 5, 0, 120);
        gen_buf_clear(&b);
        gen_rhythm_to_events(&r, &b, 62, 100, 60, 9);
        OUT("euclidean_5_16");
        write_meta(txt, "rhythm_euclidean", 0, 62, "n/a", 120, 16, "E(5,16)", b.count, &b);
    }
    /* 4. Euclidean bassline */
    {
        gen_rhythm r;
        gen_melody_cfg c;
        memset(&c, 0, sizeof c);
        gen_rhythm_euclidean(&r, 16, 5, 0, 120);
        c.root = 36; c.scale = GEN_SCALE_NAT_MINOR;
        c.pitch_min = 36; c.pitch_max = 48;
        c.vel_min = 100; c.vel_max = 110; c.duration = 200; c.seed = 4004;
        gen_buf_clear(&b);
        gen_melody_uniform(&c, &r, &b);
        OUT("euclidean_bassline");
        write_meta(txt, "euclid+melody", 4004, 36, "natural_minor", 120, 16, "E(5,16) bass", b.count, &b);
    }
    /* 5–7 arpeggios */
    {
        gen_chord ch;
        gen_arp_cfg a;
        memset(&a, 0, sizeof a);
        gen_chord_build(&ch, 60, GEN_CHORD_MAJ7, 0, 0);
        a.rate = 120; a.gate = 100; a.octaves = 2; a.velocity = 100; a.seed = 1;
        a.mode = GEN_ARP_UP; a.length = 16;
        gen_buf_clear(&b);
        gen_arp_chord(&a, &ch, &b);
        OUT("arp_up");
        write_meta(txt, "arp_up", 1, 60, "Cmaj7", 120, 16, "octaves=2", b.count, &b);
        a.mode = GEN_ARP_UPDOWN; a.length = 16;
        gen_buf_clear(&b);
        gen_arp_chord(&a, &ch, &b);
        OUT("arp_updown");
        write_meta(txt, "arp_updown", 1, 60, "Cmaj7", 120, 16, "octaves=2", b.count, &b);
        a.mode = GEN_ARP_RANDOM; a.seed = 777; a.length = 16;
        gen_buf_clear(&b);
        gen_arp_chord(&a, &ch, &b);
        OUT("arp_random");
        write_meta(txt, "arp_random", 777, 60, "Cmaj7", 120, 16, "octaves=2", b.count, &b);
    }
    /* 8–9 progressions */
    {
        gen_progression p;
        gen_harmony_cfg h;
        memset(&h, 0, sizeof h);
        gen_prog_I_V_vi_IV(&p);
        h.root = 60; h.scale = GEN_SCALE_MAJOR; h.quality = GEN_CHORD_TRIAD;
        h.ticks_per_chord = 480; h.duration = 400; h.velocity = 90; h.voice_lead = 0;
        gen_buf_clear(&b);
        gen_harmony_progression(&h, &p, &b);
        OUT("chords_I_V_vi_IV");
        write_meta(txt, "harmony_prog", 0, 60, "major", 120, 4, "I-V-vi-IV", b.count, &b);
        h.voice_lead = 1;
        gen_buf_clear(&b);
        gen_harmony_progression(&h, &p, &b);
        OUT("chords_voice_led");
        write_meta(txt, "harmony_voice_led", 0, 60, "major", 120, 4, "I-V-vi-IV voice_lead", b.count, &b);
    }
    /* 10 mutated melody */
    {
        gen_rhythm r;
        gen_melody_cfg c;
        gen_rng rng;
        memset(&c, 0, sizeof c);
        gen_rhythm_euclidean(&r, 16, 12, 0, 120);
        c.root = 60; c.scale = GEN_SCALE_MAJOR;
        c.pitch_min = 55; c.pitch_max = 79;
        c.vel_min = 80; c.vel_max = 100; c.duration = 100; c.seed = 55;
        gen_buf_clear(&b);
        gen_melody_uniform(&c, &r, &b);
        gen_rng_seed(&rng, 99);
        gen_xform_mutate_pitch(&b, GEN_SCALE_MAJOR, 60, 55, 79, 40, &rng);
        OUT("mutated_melody");
        write_meta(txt, "mutate_pitch", 55, 60, "major", 120, 16, "mutate_prob=40 seed_rng=99", b.count, &b);
    }
    /* 11 weighted */
    {
        gen_rhythm r;
        gen_melody_cfg c;
        static const uint8_t w[] = {8, 1, 4, 1, 6, 2, 1}; /* emphasize 1,3,5 */
        memset(&c, 0, sizeof c);
        gen_rhythm_euclidean(&r, 16, 12, 0, 120);
        c.root = 60; c.scale = GEN_SCALE_MAJOR;
        c.pitch_min = 60; c.pitch_max = 84;
        c.vel_min = 70; c.vel_max = 110; c.duration = 100; c.seed = 321;
        c.degree_weights = w;
        gen_buf_clear(&b);
        gen_melody_weighted(&c, &r, &b);
        OUT("weighted_melody");
        write_meta(txt, "melody_weighted", 321, 60, "major", 120, 16, "weights=8,1,4,1,6,2,1", b.count, &b);
    }
    /* 12 combined probability rhythm + walk */
    {
        gen_rhythm r;
        gen_melody_cfg c;
        gen_rng rng;
        memset(&c, 0, sizeof c);
        gen_rng_seed(&rng, 12);
        gen_rhythm_probability(&r, 16, 120, 60, &rng);
        c.root = 57; c.scale = GEN_SCALE_MINOR_PENT;
        c.pitch_min = 45; c.pitch_max = 69;
        c.vel_min = 75; c.vel_max = 115; c.duration = 90;
        c.seed = 12; c.max_interval = 3; c.repeat_prob = 5;
        gen_buf_clear(&b);
        gen_melody_walk(&c, &r, &b);
        OUT("prob_rhythm_walk");
        write_meta(txt, "prob+walk", 12, 57, "minor_pent", 120, 16, "density=60%", b.count, &b);
    }

    printf("Wrote %d demonstration MIDI files under %s\n", n, outdir);
    return 0;
}
