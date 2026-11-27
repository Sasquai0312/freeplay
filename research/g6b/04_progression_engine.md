# Progression engine

`genesis/engine/chordprog.c` is the firmware generator. `chords.c` stays in the host Makefile for its existing tests and is not in `target.json`.

`gen_native_progression` fills a caller-owned `gen_buf` and a caller-owned `uint8_t` id sidecar. It does not allocate, recurse, or use floating point. It reuses scale degrees, `gen_rng`, and `gen_buf_validate`. On any error the event count is set to 0. Output is not truncated: a short buffer returns `GEN_ERR_CAPACITY`.

A chord event carries its id in the sidecar. Velocity, duration, channel, and note are not overloaded with quality.

## Presets

Degrees, not qualities. The quality comes from the scale.

| Preset | Degrees |
|---|---|
| I–V–vi–IV | 0, 4, 5, 3 |
| I–vi–IV–V | 0, 5, 3, 4 |
| ii–V–I | 1, 4, 0 |
| I–vi–ii–V | 0, 5, 1, 4 |
| i–VI–III–VII | 0, 5, 2, 6 |
| i–iv–VI–VII | 0, 3, 5, 6 |
| i–iv–v | 0, 3, 4 |
| Random | uniform degree from the scale |

If a preset degree is past the scale, the call returns `GEN_ERR_UNSUPPORTED`.

## Quality

| Mode | Rule |
|---|---|
| Triads | major 3, minor 4, dim 5, aug 6, from the third and the fifth |
| Sevenths | maj7 11, dom7 14, min7 12, minmaj7 13 |
| Fixed | one id 0..20 |
| Seeded | one of {3, 4, 7, 8, 11, 12, 14} |

Half-diminished and diminished sevenths are not stock ids. Those degrees return `GEN_ERR_UNSUPPORTED` and an empty buffer. The UI toasts `No quality`. Major pentatonic I–V–vi–IV is rejected for the same reason. Natural-minor I–V–vi–IV is i–v–VI–iv (minor, minor, major, minor), which the default UI scale produces. The lab does not switch the user's scale to force a major spelling.

Harmonic-minor III is Aug (6). Harmonic-minor i as a seventh is MinMaj7 (13). Natural-minor VII as a seventh is Dom 7th (14). Whole-tone triads are Aug.

Pitch is `root + (degree midi at 60, minus 60)`. A result outside 0..127 is `GEN_ERR_PITCH`, not a clamp.

## Timing

Phrase length is the existing 1..64 step field. Change intervals are only 4, 8, 16, and 32, and only when the interval fits the phrase. The UI offers 32 only when the phrase is at least 32.

Sustained mode (`Hold`): one event per span. Duration ticks are `span * gate_ticks`. The last partial span is shorter. The progression repeats when the phrase is longer than one cycle.

Step mode: one event per step inside the span, duration `gate_ticks`, same id.

`gate_ticks` must be in 1..ticks-per-step (480 in the UI). Native duration is `round(ticks * 60 / ticks_per_step)` and must land in 1..3840. A 32-step sustain at 480 ticks per step and a full gate is 15,360 ticks and is rejected (`GEN_ERR_TIMING`) rather than clamped. An 8-step sustain at gate 100% is 3,840 ticks, native duration 480, which Apply stored and read back as 60 display steps on a one-step gate and as the matching native value on the chord events.

Touching endpoints do not overlap. Chord note-off behavior is not claimed: duration is the value written to parameter 1, and playback release was not measured.

Host tests cover every preset, major and minor spelling, sevenths including the half-diminished rejection, fixed and seeded modes, 100 seeds, identical seeds, a changed seed, lengths 16/32/64, partial last spans, retrigger, pitch, timing, capacity, and id 21. GCC ASan and UBSan passed. Clang ASan is not installed (`libclang_rt.asan_static-x86_64.a`).
