# 06 — Undo and Transaction Integrity

## Mechanisms (FreePlay source — unchanged)

| API | Role | Class |
|---|---|---|
| `cfw_edit_begin(ctrl, label)` | `fw_undo_push` + dirty flags | VERIFIED_SOURCE |
| `cfw_edit_lock` / `unlock` | Nested audio-IRQ off | VERIFIED_SOURCE |
| `fw_step_toggle` | Occupancy | VERIFIED_SOURCE + VERIFIED_VDEV (MIDI) |
| `setParam` | Locks | VERIFIED_VDEV (MIDI note/vel/dur) |
| `cfw_refresh_pads` | LED refresh | VERIFIED_SOURCE |

Proven sample recipe (Slicer) remains the template:

```
cfw_edit_begin(ctrl, "Genesis");
for each mutation:
  lock; toggle/setParam; unlock;
cfw_refresh_pads(ctrl);
```

## MIDI experiments

| Test | Result | Class |
|---|---|---|
| Soft-key Undo after MIDI pad place | Soft keys stayed Live Rec / Play / Perform; Undo label **not** observed; taps on candidate buttons did not clear the step | Observational — **not verified** |
| `fw_undo_push` callable from vdev | Succeeded (label in DTCM scratch) | VERIFIED_VDEV (call only) |
| Single-note edit + undo restore | Not experimentally closed via soft key | Open |
| Four-note batch as one undo | Not experimentally closed | Open — **do not claim atomic** |
| Unrelated tracks preserved during writes | Verified in Test B | VERIFIED_VDEV |
| Invalid target rejection | Host-side (sample vs MIDI rules) | Required in contract |
| Cancel before apply | Feasible if no `begin`/no writes (preview path) | INFERRED from Slicer pattern |
| Edit lock release after failure | Lock depth 0 after probes; balanced lock discipline required | VERIFIED_SOURCE practice |

## Failure injection

No controlled mid-edit crash injection performed. Do **not** describe Genesis Apply as atomic unless a future experiment restores a partial MIDI edit via one Undo.

## G3 guidance

1. Always `cfw_edit_begin` before MIDI mutations.  
2. Validate track/step before setters.  
3. On failure after begin: prefer documenting “Undo may restore if stock ring captured the snapshot”; do not promise soft Undo until UI-proven on MIDI.  
4. Capability `NATIVE_UNDO` = **mechanism available** (source + push callable), not “soft Undo verified for MIDI”.
