# 03 — Native MIDI Parameter Discovery

## Method

Controlled differentials on disposable track 8:

1. Known state via `fw_step_toggle` + baseline `getParam(0..21)`.  
2. Hold step pad + turn one native knob (or one `setParam`).  
3. Diff `getParam`; confirm neighbors; restore.  
4. Cross-check sample track clamps where relevant.

No brute-force speculative setter sweeps beyond single-parameter differentials after a candidate was UI-identified.

## P0 — Required

| Semantic | Index | Type | Range | Units | Read | Write | UI evidence | Confidence |
|---|---|---|---|---|---|---|---|---|
| Step occupancy | — | bool | on/off | — | `STEP_VT_IS_ON` | `fw_step_toggle` | Pad on/off | **Verified R/W** |
| MIDI note | **6** | s16 lock | 0–127 | MIDI note | `getParam` | `setParam` / hold+Note knob | “C 5”, “F 4”, Note page | **Verified R/W** |
| Velocity | **0** | s16 lock | 0–127 (write clamp) | MIDI velocity | `getParam` | `setParam` / hold+Velocity | “79”, “110”, “Velocity” | **Verified R/W** |
| Note duration | **1** | s16 lock | 0–3840 | 1/60 step (display steps = value/60) | `getParam` | `setParam` / hold+Note Length | “0.27”, “2.77”, “Note Length”; max UI 64.00 ↔ 3840 | **Verified R/W** |
| Pattern / track length | track vt +0x04 | s32 | 1–64 | steps | `TRK_VT_LENGTH` | stock Track Length knob | length 16 default | **Verified read** (write stock) |
| MIDI track identity | type+inst | — | — | — | see §02 | — | Pitchbend / Velocity pages | **Verified read** |

Encoding notes:

- Velocity: MIDI writes clamp `>127 → 127`. Sample track param 0 accepts 5000 (volume units) — **type-dependent semantics on shared index**.  
- Duration: `3841` and `10000` clamp to `3840` (= 64.00 steps). `15` ≈ 0.25 steps.  
- Toggle defaults may install sample-like volume `7600` into param 0 before a MIDI velocity write; always set velocity explicitly after insert.

## P1 — Desirable

| Semantic | Index | Evidence | Confidence |
|---|---|---|---|
| Pitchbend (MIDI) | **12** | Hold+Pitchbend: `None→8`; sample uses Microtune at 12 | **Verified R/W** (MIDI) |
| Chance | **18** | Hold+Chance: `0→8`, UI “20% Chance” | **Verified R/W** (mapping scale TBD) |
| Repeat-related | **16** | Hold+Repeat: `0→5`, UI Repeat Type | **Verified R/W** (enum scale TBD) |
| MIDI channel | ? / track Device page | Param 9 defaults to 32; writable 0–34-ish; track Sample/Device page browses Synth engines — **channel field not isolated** | **Inferred / Unknown** for production |
| Polyphonic notes | — | Single byte at step+31 follows note; one `getParam(6)` | **Unknown / unsupported** for adapter |

## P2 — Exploratory

| Semantic | Index | Evidence | Confidence |
|---|---|---|---|
| MIDI CC step locks | **2** (CC1 candidate per CFW Fill comments) | `setParam(2,77)` readback 77 on MIDI step | **Inferred** — not UI-differentiated in G1B |
| Project CC map | `project+0x201+ch*8` | `ccpreset.c` | VERIFIED_SOURCE (map, not step locks) |
| Param 4 | 4 | MIDI write clamp 0–127; sample ignores writes (stays 16) | Unknown meaning |
| Param 9 | 9 | Default 32 on MIDI trigs | Unknown (not exposed as channel) |

## Explicit non-exposure

Inferred / unknown parameters (**channel, CC locks, Chance scale, Repeat enums, polyphony**) must **not** be enabled in a production Genesis adapter until upgraded to Verified R/W with UI corroboration.
