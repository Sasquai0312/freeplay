# 02 — MIDI Track Discrimination

## Method

1. Read `STEP_VT_TRACK_TYPE` (+0x40) per track via live step vtable.  
2. Read step vtable pointer (`step_vt_sample` vs `step_vt_synth`).  
3. Read playing-buffer per-track instrument pointer at `buf + 0x98 + track × 0xBB4`.  
4. Compare native UI knob pages with bank select (`Shift+button 2` toggles `MODE_BANK`).

## Results (factory / disposable vdev project)

| Tracks | `TRACK_TYPE` | Step VT | Instrument | Classification |
|---|---|---|---|---|
| 0–7 | 0 | `0x20001ddc` sample | `fw_inst_vt_sample` (`0x600892e4`) | **Sample** |
| 8–15 | 1 | `0x20001d74` synth_or_midi | `inst == 0` | **External MIDI** (default) |

Class: **VERIFIED_VDEV**.

## Rules (for G3 validate)

```
if track_type == 0:
    → SAMPLE  (reject as MIDI target)
elif track_type == 1 and instrument_ptr == 0:
    → EXTERNAL_MIDI  (accept)
elif track_type == 1 and instrument_ptr != 0:
    → ASSIGNED_NON_SAMPLE (synth or other) — reject until synth path verified
```

A non-sample track is **not** automatically MIDI: `TRACK_TYPE==1` alone is insufficient; require `instrument_ptr == 0` for external MIDI on the default Play+ layout.

## UI corroboration

| Page | Sample track | External MIDI track (bank 1) |
|---|---|---|
| Note knob | Note / **Microtune** | Note / **Pitchbend** |
| Volume knob | Volume / Panning | **Velocity** / **Note Length** |
| Sample knob | Sample / Folder | Device / Patch (can show Synth N when browsing engines) |

Bank toggle: **Shift + button 2** sets `MODE_BANK` (VERIFIED_VDEV).

## Multi-index demonstration

Harness `midi_mono_validate.py` asserted kinds for all 16 tracks in one boot. Fixture: `fixtures/mono_harness_results.json`.
