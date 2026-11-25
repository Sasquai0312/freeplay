#include "genesis.h"

/* Build expanded pitch list: notes across octaves (unique per octave copy).
 * Duplicate input notes are kept once per unique pitch class in as-played order
 * for AS_PLAYED; for other modes we sort ascending first. */
static int expand_notes(const uint8_t *in, uint8_t n, uint8_t octaves,
                        uint8_t *out, int max_out, int sort_asc)
{
    uint8_t uniq[128];
    uint8_t nu = 0;
    int i, o, count = 0;
    if (!in || n == 0)
        return 0;
    for (i = 0; i < n; i++) {
        uint8_t k, found = 0;
        if (in[i] > 127)
            continue;
        for (k = 0; k < nu; k++)
            if (uniq[k] == in[i]) {
                found = 1;
                break;
            }
        if (!found && nu < 128)
            uniq[nu++] = in[i];
    }
    if (sort_asc) {
        /* insertion sort */
        for (i = 1; i < (int)nu; i++) {
            uint8_t key = uniq[i];
            int j = i;
            while (j > 0 && uniq[j - 1] > key) {
                uniq[j] = uniq[j - 1];
                j--;
            }
            uniq[j] = key;
        }
    }
    if (octaves < 1)
        octaves = 1;
    if (octaves > 4)
        octaves = 4;
    for (o = 0; o < (int)octaves; o++) {
        for (i = 0; i < (int)nu && count < max_out; i++) {
            int midi = (int)uniq[i] + o * 12;
            if (midi > 127)
                continue;
            out[count++] = (uint8_t)midi;
        }
    }
    return count;
}

static void reverse_u8(uint8_t *a, int n)
{
    int i;
    for (i = 0; i < n / 2; i++) {
        uint8_t t = a[i];
        a[i] = a[n - 1 - i];
        a[n - 1 - i] = t;
    }
}

/* Build one cycle sequence into seq[]; returns length.
 * UPDOWN: up then down without repeating the peak.
 * DOWNUP: down then up without repeating the trough.
 * CONVERGE: outside-in; DIVERGE: inside-out. */
static int build_cycle(gen_arp_mode mode, uint8_t *pitches, int np, uint8_t *seq, int max_seq, gen_rng *rng)
{
    int i, n = 0;
    if (np <= 0)
        return 0;
    switch (mode) {
    case GEN_ARP_UP:
    case GEN_ARP_AS_PLAYED:
        for (i = 0; i < np && n < max_seq; i++)
            seq[n++] = pitches[i];
        break;
    case GEN_ARP_DOWN:
        for (i = np - 1; i >= 0 && n < max_seq; i--)
            seq[n++] = pitches[i];
        break;
    case GEN_ARP_UPDOWN:
        for (i = 0; i < np && n < max_seq; i++)
            seq[n++] = pitches[i];
        for (i = np - 2; i >= 1 && n < max_seq; i--)
            seq[n++] = pitches[i];
        break;
    case GEN_ARP_DOWNUP:
        for (i = np - 1; i >= 0 && n < max_seq; i--)
            seq[n++] = pitches[i];
        for (i = 1; i < np - 1 && n < max_seq; i++)
            seq[n++] = pitches[i];
        break;
    case GEN_ARP_RANDOM:
        for (i = 0; i < np && n < max_seq; i++)
            seq[n++] = pitches[i];
        /* Fisher-Yates */
        for (i = n - 1; i > 0; i--) {
            uint32_t j = gen_rng_bound(rng, (uint32_t)(i + 1));
            uint8_t t = seq[i];
            seq[i] = seq[j];
            seq[j] = t;
        }
        break;
    case GEN_ARP_CONVERGE: {
        int lo = 0, hi = np - 1, turn = 0;
        while (lo <= hi && n < max_seq) {
            if (!turn)
                seq[n++] = pitches[lo++];
            else
                seq[n++] = pitches[hi--];
            turn = !turn;
        }
        break;
    }
    case GEN_ARP_DIVERGE: {
        int mid = np / 2, lo = mid - 1, hi = mid, turn = 0;
        if (np % 2) {
            seq[n++] = pitches[mid];
            hi = mid + 1;
            lo = mid - 1;
        }
        while ((lo >= 0 || hi < np) && n < max_seq) {
            if (!turn && hi < np)
                seq[n++] = pitches[hi++];
            else if (lo >= 0)
                seq[n++] = pitches[lo--];
            else if (hi < np)
                seq[n++] = pitches[hi++];
            turn = !turn;
        }
        break;
    }
    default:
        return 0;
    }
    (void)reverse_u8;
    return n;
}

gen_status gen_arp(const gen_arp_cfg *cfg, const uint8_t *notes, uint8_t n_notes, gen_buf *out)
{
    uint8_t pitches[64], seq[128];
    int np, cycle, total, i;
    gen_rng rng;
    uint32_t saved;
    gen_status st;
    int sort_asc;

    if (!cfg || !out)
        return GEN_ERR_PARAM;
    if (!notes || n_notes == 0)
        return GEN_ERR_EMPTY;
    if (cfg->rate == 0 || cfg->gate == 0 || cfg->velocity < 1 || cfg->velocity > 127 || cfg->channel > 15)
        return GEN_ERR_PARAM;

    sort_asc = (cfg->mode != GEN_ARP_AS_PLAYED);
    np = expand_notes(notes, n_notes, cfg->octaves ? cfg->octaves : 1, pitches, 64, sort_asc);
    if (np == 0)
        return GEN_ERR_EMPTY;

    gen_rng_seed(&rng, cfg->seed);
    cycle = build_cycle(cfg->mode, pitches, np, seq, 128, &rng);
    if (cycle <= 0)
        return GEN_ERR_UNSUPPORTED;

    total = cfg->length ? (int)cfg->length : cycle;
    saved = out->count;
    for (i = 0; i < total; i++) {
        gen_event ev;
        ev.tick = (uint32_t)i * cfg->rate;
        ev.note = seq[i % cycle];
        ev.velocity = cfg->velocity;
        ev.channel = cfg->channel;
        ev._pad = 0;
        ev.duration = cfg->gate <= cfg->rate ? cfg->gate : cfg->rate;
        st = gen_buf_push(out, &ev);
        if (st != GEN_OK) {
            out->count = saved;
            return st;
        }
    }
    return GEN_OK;
}

gen_status gen_arp_chord(const gen_arp_cfg *cfg, const gen_chord *chord, gen_buf *out)
{
    if (!chord)
        return GEN_ERR_EMPTY;
    return gen_arp(cfg, chord->notes, chord->count, out);
}
