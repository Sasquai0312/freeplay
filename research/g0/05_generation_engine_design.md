# 05 — Shared Generative Engine Architecture (Proposal Only)

**Status:** Design for later phases. **Not implemented in G0.**

## Goals

- Portable **C99** (or freestanding-compatible C) library with **zero firmware includes**.  
- Host unit tests with standard toolchain.  
- Same code linked into FreePlay ext/cave later.  
- Deterministic, bounded memory, no `malloc` on generate/transform paths.

## Proposed layout

```
genesis/
  include/genesis/
    event.h      /* GenEvent, GenBuffer */
    rng.h        /* seeded PRNG */
    scale.h      /* pitch class sets */
    rhythm.h     /* Euclidean etc. */
    melody.h     /* constrained random melody */
    transform.h  /* mutate, rotate, invert */
    engine.h     /* high-level generate API */
  src/           /* portable implementations */
  tests/         /* host unit tests */
```

Firmware adapter (future, separate):

```
cfw/src/genesis_ui.c      /* menu / panel */
cfw/src/genesis_apply.c   /* maps GenEvent → step setParam */
```

## Event representation

```c
typedef struct {
    uint16_t tick;      /* step index or subdivision tick */
    uint8_t  note;      /* 0..127 */
    uint8_t  velocity;  /* 1..127; 0 = rest / skip */
    uint8_t  duration;  /* engine units; adapter maps to firmware length */
    uint8_t  channel;   /* 0 = use track default */
    uint8_t  probability; /* 0..100; adapter may map to Chance */
    int8_t   timing;    /* microtiming offset; 0 if unused */
    uint8_t  flags;     /* poly voice index, tie, etc. */
} GenEvent;

#define GEN_EVENT_CAP 128   /* fixed */

typedef struct {
    GenEvent events[GEN_EVENT_CAP];
    uint16_t count;
    uint16_t length;    /* pattern length in steps */
    uint32_t seed;
} GenBuffer;
```

Rationale: fixed capacity matches Play max 64 steps with headroom for poly/ratchet expansion; fits RT heap/stack budgets. Class: design proposal (not verified on device).

## Design decisions

| Concern | Recommendation |
|---|---|
| Seeded randomness | xorshift32 (matches FreePlay `cfw_ram.rand` style) |
| Dynamic allocation | Forbidden in `genesis_*` generate/transform |
| Scale constraints | Bitmask or table of degree→semitone; root + octave range |
| Pitch vs rhythm | Separate generators; merge by step index |
| Harmony context | Shared `GenHarmony { root, scale_id, chord[] }` passed in |
| Polyphony | Multiple `GenEvent` with same `tick`, distinct `flags` voice id |
| Transforms | Operate on `GenBuffer` in place with scratch `GenBuffer` |
| Host tests | Pure C tests: Euclidean known vectors, seed stability, range clamps |
| Firmware deps | Only in adapter: step APIs, UI, undo |

## Generator modules (v0.1 scope)

1. **Scale table** — major/minor/modes subset (stock has 39 scale names via `fw_scale_names` — may reuse labels later).  
2. **Euclidean rhythm** — Bjorklund; density + length → step mask.  
3. **Random melody** — pick degrees in scale within octave range; optional contour.  
4. **Merge** — rhythm mask × pitch stream → `GenBuffer`.  

Deferred: Markov, Turing, Brownian, CA, voice leading, multi-track coordinator, CC LFO — same buffer/transform APIs.

## Adapter contract (future)

```c
/* returns 0 ok; negative = unsupported / bad track */
int genesis_apply(void *ctx, int track, const GenBuffer *buf, uint32_t mode);
```

Modes (bitflags): `OVERWRITE_NOTES`, `FILL_RESTS`, `REPLACE_TRIGS`, `DRY_RUN` (validate only).

Adapter responsibilities:

- Resolve MIDI param indices (from verified table).  
- `cfw_edit_begin` / locks / toggle / setParam / refresh / toast.  
- Clamp note 0–127, velocity 1–127.  
- Refuse synth tracks until discrimination exists.

## Why this minimizes firmware coupling

- Musical algorithms never include `target.h`.  
- Host CI can run without `.ppf`.  
- When MIDI indices are discovered, only `genesis_apply.c` changes.  
- Preview can consume `GenBuffer` for LED/screen without Apply.

## Non-goals for G0/G1 engine

- Real-time audio-thread generation.  
- Direct MIDI TX.  
- Persistence format (may later mirror `cfwMod.*` style).
