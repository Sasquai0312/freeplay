# 02 — Public API

Header: `genesis/include/genesis.h`

## Channels

`gen_event.channel` is **0..15**. MIDI wire channel number = `channel + 1`.

## Core types

- `gen_event` — tick, note, velocity, duration, channel (12 bytes on host).
- `gen_buf` — `{events, capacity, count}`.
- `gen_status` — `GEN_OK` or `GEN_ERR_*` (param/pitch/timing/capacity/unsupported/mono/empty/overflow).

## Generators

| API | Purpose |
|---|---|
| `gen_rhythm_*` | Fixed / probability / Euclidean / rotate |
| `gen_melody_{uniform,weighted,walk}` | Scale-aware melody over a rhythm |
| `gen_chord_*` / `gen_harmony_*` | Chords + progressions |
| `gen_arp` / `gen_arp_chord` | Offline arpeggiator |
| `gen_xform_*` | Transpose, reverse, rotate, quantize, mutate, … |

## Example

See `genesis/README.md` and `tools/gen_demos.c`.
