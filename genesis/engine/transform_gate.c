/* Gate scaling uses a 64-bit multiply. It lives in its own object so a
 * firmware link that does not call it does not pull a 64-bit divider. */
#include "genesis.h"

gen_status gen_xform_gate(gen_buf *b, int pct_min, int pct_max, gen_rng *rng)
{
    uint32_t i;
    if (!b || !rng || pct_min < 1 || pct_max > 200 || pct_min > pct_max)
        return GEN_ERR_PARAM;
    for (i = 0; i < b->count; i++) {
        int pct = (int)gen_rng_range(rng, pct_min, pct_max);
        uint64_t d = (uint64_t)b->events[i].duration * (uint64_t)pct / 100u;
        if (d == 0)
            d = 1;
        if (d > UINT32_MAX)
            return GEN_ERR_OVERFLOW;
        b->events[i].duration = (uint32_t)d;
    }
    return GEN_OK;
}
