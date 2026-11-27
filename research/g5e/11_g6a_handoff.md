# G6A handoff

G6A was not started. Nothing in this phase implements native polyphony, Chord Progression Lab, multi-track generation, or a real-time arpeggiator.

Measured objects that already exist:

| Object | Bytes | In the `0.2e` image |
|---|---:|---|
| Live MIDI adapter sections | 2,214 | yes, monophonic |
| `gen_arp` | 816 | yes, offline |
| `chords.c` compiled for this Thumb target | 1,429 | no |
| `genesis_ui.c` object | 11,657 | the lab UI is already linked |
| Cave free | 474 | no room for another large cave function |
| Extension budget after the 2,048 reserve | 7,352 | available for a future extension feature |

`chords.c` is 1,429 bytes of portable engine and is not linked. A Chord Progression Lab still needs a panel. That panel is not measured. The engine alone fits in the 7,352-byte budget. The panel has to be measured before anyone treats the feature as fitting.

The offline arpeggiator is 816 bytes and is already in the image. A real-time arpeggiator is a different program. It would need a hook. The cave has 474 bytes left, so a new cave hook has to be small or something now in the cave has to move back out.

There is no multi-track generator object. Apply and the snapshot path are one track. A multi-track generator is new code, not a copy of `do_apply` (864 bytes) plus `genesis_midi_apply_ex`.

Native polyphony is not a relink of the mono adapter. The live adapter is 2,214 bytes because each step holds one note. A polyphonic step model is new native work and needs its own size measurement. The 7,352-byte budget can hold a small adapter. It cannot be assumed to hold a second UI plus a new step format.

Further extension savings of the size of `cfw_pages_render` (4,928) would be required to reach the 8,192-byte preferred target without moving more code into the 474-byte cave remainder. That was not done. The knob-page renderer is existing FreePlay behavior.

The hardware-validated rollback images are unchanged: Alpha 1 `1.3.0-FreePlay0.2d` and v0.1 `1.3.0-FreePlay0.1b`. This candidate is `1.3.0-FreePlay0.2e`. It has not been installed on a Play+.
