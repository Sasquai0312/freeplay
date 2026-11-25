# 05 — Minimal MIDI Write Experiments

## Status

**Not executed.** Prerequisite: verified MIDI note write path via controlled discovery — blocked by missing `PlayPlus_v1.3.0.ppf`.

Class: BLOCKED.

## Planned diagnostic (for resume)

Isolated vdev-only script (not production CFW), after G1 param discovery:

1. Boot FreePlay image; disposable project.  
2. Ensure a MIDI track (not sample 0–7; not synth).  
3. Empty pattern / clear steps on that track within length.  
4. Insert note at step 0 via verified setParam + toggle.  
5. Read back; assert note.  
6. Modify; read back.  
7. Delete; confirm empty.  
8. Fixed fixture: steps 0,4,8,12 → notes 60,64,67,72 (0-based steps matching brief “1,5,9,13”).  
9. If velocity/duration verified, vary independently.  

## Explicit non-claims

- No audible MIDI output claimed (USB MIDI TX stubbed in vdev).  
- No speculative setParam index writes performed.  
- No production FreePlay behavior changed.

## Resume gate

Experiments may start only when §04 lists MIDI note index as VERIFIED_VDEV or VERIFIED_STATIC_ANALYSIS with corroborating vdev readback.
