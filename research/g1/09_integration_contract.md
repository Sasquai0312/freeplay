# 09 — Proposed Firmware Adapter Contract (for G3)

Conceptual only. **No production integration code in G1.** Independent of Genesis musical engine.

## Capability flags

```c
typedef uint32_t GenesisMidiCaps;
enum {
    GEN_MIDI_CAP_MONO       = 1u << 0,
    GEN_MIDI_CAP_POLY       = 1u << 1,
    GEN_MIDI_CAP_VELOCITY   = 1u << 2,
    GEN_MIDI_CAP_DURATION   = 1u << 3,
    GEN_MIDI_CAP_MICROTIME  = 1u << 4,
    GEN_MIDI_CAP_PROBABILITY= 1u << 5,
    GEN_MIDI_CAP_CC         = 1u << 6,  /* step or track CC locks when known */
    GEN_MIDI_CAP_NATIVE_UNDO= 1u << 7,
};
```

Until discovery completes, only `GEN_MIDI_CAP_NATIVE_UNDO` is tentatively expected (sample-proven mechanism); mono/velocity/duration flags stay off.

## Error codes

```c
enum {
    GEN_MIDI_OK = 0,
    GEN_MIDI_ERR_BAD_TRACK = -1,    /* not MIDI */
    GEN_MIDI_ERR_BAD_STEP  = -2,
    GEN_MIDI_ERR_UNSUPPORTED = -3,  /* capability missing */
    GEN_MIDI_ERR_CAPACITY  = -4,
    GEN_MIDI_ERR_BUSY      = -5,
    GEN_MIDI_ERR_INTERNAL  = -6,
};
```

## Operations

```c
/* Query */
GenesisMidiCaps genesis_midi_caps(void);
int  genesis_midi_validate_track(void *ctx, int track); /* 0 ok */
int  genesis_midi_pattern_length(void *ctx, int track); /* steps or error */

/* Read */
int  genesis_midi_read_step(void *ctx, int track, int step,
                            struct GenesisMidiStep *out);

/* Transaction */
int  genesis_midi_begin(void *ctx, const char *undo_label);
int  genesis_midi_upsert(void *ctx, int track, int step,
                         const struct GenesisMidiEvent *ev); /* insert/update */
int  genesis_midi_remove(void *ctx, int track, int step);    /* clear note/trig */
int  genesis_midi_commit(void *ctx);   /* refresh pads; end */
int  genesis_midi_cancel(void *ctx);   /* no writes after begin ⇒ Back path */
```

`GenesisMidiEvent` mirrors portable engine fields (note, velocity, duration, …) **without** embedding firmware indices — adapter maps fields using the verified table from a completed G1.

## Implementation binding (future)

| Contract op | Likely FreePlay primitive |
|---|---|
| validate_track | `STEP_VT_TRACK_TYPE` + TBD MIDI-vs-synth test |
| pattern_length | `TRK_VT_LENGTH` |
| read_step | `cfw_step_at` + getParam / isOn |
| begin | `cfw_edit_begin` |
| upsert/remove | lock + `fw_step_toggle` + setParam |
| commit | unlock discipline + `cfw_refresh_pads` + toast |
| cancel | discard plan; no begin, or rely on not writing |

## Forbidden in this contract

- Hard-coded speculative param indices.  
- Addresses not present in FreePlay `symbols.ld` / `target.h`.  
- Coupling to Euclidean/Markov/etc. generators.

## G3 readiness

Contract is ready to implement **after** §04 MIDI note (and preferably velocity/duration) are verified. Until then, treat adapter as specification only.
