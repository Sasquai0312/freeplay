# FreePlay Genesis G1 — Executive Summary

**Phase:** G1 Native MIDI Sequencer Parameter Discovery  
**Status:** COMPLETE (source-only; experimental path blocked)  
**Primary decision:** `BLOCKED_FIRMWARE_ARTIFACT`  
**Date:** 2026-11-25

## Provenance

| Item | Value |
|---|---|
| Upstream | https://github.com/sloppyjoo/freeplay @ `5835b33745b5b6eca92466585f79ccb00fb28c0b` |
| Development fork | https://github.com/Sasquai0312/freeplay |
| G0 branch / commit | `cursor/g0-architecture-audit-f3f4` / `8bfc72b` |
| G1 branch | `cursor/g1-midi-param-discovery-f3f4` (from `8bfc72b`) |
| G0 decision preserved | `MIDI_INTEGRATION_RESEARCH_REQUIRED` |

## Verdict

G1 could not verify a native MIDI note read/write path because the owner-provided stock artifact **`PlayPlus_v1.3.0.ppf` was not present** in this environment (workspace, stores, secrets, Drive search). Without that file, FreePlay cannot be built or booted in vdev, so controlled differential MIDI experiments are impossible.

Source-only work completed:

- Sequencer access map refined (step size 42 B, dual step vtables, 22 param descriptors, sample tracks 0–7).  
- Memory headroom estimated from published `PlayPlus.patch.json`.  
- Integration contract drafted for G3.  
- Exact owner recovery instructions for supplying the firmware.

**No physical hardware changes. No proprietary `.ppf` published. No FreePlay production behavior changed. No Genesis musical algorithms implemented.**

## Acceptance vs outcome

| Criterion | Outcome |
|---|---|
| Upstream baseline preserved | Yes |
| Firmware identity verified | Expected SHA known; file **missing** |
| Virtual baseline recorded | Host-only tests; vdev **BLOCKED** |
| MIDI track discrimination | Coarse only (source); fine discrimination **UNKNOWN** |
| MIDI note R/W verified | **Not achieved** |
| Multi-step MIDI fixture | **Not run** |
| Undo characterized on MIDI | **Not run** |
| Polyphony characterized | Unresolved |
| Memory headroom documented | Yes (static from patch JSON) |
| No hardware changes | Yes |

## Decision

**`BLOCKED_FIRMWARE_ARTIFACT`**

Re-run G1 immediately after placing `PlayPlus_v1.3.0.ppf` at the repo root (gitignored). Do not start G3 Apply until MIDI note write is VERIFIED_VDEV.
