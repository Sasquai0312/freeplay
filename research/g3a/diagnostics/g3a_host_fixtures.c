/* Host-side direct G2 fixture generation — mirrors cfw/src/genesis_adapt.c gen_fixture.
 * usage: g3a_host_fixtures <id>  → JSON events on stdout
 */
#include "genesis.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define GEN_TPS 480u
#define CAP 64

static int run(int id, gen_event *storage, uint32_t *count, uint32_t *seed_out)
{
    gen_buf out;
    gen_rhythm rh;
    gen_melody_cfg mc;
    gen_arp_cfg ac;
    gen_status st;
    uint8_t notes[3];

    gen_buf_init(&out, storage, CAP);
    memset(&mc, 0, sizeof mc);
    memset(&ac, 0, sizeof ac);

    switch (id) {
    case 1: {
        gen_rng rng;
        *seed_out = 42;
        gen_rng_seed(&rng, 42);
        st = gen_rhythm_probability(&rh, 16, (uint16_t)GEN_TPS, 70, &rng);
        if (st != GEN_OK)
            return -1;
        mc.root = 60;
        mc.scale = GEN_SCALE_NAT_MINOR;
        mc.pitch_min = 48;
        mc.pitch_max = 72;
        mc.vel_min = 64;
        mc.vel_max = 110;
        mc.duration = GEN_TPS;
        mc.channel = 0;
        mc.seed = 42;
        st = gen_melody_uniform(&mc, &rh, &out);
        break;
    }
    case 2:
        *seed_out = 123;
        st = gen_rhythm_euclidean(&rh, 16, 5, 0, (uint16_t)GEN_TPS);
        if (st != GEN_OK)
            return -1;
        mc.root = 62;
        mc.scale = GEN_SCALE_DORIAN;
        mc.pitch_min = 36;
        mc.pitch_max = 55;
        mc.vel_min = 80;
        mc.vel_max = 100;
        mc.duration = GEN_TPS;
        mc.channel = 0;
        mc.seed = 123;
        st = gen_melody_uniform(&mc, &rh, &out);
        break;
    case 3: {
        gen_rng rng;
        *seed_out = 2026;
        gen_rng_seed(&rng, 2026);
        st = gen_rhythm_probability(&rh, 32, (uint16_t)GEN_TPS, 60, &rng);
        if (st != GEN_OK)
            return -1;
        mc.root = 57;
        mc.scale = GEN_SCALE_MINOR_PENT;
        mc.pitch_min = 45;
        mc.pitch_max = 81;
        mc.vel_min = 70;
        mc.vel_max = 115;
        mc.duration = GEN_TPS / 2;
        mc.channel = 0;
        mc.seed = 2026;
        mc.max_interval = 3;
        mc.repeat_prob = 20;
        st = gen_melody_walk(&mc, &rh, &out);
        break;
    }
    case 4:
        *seed_out = 0;
        notes[0] = 60;
        notes[1] = 64;
        notes[2] = 67;
        ac.mode = GEN_ARP_UP;
        ac.rate = GEN_TPS;
        ac.gate = GEN_TPS;
        ac.octaves = 1;
        ac.length = 16;
        ac.velocity = 96;
        ac.channel = 0;
        ac.seed = 0;
        st = gen_arp(&ac, notes, 3, &out);
        break;
    default:
        return -2;
    }
    if (st != GEN_OK || gen_buf_validate(&out, 1) != GEN_OK)
        return -1;
    *count = out.count;
    return 0;
}

int main(int argc, char **argv)
{
    gen_event ev[CAP];
    uint32_t n = 0, seed = 0, i;
    int id = argc > 1 ? atoi(argv[1]) : 1;
    if (run(id, ev, &n, &seed) != 0) {
        fprintf(stderr, "fixture %d failed\n", id);
        return 1;
    }
    printf("{\"fixture\":%d,\"seed\":%u,\"count\":%u,\"events\":[", id, seed, n);
    for (i = 0; i < n; i++) {
        if (i)
            printf(",");
        printf("{\"tick\":%u,\"note\":%u,\"vel\":%u,\"dur\":%u,\"ch\":%u}",
               ev[i].tick, ev[i].note, ev[i].velocity, ev[i].duration, ev[i].channel);
    }
    printf("]}\n");
    return 0;
}
