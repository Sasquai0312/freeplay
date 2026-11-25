/* Genesis portable generative MIDI engine — freestanding-friendly C.
 * No malloc, no global RNG, no firmware dependencies.
 * MIDI channels in events are 0..15 (MIDI wire channel = value + 1). */
#ifndef GENESIS_H
#define GENESIS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- status ---------------------------------------------------------------- */
typedef enum gen_status {
    GEN_OK = 0,
    GEN_ERR_PARAM = -1,
    GEN_ERR_PITCH = -2,
    GEN_ERR_TIMING = -3,
    GEN_ERR_CAPACITY = -4,
    GEN_ERR_UNSUPPORTED = -5,
    GEN_ERR_MONO = -6,
    GEN_ERR_EMPTY = -7,
    GEN_ERR_OVERFLOW = -8
} gen_status;

/* ---- event ----------------------------------------------------------------- */
typedef struct gen_event {
    uint32_t tick;     /* absolute start tick */
    uint8_t  note;     /* 0..127 */
    uint8_t  velocity; /* 1..127 for sounding notes; 0 reserved */
    uint8_t  channel;  /* 0..15 */
    uint8_t  _pad;
    uint32_t duration; /* ticks; must be > 0 for sounding notes */
} gen_event;

typedef struct gen_buf {
    gen_event *events;
    uint32_t   capacity;
    uint32_t   count;
} gen_buf;

void      gen_buf_init(gen_buf *b, gen_event *storage, uint32_t capacity);
void      gen_buf_clear(gen_buf *b);
gen_status gen_buf_push(gen_buf *b, const gen_event *ev);
gen_status gen_buf_validate(const gen_buf *b, int monophonic);
/* Sort by (tick, channel, note). Stable for equal keys by original order via index. */
gen_status gen_buf_sort(gen_buf *b);

/* ---- RNG (xorshift32; seed 0 remapped to 0xA5A5A5A5u) ----------------------- */
typedef struct gen_rng {
    uint32_t state;
} gen_rng;

void     gen_rng_seed(gen_rng *r, uint32_t seed);
uint32_t gen_rng_u32(gen_rng *r);
/* Uniform in [0, bound) — bound==0 returns 0. */
uint32_t gen_rng_bound(gen_rng *r, uint32_t bound);
/* Inclusive. */
int32_t  gen_rng_range(gen_rng *r, int32_t lo, int32_t hi);

/* ---- scales ---------------------------------------------------------------- */
typedef enum gen_scale {
    GEN_SCALE_CHROMATIC = 0,
    GEN_SCALE_MAJOR,
    GEN_SCALE_NAT_MINOR,
    GEN_SCALE_HAR_MINOR,
    GEN_SCALE_MEL_MINOR,
    GEN_SCALE_DORIAN,
    GEN_SCALE_PHRYGIAN,
    GEN_SCALE_LYDIAN,
    GEN_SCALE_MIXOLYDIAN,
    GEN_SCALE_LOCRIAN,
    GEN_SCALE_MAJOR_PENT,
    GEN_SCALE_MINOR_PENT,
    GEN_SCALE_BLUES,
    GEN_SCALE_WHOLE_TONE,
    GEN_SCALE_COUNT
} gen_scale;

int  gen_scale_degree_count(gen_scale s);
int  gen_scale_contains(gen_scale s, int root, int note);
/* Map MIDI note to nearest scale pitch in [lo,hi]. Ties: prefer lower pitch. */
int  gen_scale_quantize(gen_scale s, int root, int note, int lo, int hi);
/* Degree 0..n-1 at octave (C4=60 reference for root degree 0 when root=60). */
int  gen_scale_degree_to_midi(gen_scale s, int root, int degree, int octave);
int  gen_clamp_midi(int n);

/* ---- rhythm ---------------------------------------------------------------- */
#define GEN_RHYTHM_MAX_STEPS 128

typedef struct gen_rhythm {
    uint8_t steps[GEN_RHYTHM_MAX_STEPS]; /* 1 = hit, 0 = rest */
    uint16_t step_count;
    uint16_t ticks_per_step;
} gen_rhythm;

gen_status gen_rhythm_fixed(gen_rhythm *out, const uint8_t *pattern, uint16_t n, uint16_t tps);
gen_status gen_rhythm_probability(gen_rhythm *out, uint16_t n, uint16_t tps, uint8_t density /*0..100*/, gen_rng *rng);
/* Euclidean: Bjorklund; rotate_by moves pattern right by k (doc: first pulse at index rotate). */
gen_status gen_rhythm_euclidean(gen_rhythm *out, uint16_t steps, uint16_t pulses, uint16_t rotate, uint16_t tps);
gen_status gen_rhythm_rotate(gen_rhythm *r, int amount);
uint16_t   gen_rhythm_pulse_count(const gen_rhythm *r);

/* Expand rhythm hits into note events at pitch/vel/dur/ch (one note per hit). */
gen_status gen_rhythm_to_events(const gen_rhythm *r, gen_buf *out,
                                uint8_t note, uint8_t vel, uint32_t dur, uint8_t ch);

