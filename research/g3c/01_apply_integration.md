# 01 — Apply Integration

## Path

`genesis_ui.c` → stored preview `gen_event[]` → `genesis_midi_apply_ex` (G3A).

Apply uses the **exact** events captured at Generate time (no silent regeneration).

## Preflight (before first write)

Recovery latch off; valid preview; event count match; track match; `type==1 && inst==0`; pattern length **16**; pattern id stable; destination occupancy fingerprint unchanged; note/vel ranges; `genesis_midi_validate_events`.

## Controls

| Action | Control |
|---|---|
| Apply (EMPTY_ONLY) | Bottom key on preview |
| Confirm REPLACE | Bottom again on confirm screen |
| Cancel REPLACE | Back |
| Drop preview | Back |

Apply unavailable without preview (`Generate first`), on unsupported tracks, and during recovery lockout.
