# 03 — Generator Configuration

## Defaults

| Field | Default |
|---|---|
| Lab | Melody |
| Track | Select-pad or 8 |
| Length | 16 (capped to native plen) |
| Seed | 42 |
| Root | C4 (60) |
| Scale | Natural minor |
| Density | 70% |
| Pitch | 48–72 |
| Velocity | 64–110 (1–127 enforced) |
| Gate | 100% of step (`GEN_TPS=480`) |
| Euclid pulses / rot | 5 / 0 |
| Arp mode / oct / preset | Up / 1 / Maj3 |

## Melody Lab

- Algo: Uniform | Walk (`max_interval`, `repeat_prob`).
- Rejects inverted pitch/vel ranges.
- Uses `gen_rhythm_probability` + `gen_melody_uniform` / `gen_melody_walk`.

## Euclidean Lab

- `gen_rhythm_euclidean(len, pulses, rot, TPS)` + scale-aware `gen_melody_uniform`.
- Pulses clamped to `≤ len`; zero/full pulse safe via G2.
- Does not change native track length.

## Arpeggiator

- Modes: Up, Down, UpDn, DnUp, Order, Rand, Conv, Div.
- Note sources: Maj3, Min3, 7th, Scale degrees (monophonic).
- Rate = one event per step (`GEN_TPS`); off-grid rates not exposed.
- No live held-note input; no real-time arp.

## Naming / units

- Notes: `C`–`B` + sharp; octave = `midi/12 - 1` (C4 = 60).
- Gate%: percent of one step → ticks.
- Seed: `u16`; Regenerate mixes `seed * 33 + millis + 0x9e37` (never 0).
- Length UI max 64 but capped to native pattern length on refresh.
