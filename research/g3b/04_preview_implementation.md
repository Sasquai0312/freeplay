# 04 — Preview Implementation

## Contract

`genesis_midi_preview_build` → `gen_preview` (G3A). Read-only; no sequencer mutation.

## On-screen

- Title + MIDI ok / need MIDI.
- Event count, seed, track.
- Horizontal occupancy strip (green = hit).
- Note name under occupied steps where space allows.
- Hit count; `Apply unavailable`.
- Tiles: Lab / Edit.

## Pads

Temporary occupancy LEDs on the target track’s step pads; select-pad highlights target. On exit, stock pad presentation restores (`lit=0`, LED hook returns null).

## Invalidation

Track, lab, length, and any parameter change clear `prev.valid` and leave `ST_PREV`.
