/* G3A native MIDI adapter: G2 gen_event <-> G1B verified Play+ sequencer writes.
 * No production Genesis UI. Channel metadata is not written to firmware. */
#ifndef GENESIS_ADAPT_H
#define GENESIS_ADAPT_H

#include "cfw.h"
#include "genesis.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GEN_MIDI_PARAM_VEL 0
#define GEN_MIDI_PARAM_DUR 1
#define GEN_MIDI_PARAM_NOTE 6
#define GEN_MIDI_DUR_PER_STEP 60 /* G1B-verified: display steps ≈ units/60 */
#define GEN_MIDI_DUR_MAX 3840
#define GEN_SNAP_MAGIC 0x47334153u /* "G3AS" */
#define GEN_SNAP_MAX 64
/* Verified native Track Lengths the adapter will edit. 128 is not a native length. */
#define GEN_MIDI_CAP_LEN16 (1u << 16)
#define GEN_MIDI_CAP_LEN32 (1u << 17)
#define GEN_MIDI_CAP_LEN64 (1u << 18)
#define GEN_MIDI_DEST_MAX 64

typedef enum gen_apply_policy {
    GEN_APPLY_EMPTY_ONLY = 0,
    GEN_APPLY_REPLACE_REGION = 1
} gen_apply_policy;

typedef enum gen_midi_err {
    GEN_MIDI_OK = 0,
    GEN_MIDI_ERR_BAD_TRACK = -1,
    GEN_MIDI_ERR_BAD_STEP = -2,
    GEN_MIDI_ERR_UNSUPPORTED = -3,
    GEN_MIDI_ERR_CAPACITY = -4,
    GEN_MIDI_ERR_BUSY = -5,
    GEN_MIDI_ERR_INTERNAL = -6,
    GEN_MIDI_ERR_TIMING = -7,
    GEN_MIDI_ERR_OCCUPIED = -8,
    GEN_MIDI_ERR_MONO = -9,
    GEN_MIDI_ERR_CHANNEL = -10,
    GEN_MIDI_ERR_RESTORE = -11,
    GEN_MIDI_ERR_PARAM = -12
} gen_midi_err;

typedef struct gen_native_step {
    u8 on;
    u8 _pad;
    s16 note; /* -1 = unlocked / unknown */
    s16 vel;
    s16 dur;
} gen_native_step;

typedef struct gen_snap {
    u32 magic;
    s32 track;
    s32 step0;
    s32 count; /* number of steps in region */
    gen_native_step steps[GEN_SNAP_MAX];
} gen_snap;

/* Read-only preview contract for G3B (no native writes). */
typedef struct gen_preview {
    s32 track;
    s32 step0;
    s32 length; /* pattern length */
    u32 seed;
    u32 event_count;
    u8 generator; /* opaque tag */
    u8 valid;     /* 1 if buffer validated for apply */
    u8 policy;
    u8 _pad;
    u8 occupancy[GEN_SNAP_MAX];
    u8 notes[GEN_SNAP_MAX];
    u8 vels[GEN_SNAP_MAX];
    u16 durs[GEN_SNAP_MAX]; /* native units */
} gen_preview;

u32 genesis_midi_caps(void);

int genesis_midi_validate_track(void *ctx, int track);
int genesis_midi_pattern_length(void *ctx, int track);
int genesis_midi_read_step(void *ctx, int track, int step, gen_native_step *out);

/* ticks_per_step: portable ticks corresponding to one sequencer step.
 * duration_native = round(duration_ticks * 60 / ticks_per_step), clamp 0..3840.
 * Event tick must be an exact multiple of ticks_per_step. */
int genesis_midi_ticks_to_step(u32 tick, u32 ticks_per_step, u32 *step_out);
int genesis_midi_ticks_to_dur(u32 duration_ticks, u32 ticks_per_step, s16 *dur_out);

int genesis_midi_validate_events(void *ctx, int track, int step0, const gen_event *ev, u32 n,
                                 u32 ticks_per_step, gen_apply_policy policy);

int genesis_midi_begin(void *ctx, const char *undo_label);
int genesis_midi_upsert(void *ctx, int track, int step, u8 note, u8 vel, s16 dur);
int genesis_midi_remove(void *ctx, int track, int step);
int genesis_midi_commit(void *ctx);
/* cancel: no further writes; does not pop native undo */
int genesis_midi_cancel(void *ctx);

int genesis_midi_snapshot(void *ctx, int track, int step0, int count, gen_snap *snap);
int genesis_midi_restore(void *ctx, const gen_snap *snap);

/* Request structs keep call sites ≤4 register args (AAPCS / vdev call_fn). */
typedef struct gen_apply_req {
    void *ctx;
    int track;
    int step0;
    u32 ticks_per_step;
    const gen_event *ev;
    u32 n;
    gen_apply_policy policy;
    int fail_at; /* 0=none; 1..9 diagnostic stages — see genesis_adapt.c */
    gen_snap *snap_out;
} gen_apply_req;

typedef struct gen_preview_req {
    void *ctx;
    int track;
    int step0;
    u32 ticks_per_step;
    const gen_event *ev;
    u32 n;
    u32 seed;
    u8 generator;
    u8 policy;
    u8 _pad[2];
    gen_preview *out;
} gen_preview_req;

typedef struct gen_fixture_req {
    void *ctx;
    int fixture_id; /* A=1 B=2 C=3 D=4 */
    int track;
    int step0;
    gen_apply_policy policy;
    int fail_at;
    gen_snap *snap_out;
    gen_preview *preview_out; /* if non-NULL, preview only (no writes) */
} gen_fixture_req;

/* Generate-only request (no native writes). For ABI / equivalence tests. */
typedef struct gen_gen_req {
    int fixture_id;
    u32 capacity;
    gen_event *out; /* caller-owned; target / emulated memory */
    u32 *count_out; /* optional */
    u32 *seed_out;  /* optional */
} gen_gen_req;

int genesis_midi_apply_req(const gen_apply_req *req);
int genesis_midi_preview_req(const gen_preview_req *req);
int genesis_diag_fixture(const gen_fixture_req *req);
int genesis_diag_generate(const gen_gen_req *req);

/* Convenience wrappers (host / multi-arg call sites). */
int genesis_midi_apply(void *ctx, int track, int step0, const gen_event *ev, u32 n,
                       u32 ticks_per_step, gen_apply_policy policy, gen_snap *snap_out);
int genesis_midi_apply_ex(void *ctx, int track, int step0, const gen_event *ev, u32 n,
                          u32 ticks_per_step, gen_apply_policy policy, gen_snap *snap_out,
                          int fail_at);
int genesis_midi_preview_build(void *ctx, int track, int step0, const gen_event *ev, u32 n,
                               u32 ticks_per_step, u32 seed, u8 generator, gen_apply_policy policy,
                               gen_preview *out);

#ifdef __cplusplus
}
#endif
#endif
