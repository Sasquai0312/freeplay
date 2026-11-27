# Architecture comparison

| | Path A, several notes per step | Path B, native chord id | Path C, one track per voice |
|---|---|---|---|
| Evidence | contradicted | storage and editing verified | the mono adapter already works, one note per track |
| Native compatibility | no second slot | uses the field the stock knob writes | uses the verified mono contract |
| Musical flexibility | would be free pitches | the 21 stock qualities, one root, shared velocity and duration | free pitches, inversions, per-voice velocity |
| Voices | not present | one stored note plus one id. Sounding count was not measured | one sounding note per track, so a four-note chord uses four tracks |
| Transaction | not available | one extra halfword in the snapshot | one existing snapshot per track |
| Snapshot memory | — | +128 bytes of stack snapshot | no new field, several tracks |
| UI | a voice editor that has no native target | one more value on the existing panel | several tracks to keep aligned |
| Linked code | none, because there is nothing to write | a small id map. `chords.c` stays out | the existing mono writer |
| Cave | none | none | none |
| Static RAM | none | none | none |
| Playback confidence | none | the flush reads the id; expansion was not seen | monophonic MIDI playback is the G5F owner result |
| Still open | — | owner confirmation that the id sounds as the named chord | track count and phase alignment |

Path B is the one that matches a mechanism the firmware already has. Path A is not available. Path C remains the fallback if a progression needs an inversion or a pitch the 21 names cannot spell, and it is not the default.

Useful progressions that Path B can name: major and minor triads, sus2, sus4, major 7, minor 7, dominant 7, and the native extensions when the lab offers those ids directly. A first-inversion C major is a Path C case, or a later phase.
