# 03 — Musical Algorithms

## Scales

14 modes (chromatic through whole-tone). Quantize ties prefer the **lower** pitch.

## Euclidean rhythm

Bresenham distribution of `pulses` into `steps`. Rotation: right-rotate by `k` so index `i` loads former `(i - k) mod n`. Pulse count verified equal to request (including 0 and `steps`).

## Melody

- Uniform: equal weight over in-range scale pitches.
- Weighted: per-degree weights.
- Walk: bounded degree steps, repeat probability, stays in scale set.

## Harmony

Diatonic triads/7ths/9ths from scale degrees; explicit qualities; inversions; optional octave-shift voice leading minimizing nearest-neighbor cost. Progressions: I–V–vi–IV, ii–V–I, i–VI–III–VII, I–vi–ii–V, i–iv–V–i.

**Monophonic Play+ note:** full chords remain in the portable buffer. G3 may distribute voices across multiple external MIDI tracks (one note per track per step) — routing is deferred.

## Arpeggiator

Up / Down / UpDown / DownUp / AsPlayed / Random / Converge / Diverge. UpDown omits peak repeat; DownUp omits trough repeat. Duplicate input pitches uniqued. Octave expansion clamps at MIDI 127.
