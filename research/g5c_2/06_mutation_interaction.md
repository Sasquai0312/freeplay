# Mutation interaction

`mutate.c` was not edited. Collision rules, operation order, and the refusal to write an occupied or protected step are the G5B behavior.

## Parameters on the page

Pit%, Rhy%, Vel%, GtVar, Ins%, Rem%, Rev, RotBy, OctSh, Quant, Amt, Attr, Step, Clr, Seed, and the shared pitch range. Density stays hidden. V.min and V.max still bound mutation velocity.

## Signed rotation

`mrot` stores an `int8_t`. The encoder clamps it to `-(len-1)..+(len-1)`. The value prints with a minus sign. Generate passes `(int8_t)mrot` to `rotate_steps`.

Host `test_rotate_and_steps` checks direction on a 4-step phrase, duration 480, so the span is 4 steps:

- +1 moves the note that started at tick 0 to tick 480
- −1 moves that note to tick 1440
- +4 is the identity

A 2-step phrase cannot show direction, because +1 and −1 are the same position. The UI still allows both signs up to `len-1`.

## Attribute protection

The stored value is still the 0..15 mask. Bits are pitch 1, rhythm 2, velocity 4, duration 8. The value text is `None`, `P`, `P+R`, `P+R+V`, or `P+R+V+D`. The user does not add the mask. The virtual-device run walked 0 → 1 (`P`) → 3 (`P+R`) → 15 (`P+R+V+D`) → 0 (`None`).

## Several protected steps

`pstep[8]` is the native 64-bit mask. On Mutation config, a step pad on the selected track toggles `page * 16 + column` when that step is inside the phrase. The Step parameter is a cursor, 0..len-1. Turning it sets the page to `step / 16`, so steps 16, 32, and 48 are reachable on longer patterns. LEDs for the current page light protected steps while config is showing. Preview LEDs still show generated notes.

Clr is momentary. It clears all 64 bits, toasts `Steps clear`, and does not generate.

The 64-step session protected step 16 (bit 0 of byte 2) from page 1. The 16-step session protected steps 1 and 4 from pads, then cleared them.

Select column is still track select. Pads do nothing to protection outside Mutation config, and nothing during a preview.

## Unsupported combinations and empty results

Reverse or rotate together with rhythm protection or any protected step still returns `GEN_ERR_UNSUPPORTED` before writes. The toast is `Rev/rot blocked`.

A full grid with only rhythm displacement, and nowhere to move, previews successfully and toasts `No note change` when every event matches the source. That is not `Generate failed`. A fully occupied pattern still cannot take an insertion or a displacement. The engine was not changed to create a destination.

Preview bars keep occupancy color and add an accent cap on a protected step. The preview does not write `evo_src` back to the pattern.
