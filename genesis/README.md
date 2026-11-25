# Genesis — portable generative MIDI engine

Freestanding-friendly C library for deterministic melody, rhythm, harmony and
arpeggiation. No firmware hooks, no `malloc` in generation paths, no global RNG.

## Build & test

```bash
cd genesis
make test          # unit + property tests
make demos         # write examples/*.mid + *.txt
make zip-demos     # examples/genesis_demos.zip
make thumb-size    # ARM Thumb -Os per-module sizes (needs clang thumb target)
```

## Quick API sketch

```c
gen_event storage[256];
gen_buf out;
gen_rhythm r;
gen_melody_cfg cfg = {0};

gen_buf_init(&out, storage, 256);
gen_rhythm_euclidean(&r, 16, 5, 0, 120);
cfg.root = 60; cfg.scale = GEN_SCALE_NAT_MINOR;
cfg.pitch_min = 48; cfg.pitch_max = 72;
cfg.vel_min = 80; cfg.vel_max = 100; cfg.duration = 100; cfg.seed = 42;
gen_melody_uniform(&cfg, &r, &out);
```

MIDI channels in `gen_event` are **0..15** (wire channel = value + 1).

## Modules

| File | Role |
|---|---|
| `event.c` | buffers, sort, mono validation |
| `random.c` | xorshift32 PRNG |
| `scales.c` | 14 scales, quantize, degree map |
| `rhythm.c` | fixed / probability / Euclidean |
| `melody.c` | uniform / weighted / random-walk |
| `chords.c` | chords, progressions, voice lead |
| `arpeggiator.c` | offline arp patterns |
| `transform.c` | transpose, mutate, … |
| `tools/smf_export.c` | **host-only** Standard MIDI File writer |

## Audition demos

Open `examples/*.mid` in any DAW. Sidecar `*.txt` lists seed, scale, settings and an event dump.

## G3 note

This tree never touches FreePlay firmware APIs. See `research/g2/10_g3_integration_requirements.md`.
