# 05 — Polyphonic MIDI Validation

## Question

Does one sequencer step store multiple MIDI notes?

## Experiments

1. Single note at param 6; dump 42-byte step object.  
2. Change note 60→64; only **one byte** at step offset 31 changed.  
3. No second `getParam` index identified as an additional note slot via UI differentials.  
4. No verified native UI path found to enter a Cmaj7 chord into one step.

## Result

| Item | Status |
|---|---|
| Multi-note per step | **Not verified** |
| Max notes / step | Unknown |
| Duplicate / per-note velocity / duration | Unknown |
| Neighbor preservation under poly writes | N/A |

## Decision

Retain **monophonic** implementation path. Capability flag `POLYPHONIC` stays **off**. Do not assume product marketing chords imply a firmware multi-note step API usable by Genesis.
