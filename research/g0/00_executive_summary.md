# FreePlay Genesis G0 — Executive Summary

**Phase:** G0 Architecture, MIDI Integration and Development Feasibility Audit  
**Status:** COMPLETE  
**Primary decision:** `MIDI_INTEGRATION_RESEARCH_REQUIRED`  
**Upstream pin:** `https://github.com/sloppyjoo/freeplay` @ `5835b33745b5b6eca92466585f79ccb00fb28c0b` (FreePlay 0.1 beta)  
**Date:** 2026-11-25

## Verdict

FreePlay is a solid, well-documented hook/cave/ext custom-firmware framework for Polyend Play+ 1.3.0 (and Play RT 1.6.0 virtual-only). Native **sample-track** sequencer edit paths (step get/set, length, toggle, undo, pad refresh) are **VERIFIED_SOURCE**. A portable generative engine and a Genesis menu can reuse those UI and edit recipes.

Critical gap for Genesis: **MIDI-track step parameter indices** (note, velocity, length/gate, channel, chance, microtiming, ratchets, polyphony) are **not** mapped in FreePlay source. Sample-track Note is index 6; MIDI remaps Volume→Velocity and Pan→Note Length at the UI level (public manual — **INFERRED** for firmware indices). Without stock `.ppf` artifacts, host builds and virtual Play tests that would discover those indices could not be run in this phase.

Therefore G0 does **not** authorize a device generative prototype. Next work should (1) obtain legitimate stock firmware for Play+ 1.3.0, (2) map MIDI step params in vdev, then (3) ship an offline engine + Apply path. Host-only engine scaffolding may proceed in parallel under a later phase.

## What was established

| Criterion | Result |
|---|---|
| Exact upstream baseline | Yes — single commit, no GitHub releases/tags |
| Architecture mapped | Yes — hooks, cave, ext, menus, input, steps |
| Native MIDI sequence access | Partial — track/step/length/undo **VERIFIED**; MIDI param map **UNKNOWN** |
| Arpeggiator alternatives | Evaluated — offline Apply most feasible; RT paths under-researched |
| Generation engine proposal | Yes — portable C, fixed buffers, host unit tests |
| Native UI options | Yes — root menu + panel pattern (Slicer/LFO) |
| Virtual testing | Documented; blocked here by missing stock firmware |
| Risks / recovery | Documented from upstream README/docs |
| Implementation roadmap | Yes — G1 MIDI param discovery → G2 offline Apply |

## Classification reminder

Findings use: `VERIFIED_SOURCE`, `VERIFIED_HOST_TEST`, `VERIFIED_STATIC_ANALYSIS`, `OWNER_REPORTED`, `INFERRED`, `UNKNOWN`, `BLOCKED`.

**OWNER_REPORTED:** device owner confirms FreePlay is installed and working on their physical Play+.

**No unauthorized firmware changes or hardware deployment occurred in G0.**

## Deliverables

All files under `research/g0/`:

- `00_executive_summary.md` (this file)
- `01_upstream_baseline.md`
- `02_architecture_map.md`
- `03_midi_sequencer_integration.md`
- `04_realtime_midi_feasibility.md`
- `05_generation_engine_design.md`
- `06_native_ui_integration.md`
- `07_virtual_testing.md`
- `08_risk_and_recovery.md`
- `09_implementation_roadmap.md`
- `10_evidence_register.md`
