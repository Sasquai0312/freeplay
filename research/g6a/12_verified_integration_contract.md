# Verified integration contract

G6B writes one chord id on an external MIDI step. It does not write extra notes.

## Field

| Item | Contract |
|---|---|
| Index | 10 |
| Accessor | `getParam` / `setParam` on the step. No direct byte store |
| Type | signed halfword at step offset 18 |
| Range | 0..20. Send a value in that range. The accessor clamps anything else and does not report an error |
| 0 | None. On an occupied step this is a locked value, not an unlocked step |
| Off step | `getParam` returns no value while the lock bit is clear. Toggle is what clears a step |
| Root | parameter 6, the existing note |
| Velocity | parameter 0, one value for the step |
| Duration | parameter 1, one value for the step |
| Chance, repeat, microtiming | unchanged, and not per voice |
| Track | `type == 1 && inst == 0`. Do not write index 10 on a sample track and call it a chord |
| Length | 16, 32, or 64, using the existing gate. Reject any other step before `setParam`. No 128 |

## Snapshot

`gen_native_step` must gain one `s16` chord before any chord Apply. Restore writes that halfword back with `setParam(10, saved)`. A snapshot that omits it is not a complete restore.

`EMPTY_ONLY` does not gain a chord-shaped exception. Replacing an occupied region, including its chord id, stays on the confirmed `REPLACE_REGION` path.

The recovery lock from G5C.3 stays as it is. Do not describe the native undo buffer or a power cut as atomic.

## What G6B must not claim

- More than one stored pitch per step.
- An inversion, a custom voicing, or a per-voice velocity.
- That the emulator showed several MIDI note-ons.
- Physical polyphonic playback. That remains an owner check after G6B, on a build that has not been made yet.

## Mapping for the first lab

Use the native id, not the portable interval list.

| Lab quality | Id |
|---|---|
| None | 0 |
| Major | 3 |
| Minor | 4 |
| Sus2 | 7 |
| Sus4 | 8 |
| Major 7 | 11 |
| Minor 7 | 12 |
| Dominant 7 | 14 |

Further native names (power chords, dim, aug, sixths, 9ths, 11ths, 13ths) can be offered as ids. They do not require `chords.c`.
