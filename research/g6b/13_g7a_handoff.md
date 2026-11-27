# G7A handoff

G7A was not started.

What G6B leaves in place:

- Stock chord ids 0..20 and the lock bit are the polyphony mechanism. There is still one root, one velocity, and one duration per step. No extra note slots were added.
- `chordprog.c` is the progression generator. `chords.c` is not in the firmware.
- Chords is lab 6. Preview ids sit on the UI object. Apply is `genesis_midi_apply_ex` with a sidecar. Other labs pass a null sidecar and write locked None when they replace a step.
- Mutation refuses a locked nonzero chord id. A later phase can teach Mutation to carry the sidecar through moves and pitch edits. Until then, do not describe Mutation as chord-aware.
- Stock toggle velocity 7600 cannot be restored with `setParam`. Any new writer that fail-restores a freshly toggled step must keep the recovery lock when restore returns -11.
- Unlocked chord restore normalizes the halfword to 0 after clear-lock. Do not expect the stale halfword to survive.
- Extension free is 4,752 bytes with a 2,048 reserve (budget 2,704). Cave free is 474. Static CFW RAM is full. Do not add cave hooks or static RAM for a small feature. Do not re-enable `-Oz` folding of `param_visible`; a bitmask in `0x10000`..`0xFFFFF` fails the firmware verifier.
- Playback of the stock qualities is not verified. The owner plan is `12_owner_hardware_test_plan.md`. Virtual MIDI transmission was not observed in G6A and was not re-probed here.
- Menu Enter still shares button 0 with Lab+. The 200 ms swallow covers the opening click only.

Do not flash the 0.2f candidate as part of starting G7A. Physical playback belongs to the owner pass above.
