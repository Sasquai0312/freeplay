# 10 — Evidence Register

Upstream pin: `5835b33745b5b6eca92466585f79ccb00fb28c0b`  
Classes: VERIFIED_SOURCE | VERIFIED_HOST_TEST | VERIFIED_STATIC_ANALYSIS | OWNER_REPORTED | INFERRED | UNKNOWN | BLOCKED

| ID | Claim | Class | Pointer |
|---|---|---|---|
| E001 | Upstream HEAD is FreePlay 0.1 beta at `5835b33` | VERIFIED_SOURCE | `git rev-parse` on clone of `sloppyjoo/freeplay` |
| E002 | Workspace matches upstream at that commit | VERIFIED_HOST_TEST | `diff -rq` excluding `.git` |
| E003 | No GitHub Releases/tags | VERIFIED_SOURCE | GitHub API `list_releases` empty; `git tag` empty |
| E004 | Play+ requires stock 1.3.0; SHA256 pinned | VERIFIED_SOURCE | `cfw/targets/playplus-1.3.0/target.json` L4–5; `patches/PlayPlus.patch.json` |
| E005 | Play RT 1.6.0 supported in build; hardware untested | VERIFIED_SOURCE | README compatibility table; CHANGELOG |
| E006 | K66 incomplete | VERIFIED_SOURCE | README; CONTRIBUTING |
| E007 | MIT + CC BY-SA presets; no Polyend firmware in repo | VERIFIED_SOURCE | `LICENSE`; CONTRIBUTING rules |
| E008 | Owner reports FreePlay working on Play+ | OWNER_REPORTED | Phase brief §1 |
| E009 | Hook/cave/ext architecture | VERIFIED_SOURCE | `docs/DEVELOPING.md` L47–56; `target.json` |
| E010 | 23 Play+ hooks including menus, input, LED, audio gain, note-on reads, CC | VERIFIED_SOURCE | `target.json` hooks array |
| E011 | Cortex-M7 Thumb hard-float freestanding | VERIFIED_SOURCE | `target.json` cpu; DEVELOPING |
| E012 | Cave 16KiB, ext 64KiB, ram 256B | VERIFIED_SOURCE | `target.json` cave/ram/ext sizes |
| E013 | Step access via pattern view vtables | VERIFIED_SOURCE | `slice.c` `step_at`; `FIRMWARE_NOTES` L23–25 |
| E014 | Track length via track vt+4 | VERIFIED_SOURCE | `slicer.c` L36, L103–109; notes L25 |
| E015 | setParam/getParam/isOn/trackType offsets | VERIFIED_SOURCE | `target.h` L42–47, L69; notes L27–33 |
| E016 | Sample param index table 0–15 | VERIFIED_SOURCE | `FIRMWARE_NOTES` L35 |
| E017 | Track type 0=sample, 1=MIDI or synth | VERIFIED_SOURCE | `FIRMWARE_NOTES` L33 |
| E018 | `fw_step_toggle` creates/clears trigs | VERIFIED_SOURCE | `symbols.ld`; `slicer.c` place |
| E019 | Undo via `fw_undo_push` / `cfw_edit_begin` | VERIFIED_SOURCE | `slice.c` L311–317; `symbols.ld` |
| E020 | Edit lock around step writes | VERIFIED_SOURCE | `slice.c` `cfw_edit_lock`; notes L39 |
| E021 | Pad refresh after edit | VERIFIED_SOURCE | `slice.c` L319–332 |
| E022 | Select pad col 19 track targeting | VERIFIED_SOURCE | `slicer.c`/`lfoui.c` SELECT_COL; notes L19 |
| E023 | No stock global current track | VERIFIED_SOURCE | `FIRMWARE_NOTES` L21 |
| E024 | Screen 320×240; 50 widgets; list geometry | VERIFIED_SOURCE | `FIRMWARE_NOTES` L74–76 |
| E025 | Root menu LFO/Matrix injection | VERIFIED_SOURCE | `slice.c` `cfw_menu_build_root` |
| E026 | CC project table + presets | VERIFIED_SOURCE | `ccpreset.c`; notes L78–80 |
| E027 | Sequencer play buffer / playing / tempo | VERIFIED_SOURCE | `mod.c` L534–555; `tap.c` L46–52; `target.h` |
| E028 | Sample note-on hooks only (Start/End/Attack/Decay) | VERIFIED_SOURCE | `target.json` `cfw_tgt_note` bl hooks |
| E029 | No CFW MIDI note TX/RX hooks | VERIFIED_SOURCE | hooks list absence; architecture search |
| E030 | USB MIDI TX address known to vdev stub | VERIFIED_SOURCE | `hle.py` PLAYPLUS_130 |
| E031 | Preview-before-apply pattern (Slicer) | VERIFIED_SOURCE | `slicer.c` header comments; plan/LEDs |
| E032 | Recovery to stock firmware procedures | VERIFIED_SOURCE | README “Going back…” |
| E033 | vdev capabilities and limits | VERIFIED_SOURCE | `vplay.py`; DEVELOPING L70–92 |
| E034 | Host Thumb unit tests pass | VERIFIED_HOST_TEST | `test_tools.py TestThumbFields` |
| E035 | Stock PPF absent → PPF/vdev/build blocked | BLOCKED | filesystem; test errors |
| E036 | MIDI UI Velocity/Length remap (product docs) | INFERRED | Polyend manual / MusicRadar summaries |
| E037 | Chance/Repeat/Microtiming product features | INFERRED | Polyend Play manual |
| E038 | MIDI step param firmware indices | UNKNOWN | not in FreePlay sources |
| E039 | Polyphonic step packing | UNKNOWN | — |
| E040 | MIDI vs synth discrimination beyond type==1 | UNKNOWN | — |
| E041 | Native redo API | UNKNOWN | UI string only (`vdev_startend.py`) |
| E042 | Pattern duplication API | UNKNOWN | — |
| E043 | Real-time arp Approaches B/C interfaces | UNKNOWN | — |
| E044 | Approach A offline Apply mechanism | VERIFIED_SOURCE | slicer edit recipe |
| E045 | Approach A MIDI payload readiness | UNKNOWN | depends E038–E040 |
| E046 | `cfw_ram` full; heap for larger state | VERIFIED_SOURCE | `slice.h` assert; DEVELOPING L54 |
| E047 | Patch JSON publishes FreePlay bytes only | VERIFIED_SOURCE | `patches/*.patch.json`; DEVELOPING L120 |
| E048 | clang freestanding Thumb smoke compile OK | VERIFIED_HOST_TEST | local compile |
| E049 | Python deps (unicorn/capstone/…) install OK | VERIFIED_HOST_TEST | pip install |
| E050 | LinkerAlone slice.c fails missing `cfw_ext_ok` | VERIFIED_HOST_TEST | `TestLinkerAgainstLld` failure mode |

## Missing artifacts explicitly reported

1. `PlayPlus_v1.3.0.ppf`  
2. `stock_firmware/PolyendPlay_v1.6.0.ppf`  
3. Built FreePlay images under `cfw/build/` / `cfw/release/`  
4. vdev snapshots / MIDI step memory dumps  
5. Hardware MIDI capture logs  

## Policy compliance (G0)

| Restriction | Status |
|---|---|
| No flashing / modifying physical hardware | Observed |
| No deploying modified firmware | Observed |
| No changing FreePlay functionality | Observed (docs only) |
| No speculative firmware addresses invented | Observed (only cited existing) |
| No publishing proprietary firmware binaries | Observed |
| No Genesis feature implementation | Observed |
