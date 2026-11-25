# 03 — Timing Conversion

## Relationship (experimentally confirmed)

| Quantity | Value |
|---|---|
| Portable ticks per step (fixtures) | `GEN_TPS = 480` |
| Native duration units per step | **60** (G1B UI; reconfirmed via conversion tests) |
| Native step index | `tick / ticks_per_step` when exact |
| Native duration | `round(duration_ticks * 60 / ticks_per_step)`, clamp 0…3840 |
| Pattern length | `TRK_VT_LENGTH` (vdev default **16**) |

## Rules

1. Off-grid starts (`tick % tps ≠ 0`) → `GEN_MIDI_ERR_TIMING` (not silently discarded).
2. Zero duration → rejected.
3. Overflow of `duration_ticks * 60` → rejected.
4. Steps outside `[0, pattern_length)` → `GEN_MIDI_ERR_BAD_STEP`.

## Verified cases (firmware unit via vdev)

Quarter / half / full / multi-step, min≈1 unit, max 3840, first step 0, last step 15, off-grid reject.
