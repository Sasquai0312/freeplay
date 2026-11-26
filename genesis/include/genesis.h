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

/* ---- native-compatible timing --------------------------------------------- */
/* Verified adapter contract: native_dur = round(ticks * 60 / ticks_per_step),
 * clamped by the device to 1..3840. Native Apply is monophonic, so a
 * native-compatible duration also stays inside one step (1..ticks_per_step). */
#define GEN_NATIVE_DUR_MAX 3840u

static inline int gen_native_duration(uint32_t ticks, uint32_t ticks_per_step)
{
    if (ticks_per_step == 0 || ticks == 0 || ticks > ticks_per_step)
        return 0;
    return (int)((ticks * 60u + ticks_per_step / 2u) / ticks_per_step);
}

static inline int gen_native_duration_ok(uint32_t ticks, uint32_t ticks_per_step)
{
    int n = gen_native_duration(ticks, ticks_per_step);
    return n >= 1 && (uint32_t)n <= GEN_NATIVE_DUR_MAX;
}

/* ---- Turing register ------------------------------------------------------- */
/* Looping shift-register phrase. Not a clone of any commercial module.
 * Each cell is a MIDI note 0..127 or GEN_TURING_REST. Rests live in the
 * register, so a locked or 0% pattern repeats pitch and rest together.
 * State is caller-owned. Generate reads `src` and writes the proposed next
 * state to `next` without modifying `src`. */
#define GEN_TURING_MAX_LEN   64
#define GEN_TURING_MAX_STEPS 128
#define GEN_TURING_REST      0xFFu

typedef struct gen_turing_cfg {
    uint32_t gate;
    uint32_t ticks_per_step;
    uint32_t seed;
    int16_t  root;
    int16_t  pitch_min;
    int16_t  pitch_max;
    uint16_t steps;          /* 1..128 phrase length */
    uint8_t  scale;
    uint8_t  length;         /* 1..64 register length */
    uint8_t  evolve_prob;    /* 0..100; 0 repeats, 100 redraws every advance */
    uint8_t  rest_prob;      /* 0..100, used only when a cell is written */
    uint8_t  velocity;
    uint8_t  locked;         /* nonzero: advance playback, do not rewrite cells */
    uint8_t  channel;
    uint8_t  _pad;
} gen_turing_cfg;

typedef struct gen_turing {
    gen_turing_cfg cfg;
    gen_rng  rng;
    uint8_t  cells[GEN_TURING_MAX_LEN];
    uint8_t  length;
    uint8_t  pos;
    uint8_t  ready;
    uint8_t  _pad;
} gen_turing;

gen_status gen_turing_init(gen_turing *st, const gen_turing_cfg *cfg);
gen_status gen_turing_seed(gen_turing *st, uint32_t seed);
gen_status gen_turing_reset(gen_turing *st);
gen_status gen_turing_copy(gen_turing *dst, const gen_turing *src);
/* `cfg` NULL uses the configuration stored in `src`. Playback fields in a
 * non-NULL cfg override this call only and are not stored. Register length
 * always comes from `src`. `next` receives the advanced state. */
gen_status gen_turing_generate(const gen_turing *src, const gen_turing_cfg *cfg,
                               gen_buf *out, gen_turing *next);

/* ---- Markov melody --------------------------------------------------------- */
/* First-order chain on scale degrees (at most 12), not a MIDI-note matrix.
 * Preset styles are designed heuristics, not weights learned from a corpus.
 * A learned model is a separate caller-owned object. Generate does not
 * modify the model. */
#define GEN_MARKOV_MAX_DEG 12
#define GEN_MARKOV_STEPWISE 0u
#define GEN_MARKOV_MOTIF 1u
#define GEN_MARKOV_BASSLINE 2u
#define GEN_MARKOV_EXPLORATORY 3u
#define GEN_MARKOV_LEARNED 4u

typedef struct gen_markov_model {
    uint16_t w[GEN_MARKOV_MAX_DEG][GEN_MARKOV_MAX_DEG];
    uint8_t  n_deg;
    uint8_t  style;
    uint8_t  ready;
    uint8_t  origin; /* degree used when cfg.initial_degree < 0 for a learned model */
} gen_markov_model;

