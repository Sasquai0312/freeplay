/* Host-side expected events for the G5A musical fixtures.
 * Matches Menu > Genesis do_generate() field defaults unless overridden. */
#include "genesis.h"
#include <stdio.h>
#include <string.h>

static void dump(const char *name, const gen_buf *b)
{
    uint32_t i;
    printf("{\"name\":\"%s\",\"count\":%u,\"events\":[", name, b->count);
    for (i = 0; i < b->count; i++) {
        const gen_event *e = &b->events[i];
        if (i)
            printf(",");
        printf("{\"step\":%u,\"note\":%u,\"vel\":%u,\"dur\":%u}",
               e->tick / 480u, e->note, e->velocity, e->duration);
    }
    printf("]}\n");
}

static void melody(const char *name, int len, int alg, int scale, int root, uint32_t seed,
                   int pulses)
{
    gen_event storage[128];
    gen_buf b;
    gen_rhythm rh;
    gen_melody_cfg mc;
    gen_rng rng;
    memset(&mc, 0, sizeof mc);
    gen_buf_init(&b, storage, 128);
    if (pulses >= 0)
        gen_rhythm_euclidean(&rh, (uint16_t)len, (uint16_t)pulses, 0, 480);
    else {
        gen_rng_seed(&rng, seed);
        gen_rhythm_probability(&rh, (uint16_t)len, 480, 70, &rng);
    }
    mc.root = root;
    mc.scale = (gen_scale)scale;
    mc.pitch_min = 48;
    mc.pitch_max = 72;
    mc.vel_min = 64;
    mc.vel_max = 110;
    mc.duration = 480; /* gate 100% */
    mc.channel = 0;
    mc.seed = seed;
    mc.max_interval = 3;
    mc.repeat_prob = 20;
    if (alg)
        gen_melody_walk(&mc, &rh, &b);
    else
        gen_melody_uniform(&mc, &rh, &b);
    dump(name, &b);
}

static void arp(const char *name, int len, int root)
{
    gen_event storage[128];
    gen_buf b;
    gen_arp_cfg ac;
    uint8_t notes[3];
    memset(&ac, 0, sizeof ac);
    notes[0] = (uint8_t)root;
    notes[1] = (uint8_t)(root + 4);
    notes[2] = (uint8_t)(root + 7);
    gen_buf_init(&b, storage, 128);
    ac.mode = GEN_ARP_UP;
    ac.rate = 480;
    ac.gate = 480;
    ac.octaves = 1;
    ac.length = (uint16_t)len;
    ac.velocity = 64;
    ac.channel = 0;
    ac.seed = 42;
    gen_arp(&ac, notes, 3, &b);
    dump(name, &b);
}

int main(void)
{
    melody("mel32", 32, 0, GEN_SCALE_NAT_MINOR, 60, 42, -1);
    melody("mel32s43", 32, 0, GEN_SCALE_NAT_MINOR, 60, 43, -1);
    melody("euc32", 32, 0, GEN_SCALE_NAT_MINOR, 60, 42, 11);
    melody("walk64", 64, 1, GEN_SCALE_MINOR_PENT, 57, 2026, -1);
    arp("arp64", 64, 60);
    return 0;
}
