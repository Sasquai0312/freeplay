# G2 — Executive Summary

**Phase:** Portable Generative MIDI Engine  
**Date:** 2026-11-25  
**Branch:** `cursor/g2-portable-engine-f3f4` (from G0 `8bfc72b`)  
**Primary decision:** `PORTABLE_ENGINE_VALIDATED_TARGET_SIZE_UNVERIFIED`

## Verdict

Genesis ships a portable C engine under `genesis/` with scales, Euclidean/probability rhythm, three melody modes, harmony/progressions, offline arpeggiator, transforms, deterministic RNG, host SMF export, 12 audition MIDI demos, and passing host tests. ARM Thumb `-Os` text+rodata for all engine modules sums to **≈6207 bytes** (under the 12 KiB *design* target at object level). **No FreePlay-linked size measurement** yet — firmware fit remains unverified until G3. No firmware integration in G2.

G1B native findings are documented as future constraints only; this branch does not include G1/G1B trees (branched from G0 as required).

## Stop

G2 ends here. Do not begin G3.
