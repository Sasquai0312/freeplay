# G6B executive summary

Primary decision: `CHORD_LAB_OFFLINE_VALIDATED_RESTRICTED`.

The Chord Progression Lab generates root notes and stock chord identifiers on an external MIDI track, previews them without writing, and Applies the stored preview through the existing native transaction. Chords is the seventh Lab+ entry. Live pattern lengths 16, 32, and 64 accept chord writes at the first step, the last step, and the page boundaries. A bad identifier or a bad pitch is rejected before any step changes.

Two restrictions keep the decision off a full validation:

- Mutation does not rewrite a phrase that already holds a locked chord other than None. Generate toasts `Has chords` and leaves the pattern alone. Locked None is a monophonic step and may be mutated. This is not full chord-aware Mutation.
- A stock toggle stores velocity 7600. `setParam` clamps that slot to 0..127, so 7600 cannot be written back. Note, duration, chord id, chord lock, and occupancy still return. The status is `GEN_MIDI_ERR_RESTORE` and the power-on recovery lock stays set. A velocity that was stored with `setParam` in 0..127 restores exactly.

Extension growth is 4,648 bytes. Free extension is 4,752 bytes, so the 2,048-byte reserve holds and the feature budget is 2,704 bytes. The 2,560-byte planning target was not met. The extra growth past the first link is the parameter-visibility function, left unoptimized so its index tests do not become a bitmask in the firmware verifier's low-address window. Cave use and static CFW RAM are unchanged.

Physical MIDI playback was not run. The candidate was not installed on hardware.
