# G5A — Extended native pattern length

Decision: **EXTENDED_PATTERN_SUPPORT_VALIDATED**

Genesis can generate into an existing external-MIDI track whose native Track Length is 1–64 steps. 32-step and 64-step destinations were applied, read back, and restored on the virtual Play+. 128 steps is not a native length. The hardware-validated 16-step path is unchanged: requested length still defaults to 16, and a 16-step track still caps generation at 16.

No firmware was installed on hardware. The v0.1 image `5dc172f7…4f26` was not modified. There is no `genesis-v0.1-hardware-validated` tag.

## What changed

- Production Apply no longer requires pattern length 16. The live `TRK_VT_LENGTH` is the destination limit, and that limit tops out at 64.
- Preview is paged 16 steps at a time. The encoder moves the page. Apply writes the whole stored preview.
- Occupancy tracked for a stale preview covers every step of the destination, not only the first 16.
- Capability bits `GEN_MIDI_CAP_LEN16`, `LEN32`, and `LEN64` are set. There is no 128 bit.
- Version string on this image is `1.3.0-FreePlay0.2a`.

## What did not change

- Generators, timing (`round(ticks × 60 / 480)`), external-MIDI identity (`type == 1 && inst == 0`), and the note/velocity/duration parameter indices.
- EMPTY_ONLY and REPLACE_REGION. Both were re-checked at 32 and at 64.
- Static CFW RAM. Linked cave size is still 13874 bytes. The extension grew by 508 bytes (49836 / 65536).
