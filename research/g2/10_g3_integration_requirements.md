# 10 — G3 Integration Requirements

Uses G1B verified native capabilities (reference only; not linked here):

| Field | Native |
|---|---|
| Note | param 6 (0–127) |
| Velocity | param 0 (0–127) |
| Duration | param 1 (0–3840; ≈60 units/step — provisional) |
| Occupancy | `is_on` / `fw_step_toggle` |
| Target track | `type==1 && inst==0` |

## G3 flow

1. UI collects Genesis settings (root, scale, density, seed, …).  
2. Call portable generators into a caller-owned `gen_buf`.  
3. Validate (`gen_buf_validate`, optional monophonic).  
4. Non-destructive preview (Slicer-style LEDs/text) — no native writes.  
5. On Apply: `cfw_edit_begin` → for each event convert `tick → step`, `duration_ticks → native units` → toggle/setParam → `cfw_refresh_pads`.  
6. Keep SMF exporter off-device.

## Timing conversion (provisional)

`native_dur = clamp(round(duration_ticks / ticks_per_step * 60), 0, 3840)` — re-verify in G3 against G1B findings before production use.

## Do not in G3 from this doc alone

Assume polyphony, batch Undo UI, or USB MIDI TX — still open per G1B.
