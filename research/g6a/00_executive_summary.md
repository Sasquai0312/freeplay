# G6A executive summary

Decision: `NATIVE_CHORD_PARAMETER_INTEGRATION_READY`

The Play+ does not store several MIDI notes on one step. A MIDI step stores one note, one velocity, one duration, and one chord type. The chord type is step parameter 10, a signed halfword at step offset 18, with the 21 stock names `None` through `Min 13th`.

The stock knob writes that field. `setParam` writes it and clamps it to 0..20. `getParam` reads it back. Writing the previous value restores it. Note, velocity, duration, the next step, and a sample track are left alone.

That is the integration path for a Chord Progression Lab. Genesis would choose a root and one of those 21 ids. The firmware, not Genesis, is what turns the id into sounding notes. This phase did not observe that expansion: no interval table was found, and the virtual USB MIDI sender was not entered while the pattern played. Physical polyphonic playback was not tested.

Paths that do not fit the evidence:

- Several note records per step. The step is 42 bytes and has one note byte.
- A free voice count up to eight. Nothing in the step stores a second pitch.
- Shipping the portable `chords.c` interval engine into the firmware. The native id already names the chord. The portable object measures 1,376 bytes and stays unlinked.

G6B can add the halfword to the snapshot and write it through `setParam`. It should not add static RAM, a cave hook, or a second note slot.
