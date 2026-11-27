# Native polyphony representation

Simultaneous notes are not stored as several note records, extra voice slots, or an interval list. They are stored as one chord id on the step that already holds the root.

| Item | Evidence |
|---|---|
| Parameter index | 10 |
| Storage | signed halfword, type 3 |
| Step offset | 18. The offset-table byte for index 10 is 14, and the value is read at `step + offset + 4` |
| Width | 16 bits. Id 3 is stored as `03 00` |
| Empty / off | `getParam` returns no value when the lock bit is clear |
| Occupied and empty chord | locked value 0, UI `None` |
| Valid range | 0..20. The UI and `setParam` both stop at 20 |
| Track type | external MIDI. Sample knob 7 is a different page |
| Read | step vtable `+0x18` → `getParam` at `0x6010acfc` |
| Write | step vtable `+0x10` → thunk `0x6010a729` → `setParam` at `0x6010ad88` |
| Offset table | RAM `0x20035ca0`, one byte per index |
| Type table | RAM `0x20001bb8`, 16 bytes per index |

`setParam(10, 21)` stores 20. `setParam(10, 255)` stores 0. `setParam(10, -1)` stores 0. The accessor clamps. It does not return an error code.

The clamp-table entry for index 10 says type 8, minimum 0, maximum 32767. That maximum is not the musical range. The observed range is 0..20, matching the 21-name formatter. Do not write 32767.

Searches for a second note slot found none:

- The step object is 42 bytes. The note is the signed byte at offset 31 (parameter 6).
- The stock image has no packed interval run `00 04 07` or `00 04 07 0b`.
- A scan of emulator RAM after boot found no `00 04 07` run either.

So a C major chord is not the three bytes 60, 64, 67. It is note 60 plus chord id 3.
