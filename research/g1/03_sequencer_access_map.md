# 03 — Native Sequencer Access Map

Pinned FreePlay `5835b33`. Play+ 1.3.0 addresses.  
**Do not assume sample-track behavior equals MIDI-track behavior.**

## Mechanisms

### 1. `cfw_step_at`

| Field | Detail |
|---|---|
| Location | `cfw/src/slice.c` L262–265; helper `step_at` L68–73 |
| Signature | `void *cfw_step_at(void *ctx, int track, int step)` |
| Preconditions | `ctx` = menu controller; selection at `ctrl+CTRL_SEL`; pattern view live |
| Returns | Step object pointer via `pv.vt[0](track)` → `trk.vt[0](step)` |
| Ownership | Firmware-owned pattern memory; do not free |
| Valid ranges | Track `0..NUM_TRACKS-1` (16); step `0..NUM_STEPS-1` (64); prefer `≤ track_len-1` |
| Side effects | None (read navigation) |
| Call sites | slicer, slice, sliceview, export, colors, startend |
| Class | VERIFIED_SOURCE |

### 2. `fw_step_toggle`

| Field | Detail |
|---|---|
| Location | `symbols.ld` `0x600de6f1`; used `slicer.c` L182 |
| Signature | `u32 fw_step_toggle(void *sel, int track, int step)` |
| Preconditions | Selection object; track/step in range |
| Behavior | Pad-equivalent: off→on with default step, or on→off |
| Ownership | Mutates pattern in place |
| Side effects | Changes trig occupancy; may install default param locks |
| MIDI applicability | API track-agnostic; MIDI behavior **not** exercised in CFW tests |
| Class | VERIFIED_SOURCE (symbol + sample use); INFERRED for MIDI |

### 3. `cfw_edit_begin`

| Field | Detail |
|---|---|
| Location | `slice.c` L311–317 |
| Signature | `void cfw_edit_begin(void *ctrl, const char *label)` |
| Behavior | `fw_undo_push(ctrl+CTRL_UNDO, label)`; sets dirty flags A/B |
| Preconditions | Valid `ctrl` |
| Side effects | One undo snapshot for subsequent mutations until next push |
| Call sites | slice apply, slicer place, startend, export |
| Class | VERIFIED_SOURCE |

### 4. `STEP_VT_SET_PARAM` / `STEP_VT_GET_PARAM`

| Field | Detail |
|---|---|
| Offsets | set `+0x10`, get `+0x18` (`target.h`) |
| Signatures | `void setParam(step, int idx, int value)`; `u32 getParam(step, int idx, s16 *out)` (low byte bool) |
| Preconditions | Valid step; param index meaningful for track type |
| Ranges | Sample indices 0–15 documented; harness `n_params=22` — indices ≥16 UNKNOWN meaning |
| Side effects | Writes locks; audio IRQ may read concurrently → use edit lock |
| Class | VERIFIED_SOURCE (API); sample indices VERIFIED_SOURCE; MIDI indices UNKNOWN |

### 5. Track selection

| Field | Detail |
|---|---|
| Mechanism | No global current track; CFW uses select pad col 19 or last selection pairs |
| Evidence | `FIRMWARE_NOTES` L21; `slicer.c` / `lfoui.c` SELECT_COL 19; `mod.c` L557–564 |
| Class | VERIFIED_SOURCE |

### 6. Track length

| Field | Detail |
|---|---|
| Mechanism | `track->vt[+0x04](track) -> s32` (`TRK_VT_LENGTH`) |
| Location | `slicer.c` `track_len` L103–109 |
| Range | Clamped to 1..64 |
| Class | VERIFIED_SOURCE |

### 7. Edit locking

| Field | Detail |
|---|---|
| API | `cfw_edit_lock` / `cfw_edit_unlock` (`slice.c` L75–87) |
| Behavior | Nested; first lock disables audio IRQ via `fw_audio_irq_off` |
| Class | VERIFIED_SOURCE |

### 8. Pad refresh

| Field | Detail |
|---|---|
| API | `cfw_refresh_pads(ctrl)` (`slice.c` L319–332) |
| Behavior | Mode vtable pad + side refresh for all cells |
| Class | VERIFIED_SOURCE |

### 9. Extension guards

| Field | Detail |
|---|---|
| API | `cfw_ext_ok()` — magic `CFWX` + checksum over ext region |
| Effect | Ext features (slicer, mod, …) skipped if append missing |
| Class | VERIFIED_SOURCE |

## Object layout notes (Play+ harness)

From `cfw/tests/emu_slice.py` PROFILES (VERIFIED_SOURCE of FreePlay’s own model):

| Item | Value |
|---|---|
| `step_size` | `0x2A` (42 bytes) — matches notes “42-byte engine step” |
| `track_size` | `0xBB4` |
| `step_base` | `0x134` (Play+) |
| `sample_tracks` | `range(0, 8)` |
| Step vtable sample | `step_vt_sample` |
| Step vtable non-sample | `step_vt_synth` |
| Param descriptors | `n_params=22`, `param_desc` at DTCM `0x20001bb8` (runtime) |

**Implication:** Tracks 8–15 use the non-sample step vtable. That set includes MIDI **and** Play+ synth tracks. Discrimination beyond `STEP_VT_TRACK_TYPE` / vtable pair is still UNKNOWN without vdev.

## Track type

| Method | Value | Meaning | Class |
|---|---|---|---|
| `STEP_VT_TRACK_TYPE` `+0x40` | 0 | Sample | VERIFIED_SOURCE |
| | 1 | MIDI **or** synth | VERIFIED_SOURCE (notes); fine split UNKNOWN |
