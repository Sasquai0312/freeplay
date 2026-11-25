# 02 — Existing Architecture Map

Pinned to FreePlay `5835b33745b5b6eca92466585f79ccb00fb28c0b`. Addresses below are Play+ 1.3.0 unless noted.

## Integration model

FreePlay does not replace Polyend firmware. It:

1. Compiles freestanding C into a **cave** (unused in-image space) and/or **ext** region (HEX records after image end).  
2. Redirects selected **hooks** (literal pool pointers, vtable slots, or `bl` sites) to FreePlay entry points.  
3. Usually calls the original stock function after CFW work.  
4. Guards ext calls with `cfw_ext_ok()` (magic + checksum).  

Evidence: `docs/DEVELOPING.md` L47–56; `cfw/targets/playplus-1.3.0/target.json`. Class: VERIFIED_SOURCE.

## Firmware entry points and hooks

Play+ registers **23 hooks** in `target.json` (`images[0].hooks`). Representative entries:

| Capability | Symbol | Hook kind | Addr / expect | Class |
|---|---|---|---|---|
| Root menu builder | `cfw_menu_build_root` | pointer | `0x600c9820` → was `0x600e9e59` | VERIFIED_SOURCE |
| Samples submenu | `cfw_menu_build_samples` | pointer | `0x600e9f30` | VERIFIED_SOURCE |
| Version string | `cfw_version` | pointer | `0x600c83f0` | VERIFIED_SOURCE |
| Menu screen hide/update | `cfw_menu_screen_*` | vtable | `0x600860dc` / `0x600860e0` | VERIFIED_SOURCE |
| App per-loop update | `cfw_app_update` | vtable slot 1 | `0x60078228` | VERIFIED_SOURCE |
| Input pad/button/knob/track/encoder | `cfw_in_*` | listener vtable | `0x6007b27c`… | VERIFIED_SOURCE |
| Pad LED set | `cfw_led_set` | vtable +4 | `0x60076ebc` | VERIFIED_SOURCE |
| Sample reader gain | `cfw_reader_gain` | `bl` | `0x60112e3e`, `0x60113126` | VERIFIED_SOURCE |
| Note-on Start/End/Attack/Decay reads | `cfw_tgt_note` | `bl` | four sites in `0x60110270`… | VERIFIED_SOURCE |
| CC Mapping channel builder | `cfw_menu_build_cc` | pointer | `0x600e76e4` | VERIFIED_SOURCE |
| Settings / File / Synth builders | `cfw_menu_build_*` | pointer | various | VERIFIED_SOURCE |

Cave sources (always present): `slice.c`, `sliceview.c`, `synthrand.c`, `screen.c`, `tap.c`, `delproj.c`.  
Ext sources (guarded): `export.c`, `startend.c`, `pages.c`, `colors.c`, `mod.c`, `lfoui.c`, `ccpreset.c`, `slicer.c`.

## Function pointer redirection

- Menu builders: literal-pool word replaced with CFW Thumb address.  
- Vtables: slot overwritten (menu screen, input listener, LED).  
- Call sites: `bl` retarget (`kind: "bl"`).  
Linker resolves via `symbols.ld` (`fw_*` stock + `cfw_*` CFW). Class: VERIFIED_SOURCE.

## Custom menu registration

Pattern (`slice.c`):

1. Hook builder calls stock builder.  
2. `fw_operator_new` + `fw_menu_submenu_init` / `fw_menu_action_init` / `fw_menu_option_init`.  
3. Optional reorder of root item list (LFO/Matrix inserted under Samples).  

Evidence: `cfw_menu_build_root` L157–200; `cfw_menu_build_samples` L254–259; `cfw_add_action` L227–233. Class: VERIFIED_SOURCE.

Root menu order on Play+ after FreePlay (when ext OK): File, Samples, **LFO**, **Matrix**, Synths, Scales, MIDI, Settings (`vdev_screen.py`, README). Class: VERIFIED_SOURCE.

## Screen rendering

- EVE display lists; logical **320×240**.  
- Display manager: 50 widget slots; compose high→low.  
- CFW panels: replace type-9 rect widget vtable; `tick` / `render` / `input`.  
- Stock list rows: 26 px, first row centre y=42, 6 visible (7 root).  

Evidence: `docs/FIRMWARE_NOTES.md` L74–76; `slicer.c`, `lfoui.c`, `sliceview.c`, `dlw.h`. Class: VERIFIED_SOURCE.

## Encoder and knob handling

Input listener slots (`FIRMWARE_NOTES` L16–24):

| Slot | Event | Stock / CFW |
|---|---|---|
| 0 | Pad | `cfw_in_pad` |
| 1 | Button | `cfw_in_button` |
| 2 | Knob turn | `cfw_in_knob` |
| 3 | Knob touch (“track” naming) | `cfw_in_track` |
| 4 | Encoder | `cfw_in_encoder` |

Buttons: 0/3 left keys, 6 encoder click / Menu, 7 Back, 8 Play, 10 Shift.  
Knobs 0–14 documented (Note, Chance, Tempo, Track Length, Repeat, …).  
CFW uses these for panels, Shift+encoder acceleration (`pages.c`), tap tempo (`tap.c`), Knob Touch filter (`screen.c`). Class: VERIFIED_SOURCE.

## Pad input and LED output

