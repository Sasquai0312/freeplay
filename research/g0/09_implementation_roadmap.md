# 09 — Implementation Roadmap

Pinned baseline: FreePlay `5835b33`.  
**G0 decision:** `MIDI_INTEGRATION_RESEARCH_REQUIRED`  
Parallel-safe: host-only engine scaffolding may start once MIDI discovery is scheduled.

## Principle

Smallest path to genuine musical results: **offline Generate → Preview → Apply → Undo** on one MIDI track, not exhaustive RE of real-time MIDI.

## Phase G1 — MIDI parameter discovery (blocking)

**Objectives**

1. Obtain legitimate `PlayPlus_v1.3.0.ppf` (user-supplied; never commit).  
2. Build FreePlay image; boot vdev.  
3. Create MIDI track; write known notes via UI automation / step API probes.  
4. Produce verified table: param index → note, velocity, length, channel, poly packing.  
5. Discriminate MIDI vs synth (`STEP_VT_TRACK_TYPE` alone insufficient).  
6. Document in `research/g1/` with VERIFIED_HOST_TEST evidence.

**Exit:** MIDI Apply adapter can be specified without UNKNOWN note/velocity/length indices.

## Phase G2 — Portable engine + host tests

**Objectives**

1. Implement `genesis/` library: RNG, scales, Euclidean, random melody, merge.  
2. Host unit tests (no firmware).  
3. No CFW hooks yet.

**Exit:** Deterministic buffers for known seeds; CI green without `.ppf`.

## Phase G3 — Genesis v0.1 on device (Play+)

**Scope (from G0 brief, trimmed to proven path)**

- Native Genesis menu + single panel.  
- Scale-aware random melody + Euclidean rhythm.  
- Root, scale, octave range, pattern length, density, velocity range, seed.  
- Target selected track (MIDI only).  
- Preview via pads/screen.  
- Apply via undoable step writes.  
- Undo via stock.

**Out of scope for v0.1:** RT arp, Markov/Turing/CA, multi-track conductor, CC generation, Performance lab.

**Exit:** vdev test + owner hardware smoke on Play+.

## Phase G4+ — Expand generators

Priority order after v0.1:

1. Offline arpeggiator (Approach A).  
2. Mutation transforms.  
3. Multi-track with shared harmony.  
4. Weighted probability / Turing / Markov / Brownian / CA.  
5. Generative CC (project CC map already known).  
6. Revisit Approaches B/C only after MIDI TX/RX RE.

## First functional milestone recommendation

**Genesis v0.1** as defined in G3 is the correct first milestone.  
Do **not** start G3 Apply until G1 completes. G2 may overlap G1.

## Next-phase prompt objectives (suggested)

```
FreePlay Genesis — Phase G1
MIDI Step Parameter Discovery on Play+ 1.3.0

- Use owner-provided PlayPlus_v1.3.0.ppf (do not commit).
- Build FreePlay at pinned commit; run vdev.
- Map MIDI track step parameter indices and polyphony packing.
- Distinguish MIDI vs synth tracks.
- Deliver research/g1 reports + evidence; no generative UI yet.
- Still no hardware flash of modified firmware unless owner explicitly requests
  a separate hardware validation phase.
```

Optional parallel prompt:

```
FreePlay Genesis — Phase G2
Portable generation engine (host-only)

- Implement genesis/ C library per research/g0/05_generation_engine_design.md
- Host unit tests only; no target.h; no patching.
```

## Stop rule

G0 ends here. Do not auto-start G1/G2 without an explicit phase prompt.
