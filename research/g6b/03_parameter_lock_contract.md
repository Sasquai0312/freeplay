# Parameter lock contract

Chord lock is bit 10 of the step lock word (bytes 0x26, 0x27, 0x28, and the low 7 bits of 0x29). `getParam(10)` returns a value only when that bit is set. A clear bit is unlocked: the call returns false and Genesis stores no id.

| State | Bit 10 | `getParam(10)` | Halfword |
|---|---|---|---|
| Never touched | 0 | false | ignored while unlocked |
| Toggled on | 1 | 0 | forced to locked None |
| `setParam(10, id)` | 1 | id 0..20 | that id |
| Clear-lock index 10 | 0 | false | left stale |

Clear-lock is vtable `+0x30` (`0x6010a870`). It clears one bit and does not clear the halfword. It works while the step is off and does not clear the note bit. Click and Shift+click do not unlock bit 10. Toggle off keeps the lock and the value. A later toggle on forces the chord id back to locked 0, so a restore of an off step must `setParam` while the step stays off.

Genesis packs the snapshot into the existing `gen_native_step` byte: bit 7 is the lock, bits 0..6 are the id. The step struct stays 8 bytes. The snap stays 528 bytes.

Restore:

- Locked id 0..20: `setParam(10, id)` after occupancy, note, and duration are put back. Toggle-on would have forced id 0, so the chord write is last.
- Unlocked: `setParam(10, 0)` then clear-lock index 10. The stale halfword is normalized to 0. Playback uses the lock bit (`0x6010afd4` loads the halfword only when the bit is set), so an unlocked stale halfword is not a sounding chord. The unlocked halfword is not byte-preserved.

Note, velocity, and duration locks that a failed write set on a previously unlocked step are cleared with the same clear-lock function when the snapshot said those fields were unread.

Velocity exception: a fresh toggle stores 7600 in parameter 0. `setParam(0, 7600)` clamps to 127, confirmed on the diagnostic image. That value cannot be restored through the accessor. Note, duration, chord id, chord lock, and on/off still return, and `genesis_midi_restore` returns `GEN_MIDI_ERR_RESTORE` (-11). The UI sets the recovery lock and does not retry Apply. A velocity previously written with `setParam` in 0..127 restores exactly, and that path returns success from restore (the diagnostic fail injection then returns -6).

Native cases: unlocked restore (case 6, bit 6 and bit 10 clear, note unread), locked None with velocity 100 (case 7), stock 7600 (case 7b, rc -11, note 70 and chord 0 back, velocity left at the new write), and an existing id (case 8, id 12 and note 70).
