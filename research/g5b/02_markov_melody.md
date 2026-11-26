# Markov Melody Lab

The chain is first-order on scale degrees. `GEN_MARKOV_MAX_DEG` is 12, which covers chromatic. The model is `uint16_t w[12][12]` plus degree count, style, ready, and origin: **292 bytes** on host and on Thumb. There is no 128-by-128 MIDI matrix.

`gen_markov_generate` does not write the model. The caller owns the model. Learning uses a second caller-owned model as scratch; the destination model is replaced only after the phrase has been counted.

## Preset styles

These weights are written heuristics. They were not learned from a corpus. Distance is the shortest signed step around the degree cycle (`n/2` wraps). `gen_markov_style_weight` is the source of the numbers. `GEN_MARKOV_LEARNED` returns 0 from that function.

| Style | Rule |
| --- | --- |
| Stepwise | distance 0 → 12, 1 → 40, 2 → 8, otherwise 1 |
| Motif | unison 28, ±2 → 22, ±1 → 10, otherwise 2 |
| Bassline | base 2; +36 if the target degree is 0; +24 if the scale has at least 5 degrees and the target index is 4; +8 if the absolute distance is at most 1; +6 if the target index is at most 2 |
| Exploratory | unison 3, every other degree 10 |

On a seven-degree scale, index 4 is the diatonic fifth. On chromatic, index 4 is not a fifth. The bassline bonus still uses index 4.

A row whose weights sum to 0 is sampled uniformly across `n_deg`. That is the documented fallback. It is not a silent swap to a preset. Zero-weight cells in a row that has some positive weight are never chosen.

## Pitch and rests

The next degree is placed on the nearest in-range MIDI note of that degree. A tie keeps the lower pitch. `octave_motion == 0` tries the current MIDI octave first. If the chosen degree has no pitch in range, neighboring degrees are tried at increasing distance until one fits.

`rest_prob` 0 always sounds. `rest_prob` 100 writes no notes and needs no output capacity. A rest does not advance the chain. The degree is chosen when a note sounds, so a following rest holds that already chosen degree.

## Learned mode

`gen_markov_learn` reads the source in tick order and does not modify it.

- Events must be strictly increasing in `tick`, or the call returns `GEN_ERR_TIMING` and the destination model is unchanged.
- A note maps to a degree by pitch class against `gen_scale_degree_to_midi` at octave 0. Out-of-scale notes are skipped and break the chain: no transition is counted across them.
- `gap_ticks == 0` never breaks on time. A positive gap breaks the chain when the tick delta is greater than `gap_ticks`.
- A repeated in-scale note increments the self-transition.
- Empty input returns `GEN_ERR_EMPTY` and leaves the destination model unchanged.
- A single in-scale note marks the model ready, stores that degree as `origin`, and leaves every weight at 0. Generation then uses the uniform fallback.
- A phrase of only out-of-scale notes commits a ready model with no transitions and origin 0.
- `origin` is the last in-scale degree, so generation can continue the phrase when `initial_degree` is negative.
- Counts are `uint16_t`. `gen_markov_add` returns `GEN_ERR_OVERFLOW` when the cell is already 65535 and does not wrap. Learning returns the same error before it copies into the destination, so a failed learn does not publish a partial matrix.

Learned mode is implemented and is inside the measured 1907-byte Markov object. It is not substituted by a preset.
