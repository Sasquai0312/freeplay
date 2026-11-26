# Native length semantics

Evidence class: virtual Play+ running the G5A image, plus the stock Track Length control. Not a hardware session.

## Meanings

| Term | Where it lives | What it is |
|---|---|---|
| Track length | `TRK_VT_LENGTH` at track vtable +0x04 | Step count for that track. Integer 1–64. |
| Pattern | Pattern view, `sel+0x5c` | Holds 16 tracks. It does not have one shared length. |
| Active steps | Steps `0 .. length-1` | The destination Genesis may edit. |
| Step object past length | `cfw_step_at` may still return an object | Not a valid destination. Step 64 at length 64 reads as on/note 0 and is rejected by `genesis_midi_read_step`. |
| Page | 16 pads (`LED` columns 0–15) | Display page. Not capacity. `NUM_STEPS` is already 64. |
| Page index | Genesis UI `page` | `step = page * 16 + column`. |
| Playback position | Not used by Apply | Not measured in this phase. |

Length is a step count, not a timing unit and not an encoded enum. The same pattern can mix lengths: raising track 8 left track 9 at 16.

## How length changes

Knob 10 is Track Length (Play Mode shares that screen). The value shows “~” until a track is selected. Shift+button 2 sets bank 1, the select pad (row `track % 8`, column 19) selects the MIDI track, then knob 10 steps the length by one. From 16, 48 steps reach 64. One more step stays at 64. 65, 96, and 128 all stop at 64.

`genesis_midi_pattern_length` returns that count for an external MIDI track and an error otherwise. It already rejected values outside 1–64 before this phase. The production 16-step ceiling was the UI constant `PROD_PLEN`, the 16-bit occupancy mask, and the pad map, not the sequencer.

G3B’s 32-step experiment only scanned for a setter symbol. It did not turn knob 10 after selecting a track. That negative result is superseded.

Genesis does not write Track Length. Generating into a longer pattern that already exists is the supported path.
