# 02 — Hardware Control Mapping

Verified FreePlay panel conventions (LFO/Slicer family).

| Action | Control |
|---|---|
| Open Genesis | Menu → **Genesis** (Enter) |
| Exit Genesis | Back (from config); stock leave |
| Cycle lab | Top key (`Lab+`) |
| Apply placeholder | Bottom key → toast only |
| Generate | Click |
| Regenerate (new seed) | Shift + Click |
| Adjust value | Encoder turn |
| Next / prev param | Shift + Encoder turn |
| Return config from preview | Back or Bottom (`Edit`) |
| Target track | Select-pad column (bank via `MODE_BANK`) |
| Preview page | First 16 steps on pads / strip (page 0) |

## Avoided

Transport Play, pattern switch, and stock knob pages are swallowed while Genesis is open (`kind` 2/3) to avoid accidental edits. No new unverified intercepts.
