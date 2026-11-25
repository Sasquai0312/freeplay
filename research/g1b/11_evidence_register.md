# 11 — Evidence Register

| ID | Claim | Class | Artifact |
|---|---|---|---|
| G1B-E001 | Stock PPF SHA match | VERIFIED_HOST | `/workspace/PlayPlus_v1.3.0.ppf` |
| G1B-E002 | Baseline vdev screen/slicer/tap/emu | VERIFIED_VDEV | `/tmp/g1b-*.log` |
| G1B-E003 | functions.tsv stub = staging names only | VERIFIED_SOURCE | `cfw/vdev/boot.py` |
| G1B-E004 | Tracks 0–7 sample / 8–15 ext MIDI default | VERIFIED_VDEV | `fixtures/track_classification.json`, mono harness |
| G1B-E005 | Instrument ptr 0 discriminates MIDI vs assigned | VERIFIED_VDEV | session buffer dump |
| G1B-E006 | Bank toggle Shift+button 2 | VERIFIED_VDEV | session |
| G1B-E007 | MIDI note = param 6 R/W | VERIFIED_VDEV | UI hold+knob + API |
| G1B-E008 | Velocity = param 0 R/W + clamp | VERIFIED_VDEV | UI “Velocity” + clamp vs sample |
| G1B-E009 | Duration = param 1; /60 steps; max 3840 | VERIFIED_VDEV | UI “Note Length” + clamp |
| G1B-E010 | Four-note sequence fixture | VERIFIED_VDEV | `fixtures/mono_harness_results.json` |
| G1B-E011 | Neighbor track preserved | VERIFIED_VDEV | Test B |
| G1B-E012 | Polyphony not verified | UNKNOWN | `05_polyphonic_validation.md` |
| G1B-E013 | Soft Undo MIDI UI not observed | OBSERVATIONAL | `06_transaction_and_undo.md` |
| G1B-E014 | `fw_undo_push` callable | VERIFIED_VDEV | session |
| G1B-E015 | USB MIDI TX not entered (dev=0) | VERIFIED_VDEV | `07_midi_output_observability.md` |
| G1B-E016 | Cave 13719 / Ext 35914 | VERIFIED_HOST | `link.map` |
| G1B-E017 | Pitchbend param 12 MIDI | VERIFIED_VDEV | hold+Pitchbend |
| G1B-E018 | Chance param 18 / Repeat param 16 | VERIFIED_VDEV (idx); scale INFERRED | hold knobs |
| G1B-E019 | Channel / step CC for production | UNKNOWN / INFERRED | not enabled |

## Diagnostics (reproducible, not firmware)

- `diagnostics/midi_probe.py` — exploratory phases  
- `diagnostics/midi_mono_validate.py` — acceptance harness  
- `fixtures/*.json` — captured observations  

Proprietary `.ppf` / `cfw/build` / `rebuild/` remain gitignored.
