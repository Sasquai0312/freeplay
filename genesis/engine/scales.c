#include "genesis.h"

/* Interval patterns from root (semitones), terminated by 0xFF. */
static const uint8_t SC_CHROMATIC[] = {0,1,2,3,4,5,6,7,8,9,10,11,0xFF};
static const uint8_t SC_MAJOR[] = {0,2,4,5,7,9,11,0xFF};
static const uint8_t SC_NAT_MIN[] = {0,2,3,5,7,8,10,0xFF};
static const uint8_t SC_HAR_MIN[] = {0,2,3,5,7,8,11,0xFF};
static const uint8_t SC_MEL_MIN[] = {0,2,3,5,7,9,11,0xFF};
static const uint8_t SC_DORIAN[] = {0,2,3,5,7,9,10,0xFF};
static const uint8_t SC_PHRYG[] = {0,1,3,5,7,8,10,0xFF};
static const uint8_t SC_LYDIAN[] = {0,2,4,6,7,9,11,0xFF};
static const uint8_t SC_MIXO[] = {0,2,4,5,7,9,10,0xFF};
static const uint8_t SC_LOCR[] = {0,1,3,5,6,8,10,0xFF};
static const uint8_t SC_MAJ_PENT[] = {0,2,4,7,9,0xFF};
static const uint8_t SC_MIN_PENT[] = {0,3,5,7,10,0xFF};
static const uint8_t SC_BLUES[] = {0,3,5,6,7,10,0xFF};
static const uint8_t SC_WHOLE[] = {0,2,4,6,8,10,0xFF};

static const uint8_t *const SCALES[GEN_SCALE_COUNT] = {
    SC_CHROMATIC, SC_MAJOR, SC_NAT_MIN, SC_HAR_MIN, SC_MEL_MIN,
    SC_DORIAN, SC_PHRYG, SC_LYDIAN, SC_MIXO, SC_LOCR,
    SC_MAJ_PENT, SC_MIN_PENT, SC_BLUES, SC_WHOLE
};

int gen_clamp_midi(int n)
{
    if (n < 0)
        return 0;
    if (n > 127)
        return 127;
    return n;
}

int gen_scale_degree_count(gen_scale s)
{
    const uint8_t *p;
    int n = 0;
    if ((unsigned)s >= GEN_SCALE_COUNT)
        return 0;
    p = SCALES[s];
    while (p[n] != 0xFF)
        n++;
    return n;
}

static int pc_in_scale(gen_scale s, int pc)
{
    const uint8_t *p;
    int i;
    if ((unsigned)s >= GEN_SCALE_COUNT)
        return 0;
    pc %= 12;
    if (pc < 0)
        pc += 12;
    p = SCALES[s];
    for (i = 0; p[i] != 0xFF; i++)
        if (p[i] == (uint8_t)pc)
            return 1;
    return 0;
}

int gen_scale_contains(gen_scale s, int root, int note)
{
    int pc;
    if (note < 0 || note > 127)
        return 0;
    root %= 12;
    if (root < 0)
        root += 12;
    pc = (note - root) % 12;
    if (pc < 0)
        pc += 12;
    return pc_in_scale(s, pc);
}

int gen_scale_degree_to_midi(gen_scale s, int root, int degree, int octave)
{
    const uint8_t *p;
    int n, d, octs, midi;
    if ((unsigned)s >= GEN_SCALE_COUNT)
        return -1;
    p = SCALES[s];
    n = gen_scale_degree_count(s);
    if (n <= 0)
        return -1;
    if (degree < 0) {
        octs = -((-degree + n - 1) / n);
        d = degree - octs * n;
    } else {
        octs = degree / n;
        d = degree % n;
    }
    midi = root + (int)p[d] + (octave + octs) * 12;
    return gen_clamp_midi(midi);
}

int gen_scale_quantize(gen_scale s, int root, int note, int lo, int hi)
{
    int best = -1, best_dist = 999, n;
    if (lo < 0)
        lo = 0;
    if (hi > 127)
        hi = 127;
    if (lo > hi)
        return gen_clamp_midi(note);
    note = gen_clamp_midi(note);
    for (n = lo; n <= hi; n++) {
        if (!gen_scale_contains(s, root, n))
            continue;
        {
            int d = n - note;
            if (d < 0)
                d = -d;
            /* ties: prefer lower pitch */
            if (d < best_dist || (d == best_dist && (best < 0 || n < best))) {
                best_dist = d;
                best = n;
            }
        }
    }
    return best >= 0 ? best : gen_clamp_midi(note);
}
