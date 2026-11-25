# 08 — Non-Destructive Preview Feasibility

## Analogue

Slicer panel: plan on LEDs / screen, mutate only on Apply, Back leaves stock pads (`slicer.c`, `vdev_slicer` 38/38).

## Genesis preview (evaluation only — no production menu)

| Capability | Feasible? | Basis |
|---|---|---|
| Show generated step positions | Yes | LED plan bits like Slicer; no sequencer writes |
| Show note values | Yes | Screen text from generator buffer (host-side strings) |
| Show velocity / duration | Yes | Same |
| Highlight target MIDI track | Yes | Select-pad blink pattern (Slicer) + bank awareness |
| Cancel without modifying sequencer | Yes | Never call toggle/setParam until Apply |

## Diagnostic preview

Not required for G1B verification; monophonic R/W was validated via API + hold-knob locks. A diagnostic preview panel is optional and must stay out of production footprint estimates unless landed.

## Capability flag

`NONDESTRUCTIVE_PREVIEW` may be marked **feasible** (design), not “implemented”.
