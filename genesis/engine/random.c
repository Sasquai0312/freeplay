#include "genesis.h"

void gen_rng_seed(gen_rng *r, uint32_t seed)
{
    r->state = seed ? seed : 0xA5A5A5A5u;
}

uint32_t gen_rng_u32(gen_rng *r)
{
    uint32_t x = r->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    r->state = x ? x : 0xA5A5A5A5u;
    return r->state;
}

uint32_t gen_rng_bound(gen_rng *r, uint32_t bound)
{
    uint32_t x, lim;
    if (bound <= 1)
        return 0;
    /* rejection sampling — unbiased */
    lim = (UINT32_MAX / bound) * bound;
    do {
        x = gen_rng_u32(r);
    } while (x >= lim);
    return x % bound;
}

int32_t gen_rng_range(gen_rng *r, int32_t lo, int32_t hi)
{
    uint32_t span;
    if (hi < lo) {
        int32_t t = lo;
        lo = hi;
        hi = t;
    }
    /* hi - lo fits in signed 32 for MIDI-scale ranges; cast carefully */
    span = (uint32_t)(hi - lo) + 1u;
    return lo + (int32_t)gen_rng_bound(r, span);
}