- Pad index = `row * 20 + col` (16 steps + 4 side). Col 19 = track select.  
- `cfw_led_set` wraps LED vtable +4 for folder colours / slicer preview.  
- `cfw_refresh_pads` walks pad + side refresh vtable methods after edits.  

Evidence: `FIRMWARE_NOTES` L19–20; `slice.c` L319–332; `colors.c`; `slicer.c` SELECT_COL 19. Class: VERIFIED_SOURCE.

## Track selection

- Stock firmware has **no global “current track”**.  
- FreePlay uses last step-pad pair in selection (`mod.c` L557–564) or select-pad focus masks in LFO/Slicer UI.  
- Slicer/LFO: select pad row → track.  

Class: VERIFIED_SOURCE (`FIRMWARE_NOTES` L21; `slicer.c` `set_track`; `lfoui.c`).

## Parameter editing

Step object vtable (`target.h` + `FIRMWARE_NOTES`):

| Offset | Method | Use in CFW |
|---|---|---|
| `+0x08` | set trig | Documented; emu harness | VERIFIED_SOURCE / notes |
| `+0x0c` `STEP_VT_IS_ON` | get trig on | Slicer, Slice, colors | VERIFIED_SOURCE |
| `+0x10` `STEP_VT_SET_PARAM` | setParam(idx, value) | Start/End/Sample writes | VERIFIED_SOURCE |
| `+0x18` `STEP_VT_GET_PARAM` | getParam(idx, &out) | Read locks | VERIFIED_SOURCE |
| `+0x3c` | clear | Documented, unused in CFW | VERIFIED_SOURCE (notes) |
| `+0x40` `STEP_VT_TRACK_TYPE` | 0 sample, 1 MIDI/synth | Filter sample-only edits | VERIFIED_SOURCE |

Sample param indices 0–15 documented (`FIRMWARE_NOTES` L35). Edit recipe: `cfw_edit_begin` → lock → setParam → unlock → `cfw_refresh_pads` (`slice.c` L311–332, `slicer.c` `place`). Class: VERIFIED_SOURCE.

## Clock and transport

| Symbol / field | Role | Class |
|---|---|---|
| `APP_SEQ_PTR` / sequencer singleton | Play buffer, playing flag | VERIFIED_SOURCE (`target.h`, `mod.c`, `tap.c`) |
| `SEQ_VT_PLAY_BUF` `+0x08` | Playing pattern buffer | VERIFIED_SOURCE |
| `SEQ_VT_PLAYING` `+0x28` | Playing? | VERIFIED_SOURCE |
| `SEQ_VT_SET_PLAYING` `+0x20` | Stop (usbdisk, RT target.h) | VERIFIED_SOURCE |
| `BUF_VT_SET_TEMPO` | Live tempo | VERIFIED_SOURCE (`tap.c`) |
| `fw_pattern_switch` | Queue pattern change | VERIFIED_SOURCE (`symbols.ld`) |
| `PM_PLAYING` / `PM_QUEUED` | Pattern manager | VERIFIED_SOURCE |
| `BUF_PLAYHEAD` | Track 1 step cursor | VERIFIED_SOURCE |

No CFW hooks into MIDI clock or note dispatch. Class: VERIFIED_SOURCE for fields; UNKNOWN for full clock graph.

## Project persistence

- Stock Save copies stock project files only.  
- FreePlay side files: `cfwSettings`, `cfwColors`, `cfwCC`, `cfwMod.<project>`.  
- Open project `/workspace`; saves under `/Projects/<name>/`.  

Class: VERIFIED_SOURCE (`FIRMWARE_NOTES` L82–84).

## MIDI functionality (existing)

| Feature | Path | Class |
|---|---|---|
| CC Mapping presets Load/Save | `ccpreset.c`, hook `cfw_menu_build_cc` | VERIFIED_SOURCE |
| Project CC table | `project + 0x201 + channel*8`, 32 channels | VERIFIED_SOURCE |
| USB composite (MIDI/MSC) notes in `usbdisk.c` | Incomplete / `#ifdef CFW_USB` style; not primary release path for Genesis | VERIFIED_SOURCE / INFERRED |
| MIDI note sequencing API | **None in CFW** | UNKNOWN / absent |

## Memory allocation

- `cfw_ram`: 256 B, layout-versioned, cleared by `cfw_ram_check`. `_Static_assert(sizeof <= 256)`.  
- Heap: `fw_operator_new` for menus, slicer state, mod state, pages.  
- Audio IRQ path: no alloc, no FS (`DEVELOPING.md` L68).  

Class: VERIFIED_SOURCE.

## Reusable without changing the underlying mechanism

Safe to reuse as-is for Genesis:

1. Menu add pattern (`cfw_add_action`, submenu init).  
2. Panel pattern (Slicer/LFO: tick/render/input, select pads, encoder, soft keys).  
3. `cfw_step_at` / track length (`TRK_VT_LENGTH 0x04`).  
4. `fw_step_toggle` + set/getParam under `cfw_edit_lock`.  
5. `cfw_edit_begin` / `fw_undo_push` + dirty flags.  
6. `cfw_refresh_pads` + toast.  
7. Track targeting via select pads.  
8. Seeded RNG pattern (`cfw_ram.rand` / synthrand xorshift).  
9. Ext guard (`cfw_ext_ok`).  
10. Host vdev test harness patterns.

Do **not** assume sample param indices apply to MIDI tracks without verification.
