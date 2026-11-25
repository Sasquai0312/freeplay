# 03 — MIDI Sequencer Integration (Priority Research)

Pinned: FreePlay `5835b33`. Goal: can Genesis write generated events into the **native** sequencer on a **MIDI** track?

## Summary table

| Capability | Access? | Evidence | Class |
|---|---|---|---|
| Selected MIDI track | Partial — select pads / last pad; no stock “current track” | `FIRMWARE_NOTES` L21; slicer/LFO | VERIFIED_SOURCE |
| Current pattern | Partial — pattern manager / seq play buffer | `APP_PATMGR`, `SEQ_VT_PLAY_BUF` | VERIFIED_SOURCE (fields); full pattern ID API UNKNOWN |
| Pattern length | Per-track length via `track vt+4` | `slicer.c` `track_len` L103–109; notes L25 | VERIFIED_SOURCE |
| Individual sequencer steps | Yes — `cfw_step_at` | `slice.c` L68–73, L262–265 | VERIFIED_SOURCE |
| MIDI note numbers | Sample Note param idx **6** verified; MIDI track index **unverified** | notes L35; no MIDI-specific CFW use | VERIFIED_SOURCE (sample); UNKNOWN (MIDI) |
| Polyphonic note data | Not in CFW; manual describes multi-note MIDI steps | Public Play manual | UNKNOWN (firmware); INFERRED (product feature exists) |
| Velocity | MIDI UI remaps Volume→Velocity 0–127 | Public manual / MusicRadar | INFERRED; firmware idx UNKNOWN |
| Note duration / length | MIDI UI remaps Pan→Note Length (0.25–64 steps) | Public manual | INFERRED; firmware idx UNKNOWN |
| Microtiming | Knob 1 Microtune (sample); MIDI microtiming UNKNOWN | notes knobs L20 | UNKNOWN for MIDI |
| Probability / Chance | Knob 4 Chance/Action (UI); step storage UNKNOWN | notes; manual Chance/Action | UNKNOWN |
| Ratchets / Repeat | Knob 13 Repeat (UI); step storage UNKNOWN | notes; manual Repeat Type/Grid | UNKNOWN |
| MIDI channel | Per-track MIDI channel in product UI | Public manual | UNKNOWN (firmware field) |
| MIDI CC parameters | Project CC map verified; **step CC locks** UNKNOWN | `ccpreset.c`; notes L78–80 | VERIFIED_SOURCE (map); UNKNOWN (locks) |
| Step insertion | `fw_step_toggle` creates default-on step | `symbols.ld`; `slicer.c` place | VERIFIED_SOURCE |
| Step modification | `STEP_VT_SET_PARAM` | slicer/slice/startend | VERIFIED_SOURCE |
| Step deletion | `fw_step_toggle` off; clear `+0x3c` documented unused | slicer; notes | VERIFIED_SOURCE / notes |
| Pattern duplication | Not implemented in CFW | — | UNKNOWN |
| Native undo | `fw_undo_push` / `cfw_edit_begin` | `slice.c` L311–317 | VERIFIED_SOURCE |
| Native redo | Soft-key “Redo” seen in vdev UI text | `vdev_startend.py` | INFERRED (UI exists); CFW API UNKNOWN |
| Project save/reload | Stock save; CFW side-files separate | notes L82–84 | VERIFIED_SOURCE |

## Verified write path (sample tracks — template for MIDI)

Proven recipe used by Slicer (`slicer.c` `place` L168–199):

```
cfw_edit_begin(ctrl, "Label");          // undo + dirty flags
for each step:
  if on != want: fw_step_toggle(sel, track, step);
for each placed step:
  cfw_edit_lock();
  step->setParam(PARAM_*, value);
  cfw_edit_unlock();
cfw_refresh_pads(ctrl);
toast(...);
```

Supporting primitives:

- `cfw_step_at(ctx, track, step)` — pattern view → track → step.  
- `track_len` via `TRK_VT_LENGTH` (`0x04`).  
- `STEP_VT_TRACK_TYPE == 0` filters sample; `== 1` is MIDI **or** synth (notes L33).  

