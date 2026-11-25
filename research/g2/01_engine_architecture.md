# 01 — Engine Architecture

## Layout

```
genesis/
  include/genesis.h          # public API
  include/genesis_smf.h      # host-only SMF
  engine/{event,random,scales,rhythm,melody,chords,arpeggiator,transform}.c
  tests/test_genesis.c
  tools/{smf_export.c,gen_demos.c}
  examples/                  # MIDI + sidecar configs
  Makefile
  README.md
```

## Principles

- Caller-owned `gen_buf` / fixed arrays — no heap in generators.
- Explicit `gen_rng` state — no global RNG.
- Modules independently compilable (`-ffunction-sections`).
- Capacity failure rolls back `count` to the pre-call value (no partial success).
- Polyphonic chords preserved in buffers; `gen_buf_validate(..., monophonic=1)` detects overlaps.
- SMF exporter is host-only and must not be linked into firmware.
