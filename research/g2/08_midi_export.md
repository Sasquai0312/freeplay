# 08 — MIDI Export

## Implementation

Host-only `tools/smf_export.c` + `include/genesis_smf.h`.

- SMF Type 1: tempo track + note track  
- Default TPQ 480; tempo µs/quarter configurable  
- VLQ delta times; note-on / note-off; end-of-track  
- Overlap policy: same pitch+channel — prior note-off truncated to the new note-on (no stuck notes)

## Validation

12 demos: `MThd` present; ≥2 `MTrk`; `mido` note-on count == note-off count.

## Firmware

Do **not** link SMF code into Play+ images.