Class: VERIFIED_SOURCE.

## MIDI vs sample distinction

`STEP_VT_TRACK_TYPE` returns 1 for both MIDI and synth (`FIRMWARE_NOTES` L33). Genesis **must** further discriminate MIDI from synth before writing notes — method **UNKNOWN** in FreePlay (synth slots / instrument vtable comparison used for sample instruments in `mod.c`, not for MIDI).

Class: VERIFIED_SOURCE (coarse type); UNKNOWN (MIDI vs synth).

## Sample parameter map (do not assume for MIDI)

From `FIRMWARE_NOTES` L35 (VERIFIED_SOURCE for sample tracks):

| Idx | Sample meaning |
|---|---|
| 0 Volume | |
| 1 Pan | |
| 6 Note | |
| 9 Sample | |
| 10–11 Start/End | |
| … | |

Public product docs state that in MIDI mode Volume→Velocity and Pan→Note Length. That suggests **different semantic mapping** and possibly different indices or shared indices with reinterpretation — **must be confirmed** against firmware before Apply. Class: INFERRED (UI); UNKNOWN (storage indices).

## Preserving unrelated pattern data

Verified behaviors usable for safe Apply:

- `setParam` writes one lock; other locks on the step remain unless overwritten (INFERRED from getParam “false if not locked” + selective writes in slicer).  
- Slicer **replaces** trigs for layouts In time / Every step (`place` toggles all steps in length) — destructive to rhythm.  
- Layout “My trigs” only retargets existing trigs — preserves which steps are on.  

For Genesis Apply recommendations:

1. Prefer **fill empty / overwrite selected steps** modes over whole-track wipe.  
2. Always `cfw_edit_begin` so Undo restores prior state.  
3. Do not clear params you did not generate.  
4. Respect `track_len`; do not write past length.  

Class: VERIFIED_SOURCE for slicer modes; INFERRED for partial param preservation.

## Preview without committing

| Approach | Feasibility | Class |
|---|---|---|
| UI-only preview (grid LEDs + screen strip like Slicer) | High — already done without writing until Apply | VERIFIED_SOURCE (`slicer.c` plan/bits/LEDs) |
| Temporary sequencer write + Undo | Medium — needs verified undo depth / redo | VERIFIED_SOURCE (undo push); UNKNOWN (stack depth) |
| Shadow buffer playback without native sequencer | Needs RT MIDI out hook | UNKNOWN / likely heavy RE |
| Mute-track alternate pattern | Pattern APIs incomplete | UNKNOWN |

**Recommendation:** Genesis v0.1 preview = Slicer-style non-destructive plan visualization; Apply = one undoable write. Class: VERIFIED_SOURCE pattern.

## What is missing for MIDI Apply (blockers)

1. MIDI step parameter index table (note, velocity, length, channel, …).  
2. Polyphonic packing format (how multiple notes live in one step).  
3. MIDI vs synth track discrimination.  
4. Optional: chance/repeat/microtiming encodings if Genesis writes them.  
5. Stock `PlayPlus_v1.3.0.ppf` in the build environment to discover (1)–(4) via vdev/static analysis.  

Items 1–4: UNKNOWN. Item 5: BLOCKED in G0 host.

## Verified vs inferred vs unknown write paths

| Path | Status |
|---|---|
| Toggle trig on MIDI track via `fw_step_toggle` | INFERRED feasible (API is track-agnostic); not exercised on MIDI in CFW tests |
| `setParam(6, note)` on MIDI track | UNKNOWN |
| `setParam(volumeIdx, velocity)` on MIDI | UNKNOWN |
| Write poly chord to one step | UNKNOWN |
| Write only notes leaving other locks | INFERRED if setParam is per-index |
| Undo after Genesis Apply | VERIFIED_SOURCE mechanism; label string free |

## Conclusion

Native sequencer **infrastructure** for Genesis Apply is largely present and proven on sample tracks. **MIDI payload mapping is the critical unresolved dependency.** Until mapped under controlled vdev/static analysis with legitimate stock firmware, device Apply of melodies remains research, not implementation.
