# Playback evidence

| Class | Result |
|---|---|
| Native storage | verified |
| Native editing | verified, stock knob and `setParam` |
| Playback path reads the id | observed, as one index inside a generic flush |
| Expansion into several note-ons | not observed |
| Virtual USB MIDI dispatch | not observed |
| Physical polyphonic playback | not tested |

With the audio clock enabled, Play made `playing()` return 1. During that window the step's chord halfword was read by `0x6010b19a`.

That instruction is the type-3 arm of `0x6010afd4`. The function takes the step and a parameter index, checks the lock bit, and loads a signed halfword through the offset table at `0x20035ca0`. It stores the value at the caller struct `+0x14` and returns.

The caller is the loop at `0x6010b4ca`. The index runs from 0 while it is not `0x16`, and each index is passed through the step vtable slot `+0x50`. Index 10 is visited because every locked parameter is visited. In the probe that call for index 10 happened twice in 400 ms (`return address 0x6010b4f2`). The note byte at step+31 was read much more often than the chord halfword in an earlier memory trace (18 reads against 1).

No interval table was found in the stock image or in emulator RAM. `usb_midi_tx` at `0x600c6688` was not entered while the pattern played. The virtual device has no USB MIDI device, which matches the G1B finding. Stopping transport can enter that function and then fault on an unmapped pointer. That fault is the stubbed sender, not a captured note-on.

The owner's G5F report already says monophonic external MIDI playback and note release pass on the hardware. That report does not include chords. G6A does not extend it.

A later owner check, not this phase, is what would confirm that id 3 on note 60 sounds as C major.
