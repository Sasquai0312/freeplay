# 04 — MIDI Parameter Map

**Status:** Incomplete — controlled differential discovery **BLOCKED** (no stock PPF / vdev).

## Method intended (not executed)

1. Disposable vdev pattern; known MIDI track/step.  
2. Baseline dump of step object (42 B) + getParam for indices 0..21.  
3. Change one native UI parameter; re-dump; diff.  
4. Repeat independent values; confirm neighbors unchanged.  
5. No brute-force setParam probing.

## MIDI parameter evidence table

| Parameter | Index / representation | Type | Range | Read | Write | Evidence | Confidence |
|---|---|---|---|---|---|---|---|
| MIDI track identity | Non-sample tracks (likely 8–15) use `step_vt_synth`; type==1 | — | — | track/step APIs | — | emu_slice; notes | INFERRED (coarse); fine ID UNKNOWN |
| Sample vs non-sample | `STEP_VT_TRACK_TYPE` 0 vs 1 | u32 | 0/1 | vt+0x40 | — | notes; colors/slice filters | VERIFIED_SOURCE |
| MIDI vs synth | — | — | — | UNKNOWN | — | — | UNKNOWN |
| Step occupancy | trig on/off | bool | — | `STEP_VT_IS_ON` | `fw_step_toggle` / set trig +0x08 | slice/slicer | VERIFIED_SOURCE (API); MIDI exercise BLOCKED |
| MIDI note | Sample Note idx **6** only | s16? | 0–127 product | getParam | setParam | sample notes VERIFIED; MIDI UNKNOWN | UNKNOWN for MIDI |
| Velocity | Product: Volume knob→Velocity | — | 0–127 | UNKNOWN | UNKNOWN | Public manual | INFERRED UI only |
| Note duration | Product: Pan→Note Length | — | 0.25–64 steps | UNKNOWN | UNKNOWN | Public manual | INFERRED UI only |
| MIDI channel | Per-track product UI | — | 1–16? | UNKNOWN | UNKNOWN | Manual | UNKNOWN |
| Polyphonic storage | — | — | — | UNKNOWN | UNKNOWN | — | UNKNOWN |
| Microtiming | — | — | — | UNKNOWN | UNKNOWN | — | UNKNOWN |
| Probability / Chance | — | — | — | UNKNOWN | UNKNOWN | — | UNKNOWN |
| Ratchet / Repeat | — | — | — | UNKNOWN | UNKNOWN | — | UNKNOWN |
| MIDI CC (project map) | `project+0x201+ch*8` | u8[8] | 0–127 | ptr read | write | ccpreset.c | VERIFIED_SOURCE (map only) |
| Step CC locks | — | — | — | UNKNOWN | UNKNOWN | — | UNKNOWN |
| Track length | track vt+4 | s32 | 1–64 | `track_len` | stock knob | slicer | VERIFIED_SOURCE |
| Pattern length | Per-track; pattern manager fields | — | — | Partial | UNKNOWN | target.h | PARTIAL / UNKNOWN |

## Sample-track reference (not MIDI)

From `FIRMWARE_NOTES` (VERIFIED_SOURCE for **sample** steps only):

0 Volume, 1 Pan, 2 Filter, 3 Resonance, 4 Bit Depth, 5 Overdrive, **6 Note**, 7 Delay, 8 Reverb, 9 Sample, 10 Start, 11 End, 12 Microtune, 13 Attack, 14 Decay, 15 Folder.

Harness indicates **22** descriptors (`n_params=22`) — meaning of indices 16–21 UNKNOWN even for sample, and MIDI remapping UNKNOWN.

## Explicit unknowns

All MIDI note/velocity/duration/channel/poly/chance/repeat/microtiming **storage indices** remain UNKNOWN. No speculative indices are asserted.
