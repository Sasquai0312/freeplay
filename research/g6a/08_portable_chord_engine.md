# Portable chord engine

`genesis/engine/chords.c` is not linked into the firmware. SMF export is host-only and must stay that way.

What the file already does:

| Capability | Support |
|---|---|
| Major and minor triads | `GEN_CHORD_MAJ`, `GEN_CHORD_MIN`, and `GEN_CHORD_TRIAD` when the scale third is major or minor |
| Seventh chords | `GEN_CHORD_MAJ7`, `GEN_CHORD_MIN7`, `GEN_CHORD_DOM7`, plus scale-derived sevenths |
| Suspended | `GEN_CHORD_SUS2`, `GEN_CHORD_SUS4` |
| Extended | `GEN_CHORD_NINTH` is a dominant ninth (0, 4, 7, 10, 14). No 11th or 13th |
| Inversions | `gen_chord_build` rotates notes and lifts the wrapped ones an octave |
| Voicing | octave shift and voice-leading by whole-chord octave moves |
| Progressions | I–V–vi–IV, ii–V–I, i–VI–III–VII, I–vi–ii–V, i–iv–V–i, plus a seeded progression |
| Deterministic seed | `gen_harmony_seeded` takes `cfg->seed` |
| Simultaneous events | `gen_harmony_progression` pushes one `gen_event` per chord tone |

`GEN_CHORD_MAX_NOTES` is 5. A native Min 13th is outside that buffer.

Native ids that line up with a portable quality:

| Portable quality | Native id |
|---|---|
| Major / major triad | 3 Major |
| Minor | 4 Minor |
| Sus2 | 7 |
| Sus4 | 8 |
| Maj7 | 11 |
| Min7 | 12 |
| Dom7 | 14 |

The portable dominant ninth does not match native Maj 9th or Min 9th. Dim, aug, sixths, MinMaj7, 11ths, and 13ths exist only on the device. Inversions exist only in the portable engine.

Measured Thumb size, this tree, `make thumb-size` (`-Os`, text + rodata + data): `chords.o` is 1,376 bytes. The G5E note of 1,429 was the earlier measurement. At `-Oz` the public functions are `gen_chord_build` 282, `gen_chord_diatonic` 276, `gen_chord_voice_lead` 256, `gen_harmony_progression` 282, `gen_harmony_seeded` 92, and five progression fillers at 28 each.

Path B does not need those functions. The native id is the chord. Linking the object would pull `gen_buf_push`, the RNG, and scale helpers so the host engine can emit several pitches the step cannot store.
