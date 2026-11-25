# 06 — Polyphonic Representation

## Status

**Unresolved.** No vdev chord experiments (firmware missing).

## Planned diagnostic chord

Cmaj7: 60, 64, 67, 71 on one step — only if native UI/API supports multi-note steps.

## Questions (all UNKNOWN)

| Question | Status |
|---|---|
| Max notes per step | UNKNOWN |
| Velocity per note vs per step | UNKNOWN |
| Duration per note vs per step | UNKNOWN |
| Note ordering significance | UNKNOWN |
| Channel per track / step / note | UNKNOWN |
| Edit/remove individual chord tones | UNKNOWN |

## Monophonic preservation

Monophonic progress remains the G3 fallback: one note per step via toggle + setParam once indices are verified. Polyphony is optional capability flag in the integration contract.

## Product context (not firmware proof)

Public Play/Play+ manuals describe MIDI sequencing and chords in UI terms. That does **not** verify storage layout. Class: INFERRED (product exists) / UNKNOWN (representation).
