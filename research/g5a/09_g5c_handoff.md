# Handoff for G5B and G5C

G5A stops here. G5B (portable Evolution) and G5C were not started.

## Native contract G5B must respect

- Destination length is per track, `TRK_VT_LENGTH`, range 1–64. Do not invent a 128-step native pattern.
- Generate at most `min(requested, native_length)`. Default requested length stays 16.
- Pass events through `genesis_midi_apply_ex` / `genesis_midi_preview_build`. Do not write step parameters from a new editor.
- Timing stays `native_dur = round(ticks * 60 / ticks_per_step)` with `ticks_per_step` 480 in the UI.
- One note per step. No polyphonic Apply, no multi-track Apply, no channel write.
- If native length is not 16, 32, or 64 it is still editable when it is between 1 and 64. Check `genesis_midi_pattern_length` rather than the capability bits alone. The bits advertise the fixture lengths.
- A step index `>= plen` is an error even if `cfw_step_at` returns a pointer.
- Preview paging is a UI concern. Apply consumes the full event list the caller stored.
- Occupancy for “stale preview” must cover the whole destination, not 16 bits.
- Diagnostic fail stages 8 and 9 exist. Do not surface them on a control.
- Recovery latch is unchanged: `cfw_genesis_set_recovery` / `cfw_genesis_recovery_active`.

## API surface that moved

- Removed the UI-only requirement that pattern length equal 16.
- Added `GEN_MIDI_CAP_LEN16` (1<<16), `GEN_MIDI_CAP_LEN32` (1<<17), `GEN_MIDI_CAP_LEN64` (1<<18), `GEN_MIDI_DEST_MAX` 64.
- `genesis_midi_caps()` return value gained those three bits. Existing bit 0 is still monophonic.
- Version string is `1.3.0-FreePlay0.2a` so a device can be told apart from the v0.1 image.

Generator function signatures and `gen_event` are unchanged. G5B can add Turing, Markov, and transforms beside the current labs without retargeting the adapter, as long as each event’s step is inside the native length and the buffer reports `GEN_ERR_CAPACITY` instead of truncating.

## Not in this phase

Chord Progression, Mutation Lab, multi-track Apply, real-time arpeggiator, and programmatic Track Length writes. The knob remains the way a longer pattern is created.
