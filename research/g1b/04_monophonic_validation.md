# 04 — Monophonic MIDI Write Validation

Harness: `diagnostics/midi_mono_validate.py`  
Fixture: `fixtures/mono_harness_results.json`  
Track: 8 (external MIDI). Indices: note=6, velocity=0, duration=1.

## Test A — Single note lifecycle

| Step | Result |
|---|---|
| Insert note 60 vel 100 | Readback (60, 100) |
| Modify to 67 | Readback 67 |
| Remove via `fw_step_toggle` | `is_on == false` |
| **Pass** | **Yes** |

Note: param locks may remain readable after occupancy off; Genesis must treat `is_on` as occupancy.

## Test B — Fixed 16-step sequence

| Step | Note | Velocity | Readback |
|---|---|---|---|
| 0 | 60 | 100 | OK |
| 4 | 64 | 80 | OK |
| 8 | 67 | 110 | OK |
| 12 | 72 | 90 | OK |

Unrelated steps off; sample track 0 occupancy unchanged. **Pass.**

## Test C — Boundaries

| Case | Result |
|---|---|
| Steps 0 and 15 | OK |
| Notes 0 and 127 | OK |
| Velocity 1 and 127 | OK |
| Duration 15 (~0.25) and 3840 (64.00) | OK |
| Velocity 5000 clamped to 127 | OK |
| Sample track rejected (`type==0`) | OK |
| MIDI track `type==1` | OK |
| Step 63 write | Memory accepts note (beyond default length 16) — **host must reject `step >= track_len`** before setters |

**Pass** with host-side range guards required for production.

## Native UI corroboration

Hold step + Note / Velocity / Note Length knobs changed the same indices (see `fixtures/ui_diff_log.json` / session logs). API and UI agree for P0 fields.