typedef struct gen_markov_cfg {
    uint32_t gate;
    uint32_t ticks_per_step;
    uint32_t seed;
    int16_t  root;
    int16_t  pitch_min;
    int16_t  pitch_max;
    int16_t  initial_degree; /* -1: preset draws, learned uses model->origin */
    uint16_t steps;
    uint8_t  scale;
    uint8_t  velocity;
    uint8_t  rest_prob;
    uint8_t  octave_motion; /* 0: prefer the current octave, then fall back */
    uint8_t  channel;
    uint8_t  _pad[3];
} gen_markov_cfg;

gen_status gen_markov_preset(gen_markov_model *m, uint8_t scale, uint8_t style);
/* Heuristic weight for one preset cell. LEARNED returns 0. */
uint8_t    gen_markov_style_weight(uint8_t style, uint8_t n_deg, uint8_t from, uint8_t to);
/* Replace counts from `src`. `scratch` is required and must differ from `m`.
 * On error `m` is unchanged. gap_ticks == 0 does not break the chain on time gaps.
 * Out-of-scale notes are skipped and break the chain. */
gen_status gen_markov_learn(gen_markov_model *m, uint8_t scale, int root,
                            const gen_buf *src, uint32_t gap_ticks,
                            gen_markov_model *scratch);
/* Accumulate one observed transition. Saturating at 65535 returns OVERFLOW
 * and leaves the cell unchanged. */
gen_status gen_markov_add(gen_markov_model *m, uint8_t from, uint8_t to);
gen_status gen_markov_generate(const gen_markov_model *m, const gen_markov_cfg *cfg,
                               gen_buf *out);

/* ---- mutation lab ---------------------------------------------------------- */
/* One request applies a fixed order of operations to a copy of `src`.
 * `src` is never written. Aliasing `out->events` with `src->events` is rejected
 * before any write. `out->count` must be 0. Unconditional reverse/rotate are
 * distinct from probabilistic mutations: probability 0 with those flags clear
 * copies the source. */
#define GEN_MUTATE_MAX_STEPS 128
#define GEN_MUTATE_NATIVE_STEPS 64
#define GEN_MUT_PROTECT_PITCH 0x01u
#define GEN_MUT_PROTECT_RHYTHM 0x02u
#define GEN_MUT_PROTECT_VELOCITY 0x04u
#define GEN_MUT_PROTECT_DURATION 0x08u

typedef struct gen_mutate_req {
    uint32_t ticks_per_step;
    uint32_t gate_min;
    uint32_t gate_max;
    uint32_t seed;
    int16_t  root;
    int16_t  pitch_min;
    int16_t  pitch_max;
    uint16_t dest_steps;     /* 1..128; bytes 0..7 of the mask cover native 1..64 */
    int8_t   octave;         /* -2..2; 0 = off; unconditional */
    int8_t   rotate_steps;   /* 0 = off; unconditional; passed to gen_xform_rotate */
    uint8_t  scale;
    uint8_t  pitch_prob;
    uint8_t  quantize;       /* 0 or 1; unconditional */
    uint8_t  displace_prob;
    uint8_t  insert_prob;
    uint8_t  remove_prob;
    uint8_t  velocity_prob;
    uint8_t  gate_prob;
    uint8_t  reverse;        /* 0 or 1; unconditional */
    uint8_t  intensity;      /* 0..24 semitones / steps */
    uint8_t  protect;        /* GEN_MUT_PROTECT_* */
    uint8_t  vel_min;
    uint8_t  vel_max;
    uint8_t  _pad;
    /* Bit i protects step i. Bits 0..63 are the native mask.
     * Bits 64..127 are the portable extension. */
    uint8_t  protect_steps[GEN_MUTATE_MAX_STEPS / 8];
} gen_mutate_req;

void gen_mutate_protect_step(gen_mutate_req *req, unsigned step, int on);
int  gen_mutate_step_protected(const gen_mutate_req *req, unsigned step);
/* `out->capacity` must be at least `dest_steps` (one event per step). */
gen_status gen_mutate(const gen_buf *src, const gen_mutate_req *req, gen_buf *out);

#ifdef __cplusplus
}
#endif
#endif /* GENESIS_H */