/* ---- melody ---------------------------------------------------------------- */
typedef struct gen_melody_cfg {
    int        root;          /* 0..127 */
    gen_scale  scale;
    int        pitch_min;     /* inclusive */
    int        pitch_max;     /* inclusive */
    uint8_t    vel_min;       /* 1..127 */
    uint8_t    vel_max;       /* 1..127 */
    uint32_t   duration;      /* ticks per note; 0 = use ticks_per_step */
    uint8_t    channel;       /* 0..15 */
    uint32_t   seed;
    /* random-walk */
    uint8_t    max_interval;  /* scale degrees */
    uint8_t    repeat_prob;   /* 0..100 */
    /* weighted degrees: weight[i] for degree i; NULL = equal */
    const uint8_t *degree_weights; /* length = scale degree count; optional */
} gen_melody_cfg;

gen_status gen_melody_uniform(const gen_melody_cfg *cfg, const gen_rhythm *rhythm, gen_buf *out);
gen_status gen_melody_weighted(const gen_melody_cfg *cfg, const gen_rhythm *rhythm, gen_buf *out);
gen_status gen_melody_walk(const gen_melody_cfg *cfg, const gen_rhythm *rhythm, gen_buf *out);

/* ---- chords / harmony ------------------------------------------------------ */
typedef enum gen_chord_quality {
    GEN_CHORD_TRIAD = 0,
    GEN_CHORD_SEVENTH,
    GEN_CHORD_NINTH,
    GEN_CHORD_SUS2,
    GEN_CHORD_SUS4,
    GEN_CHORD_MAJ,
    GEN_CHORD_MIN,
    GEN_CHORD_MAJ7,
    GEN_CHORD_MIN7,
    GEN_CHORD_DOM7
} gen_chord_quality;

#define GEN_CHORD_MAX_NOTES 5

typedef struct gen_chord {
    uint8_t notes[GEN_CHORD_MAX_NOTES];
    uint8_t count;
} gen_chord;

gen_status gen_chord_build(gen_chord *out, int root, gen_chord_quality q, int inversion, int octave_shift);
gen_status gen_chord_diatonic(gen_chord *out, int key_root, gen_scale scale, int degree /*0..6*/,
                              gen_chord_quality q, int inversion);
/* Voice-lead `next` toward `prev` by octave shifts of whole chord (±1..2 oct) minimizing total movement. */
gen_status gen_chord_voice_lead(const gen_chord *prev, gen_chord *next);

typedef struct gen_progression {
    uint8_t degrees[16]; /* scale degrees 0..6 */
    uint8_t count;
} gen_progression;

/* Well-known progressions (degrees in major unless noted). */
void gen_prog_I_V_vi_IV(gen_progression *p);
void gen_prog_ii_V_I(gen_progression *p);
void gen_prog_i_VI_III_VII(gen_progression *p); /* natural minor degrees */
void gen_prog_I_vi_ii_V(gen_progression *p);
void gen_prog_i_iv_V_i(gen_progression *p);

typedef struct gen_harmony_cfg {
    int        root;
    gen_scale  scale;
    gen_chord_quality quality;
    uint32_t   ticks_per_chord;
    uint32_t   duration; /* note duration ticks */
    uint8_t    channel;
    uint8_t    velocity;
    int        voice_lead; /* nonzero = voice lead consecutive chords */
    uint32_t   seed;       /* for seeded random progression */
} gen_harmony_cfg;

gen_status gen_harmony_progression(const gen_harmony_cfg *cfg, const gen_progression *prog, gen_buf *out);
gen_status gen_harmony_seeded(const gen_harmony_cfg *cfg, uint8_t bars, gen_buf *out);

/* ---- arpeggiator ----------------------------------------------------------- */
typedef enum gen_arp_mode {
    GEN_ARP_UP = 0,
    GEN_ARP_DOWN,
    GEN_ARP_UPDOWN,
    GEN_ARP_DOWNUP,
    GEN_ARP_AS_PLAYED,
    GEN_ARP_RANDOM,
    GEN_ARP_CONVERGE,
    GEN_ARP_DIVERGE
} gen_arp_mode;

typedef struct gen_arp_cfg {
    gen_arp_mode mode;
    uint32_t     rate;       /* ticks between notes */
    uint32_t     gate;       /* note duration ticks */
    uint8_t      octaves;    /* 1..4 additional octaves above */
    uint16_t     length;     /* number of arp steps to emit; 0 = one full cycle */
    uint8_t      velocity;
    uint8_t      channel;
    uint32_t     seed;
} gen_arp_cfg;

gen_status gen_arp(const gen_arp_cfg *cfg, const uint8_t *notes, uint8_t n_notes, gen_buf *out);
gen_status gen_arp_chord(const gen_arp_cfg *cfg, const gen_chord *chord, gen_buf *out);

/* ---- transforms ------------------------------------------------------------ */
gen_status gen_xform_transpose(gen_buf *b, int semitones);
gen_status gen_xform_reverse(gen_buf *b);
gen_status gen_xform_rotate(gen_buf *b, int steps, uint32_t ticks_per_step);
gen_status gen_xform_quantize(gen_buf *b, gen_scale scale, int root, int lo, int hi);
gen_status gen_xform_velocity(gen_buf *b, int delta_min, int delta_max, gen_rng *rng);
gen_status gen_xform_mutate_pitch(gen_buf *b, gen_scale scale, int root, int lo, int hi,
                                  uint8_t prob /*0..100*/, gen_rng *rng);
gen_status gen_xform_gate(gen_buf *b, int pct_min, int pct_max, gen_rng *rng); /* scale durations */
gen_status gen_xform_remove(gen_buf *b, uint8_t prob, gen_rng *rng);

#ifdef __cplusplus
}
#endif
#endif /* GENESIS_H */
