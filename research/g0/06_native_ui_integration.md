# 06 — Native UI Integration Feasibility

Pinned: FreePlay `5835b33`.

## Recommended integration pattern

Follow **LFO / Slicer panel** pattern rather than long action-only lists:

1. Add root (or submenu) entry **Genesis** via `cfw_menu_build_root` (or nested under a new submenu).  
2. Builder creates empty/near-empty submenu; menu screen update shows custom widget (`WMODE_*`).  
3. Input hooks route encoder / soft keys / select pads while panel active.  
4. Soft keys: Preview (if non-destructive) / Apply / section switch; Back exits.  

Evidence: `lfoui.c`, `slicer.c`, `slice.c` root injection. Class: VERIFIED_SOURCE.

## Proposed sections

| Section | Role | Analog |
|---|---|---|
| Arpeggiator | Offline arp params → buffer | Future; gated on Apply |
| Harmony | Root, scale, chord vocabulary | Note page Scale/Root stock; CFW shift lists |
| Sequence Lab | Melody generator | Synth Randomize intensity rows |
| Rhythm Lab | Euclidean / density | Slicer count encoder |
| Mutation Lab | Transforms on buffer | Deferred |
| Performance | RT transforms | Deferred (unknown MIDI TX) |
| Settings | Seed, track target, apply mode | Settings option rows |

For **v0.1**, collapse to one panel with encoder-navigated fields: Scale, Root, Octave, Length, Density, Velocity range, Seed, Track — plus Apply. Separate top-level section rows can wait until capacity and navigation are proven.

## Menu capacity

- Root currently: File, Samples, LFO, Matrix, Synths, Scales, MIDI, Settings (Play+).  
- Adding one **Genesis** row is consistent with LFO/Matrix injection (`ROOT_LFO_ROW` pattern).  
- Visible rows: 7 in root menu (`FIRMWARE_NOTES`). Extra rows scroll — acceptable.  
- `vdev_menutree.py` whitelists FreePlay-added rows; new rows need test updates.  

Class: VERIFIED_SOURCE.

## Screen and rendering constraints

- 320×240 EVE; avoid heavy bitmaps in hot path.  
- Prefer text + simple geometry (LFO scope, slicer strip).  
- Redraw from tick; menu update is not every frame (`DEVELOPING.md`).  
- Closing panel must refresh underlying widgets.  

Class: VERIFIED_SOURCE.

## Parameter navigation conventions

| Control | FreePlay convention | Genesis use |
|---|---|---|
| Encoder turn | Move focus / change value | Field select or value |
| Encoder click | Take / Apply | Edit field / Apply |
| Shift+encoder | Coarse steps | Octave / large density |
| Top/bottom keys | Mode / secondary | Section / Preview |
| Select pads (col 19) | Track focus | Target MIDI track |
| Back | Exit / release grab | Cancel without Apply |
| Toast | Short status | “Applied N steps” |

Class: VERIFIED_SOURCE (conventions).

## Pad-grid interaction

- Preview: light planned steps (slicer LED pattern).  
- Do not steal Play/Stop/Perform pads (`lfoui.c` comment: leave stock).  
- Avoid conflicting with step editing while panel open — match Slicer (preview on pads, edit deferred to Apply).  

Class: VERIFIED_SOURCE.

## Track targeting

- Select pad → track index (0–7 visible; bank for 9–16 via `MODE_BANK` / `CTRL_TRACK_PAGE`).  
- Verify `STEP_VT_TRACK_TYPE` and MIDI-vs-synth before Apply.  
- Show track number on panel (slicer does).  

Class: VERIFIED_SOURCE / UNKNOWN (MIDI check).

## Preview / Apply / Cancel / Undo

| Action | Mechanism |
|---|---|
| Preview | Non-destructive LED/screen plan from `GenBuffer` |
| Apply | `cfw_edit_begin("Genesis")` + writes |
| Cancel / Back | Discard buffer; no write |
| Undo | Stock undo after Apply (`fw_undo_push`) |

Class: VERIFIED_SOURCE for undo/edit; preview pattern VERIFIED_SOURCE.

## Conflicts with existing FreePlay

| Risk | Mitigation |
|---|---|
| Root menu crowding | Single Genesis entry; sub-panels inside |
| `cfw_ram` full | Heap state like slicer/mod (`fw_operator_new`) |
| Ext size 64K | Keep engine compact; measure after link |
| menutree tests | Whitelist new labels |
| Sample-only assumptions in helpers | New collect path with `sample_only=0` + MIDI filter |
| LFO WMODE exclusivity | Distinct `WMODE_GENESIS`; one panel at a time |

Class: VERIFIED_SOURCE (constraints) + INFERRED (mitigations).

## Consistency recommendation

Ship Genesis UI as **one root submenu + one interactive panel** cloned from Slicer control grammar (select pad, encoder, soft keys, Apply click, Undo via stock). Expand to seven named labs only after v0.1 Apply works.
