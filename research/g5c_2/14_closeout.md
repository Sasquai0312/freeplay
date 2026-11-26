# G5C.2 closeout

## A. Phase status and primary decision

`EVOLUTION_UI_OFFLINE_VALIDATED`

The Evolution page on the virtual Play+ keeps Turing, Markov (four presets and Learn), and Mutation on the existing lab after Arp. Preview and Apply follow the G5C.1 session. Restore failure locks further Apply until the track is re-read. The linked image stays inside the code cave, the extension reserve, and the full static RAM block.

This decision is offline UI validation. It is not a hardware-readiness claim.

## B. Git provenance

| Item | Value |
|---|---|
| Work branch | `cursor/g5c2-evolution-ui-f3f4` |
| Base | `cursor/g5c1-evolution-lab-f3f4` |
| Start HEAD | `5aadd94b997e441f5521f86beaa765de26233f24` |
| Integration commit | `4df7541e327dafe4b5ce8329730dd5e080f19d7f` |
| G5B | `34dc6212bc3f8c6fca38deacca6f1e274e8f9314` |
| G5A | `802c4de2fca77767d63ce7e5dd81a6b7b91030bb` |
| Version string | `1.3.0-FreePlay0.2c` |

No merge to `main`. No release tag. Historical research trees were restored after the harnesses rewrote them.

## C. Frozen baseline verification

Stock SHA-256 `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876`.

G5C.1 candidate SHA-256 `1e302709a828e6dad8f4b635f5c5fac5a5583e08b63b6e04930f05be02a6f28c`.

Host `make test` on the untouched tip passed, with `sizeof gen_evo=1096`. Portable generators were not edited. Detail is in `01_baseline_verification.md`.

## D. Interface architecture

One `LAB_EVO` page. Top still walks Melody, Euclid, Arp, Evo, Melody. The three engines are `evo_mode` values and share `adjust`, `value_str`, render, and input. Config shows the mode, phrase length, native length, the focused parameter, and a mode-specific line. Preview reuses the 16-step occupancy strip and pages with the encoder. `03_ui_architecture.md`.

## E. Physical control mapping

Display is the existing 320×192 panel. Top cycles labs. Bottom applies. Click generates. Shift+Click reseeds and generates. Back leaves from config, drops a preview, or returns from an error to config. Shift+Back while recovery is set re-reads the track. The encoder sets the value; Shift+encoder selects the parameter and drops a preview. Knobs are swallowed inside Genesis. Select-column pads choose the track. Step pads toggle mutation protection on the selected track during Mutation config. No long-press reaches Genesis. Bank (button 2) must be set before the lab opens. `02_control_inventory.md`.

## F. Turing interaction

Register length and phrase length are separate. Lock draws `LOCK`. Reset calls `gen_evo_turing_reinit` immediately, toasts `Register reset`, and writes no native notes. Preview does not copy proposed onto committed. A second preview with the same inputs matches. A successful Apply advances once. `04_turing_interaction.md`.

## G. Markov preset and learned interaction

Styles are Step, Motif, Bass, Expl, and Learn. The Learn line shows the preset name, or Pending, Learned, Error, or No model. An empty source toasts `No source notes`. A pending model is published only by `gen_evo_apply_result` after `GEN_MIDI_OK`. A preset does not replace Learn. There is no matrix editor and no on-screen degree count. `05_markov_interaction.md`.

## H. Mutation interaction

The existing probabilities, reverse, octave, quantize, and seed stay on the page. Collision rules and operation order are the G5B engine. Reverse or rotate with rhythm protection or any protected step toasts `Rev/rot blocked`. A successful mutation that copies the source toasts `No note change` and stays a preview. `06_mutation_interaction.md`.

## I. Signed rotation and protection accessibility

RotBy is signed, `-(len-1)..+(len-1)`, stored as `int8_t`. Host checks: +1 moves the tick-0 note to tick 480, −1 moves it to tick 1440, +4 is identity on a 4-step phrase. Attribute text is `None` or `P+R+V+D` while the stored mask stays bits 1, 2, 4, and 8. `pstep[8]` covers steps 0..63. Pads toggle the current page of 16. The Step cursor changes the page. Clr clears the mask.

## J. Preview visualization

Sixteen occupancy bars, note names, and encoder paging. Protected steps get a cap on the bar. Turing draws the committed register cells when `committed.ready` is set. Lengths exercised: 1, 16, 32, 64. The strip does not draw 128 steps. Drawing reads the preview. It does not write source events, generated events, or the native pattern.

## K. Preview and Apply lifecycle

Phases remain EMPTY, VALID, STALE, ERROR, APPLIED. `evo_bind` includes mode, configuration, mutation fields, `pstep`, pattern identity, and a live step re-read. Parameter edits and mode changes drop the proposal. A second Bottom without a new preview toasts `Generate first`. `07_preview_apply_hardening.md`.

## L. Restore failure handling

`GEN_MIDI_ERR_RESTORE` has two sources in the unmodified writer. Stage 7 (`GEN_FAIL_DURING_RESTORE`) commits the new notes and returns the error. A failure inside `genesis_midi_restore` cancels and returns the same code, so the destination is indeterminate from the code alone. The UI treats either case as “the pattern may have changed”: recovery is set, the preview is invalidated, Turing is not advanced, a pending model is not published, Apply is not retried, and the toast is `Pattern may differ`. Shift+Back re-reads the track before another generate is allowed. Unshifted Back leaves the lock set. Atomic native writes were not established.

## M. Runtime heap analysis

One `fw_operator_new` object. `sizeof genesis_ui` is 3,124 (G5C.1 was 3,116). The extra 8 bytes are `pstep`. `gen_evo` stays 1,096. No second allocation. The operator-new free-heap size is not a firmware symbol in this tree. A successful link is not a heap measurement. `08_runtime_memory_audit.md`.

## N. Worst-case stack analysis

Own frames are unchanged for `do_generate` (1,120), `do_apply` (624), and `gen_xform_mutate_pitch` (552). Estimated nesting, not a watermark: Turing preview about 1,344, Markov preset about 1,272, Learn about 1,192 then a separate preview, mutation preview about 1,832, Apply during validation about 2,136. No stack high-water was measured. The Play+ stack ceiling is not in this tree.

## O. Historical verifier literal classifications

| Literal | Class | Role |
|---|---|---|
| `0x10003` | Expected | Inclusive byte bound in `cfw_ext_ok` (`CFW_EXT_MAX/4` words) |
| `0xffa50` | Expected | EVE addend in `cfw_pages_render` for `TILE_TEXT_X` |
| `0x6e400002` | Expected | Packed UI defaults: scale 2, algorithm 0, velocity 64..110 |

All three are in the frozen G5C.1 image. The first two are also in G5A. They were added to `KNOWN_CONSTANTS` with the existing entries kept, and `literal_check.py` recomputes each formula. `verify.py` on this image exited 0. Emulated slice tests inside that process: 43/43. `09_firmware_verifier_audit.md`.

## P. Emulator screenshots and interaction tests

`g5c2_interact.py`: 76/76. Framebuffers under `research/g5c_2/screenshots/` (`01_melody.png` through `14_stock.png`). They are emulator output. `10_virtual_device_tests.md`.

## Q. Portable and native regression results

Host clang: three suites passed. Native, on a snapshot of this image: TestThumbFields 3/3, MIDI mono 6/6, G3B 17/17, G3A 42/42, G3C 43/43, G5A 60/60, screen 23/23, tap 6/6, slicer 38/38, verify emu 43/43. `11_regression_results.md`.

## R. Sanitizer results

GCC 13.3 ASan and UBSan: three host suites passed, exit 0. Clang `make host-asan` was not a pass; this toolchain still cannot link `libclang_rt.asan`.

## S. Linked firmware memory measurements

| Region | Span | Used | Capacity | Left |
|---|---|---:|---:|---:|
| Code cave | `0x6018ca40..0x60190072` | 13,874 | 16,384 | 2,510 |
| Extension | `0x60190b00..0x6019fb98` | 61,592 | 65,536 | 3,944 |
| Static `cfw_ram` | `0x20002700` | 256 | 256 | 0 |

Extension grew by 1,908 bytes from the G5C.1 used size of 59,684. Cave end is unchanged. No `.ARM.exidx`. No new `.data` or `.bss`. Verifier: 889 stock records changed (data and checksum only), 3,850 data records inserted, every other stock line byte-identical. File size 4,271,251 bytes.

## T. Remaining memory headroom

Extension left: 3,944 bytes, above the 2,048 reserve. Additional extension use is under the 3,804 preference. Cave left: 2,510, same as G5C.1. Static RAM left: 0. Heap grew by 8 bytes inside the one existing object. Stack headroom is unknown because the ceiling and the high-water mark were not measured.

## U. Firmware candidate hashes and artifact locations

| Artifact | SHA-256 |
|---|---|
| Stock `PlayPlus_v1.3.0.ppf` | `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` |
| G5C.1 | `1e302709a828e6dad8f4b635f5c5fac5a5583e08b63b6e04930f05be02a6f28c` |
| G5C.2 | `bff1c17fa56cdd4fc79e5b69a9ee5df4a85008a80cd6f13cfdc80f94d6c806ac` |

Build directory: `/tmp/fp-g5c2/PlayPlus_FreePlay_01.ppf`. Downloadable copy: `/opt/cursor/artifacts/PlayPlus_FreePlay_G5C2_offline.ppf`. The `.ppf` is not in git. Label: offline-tested, not hardware-validated. `12_linker_and_candidate.md`.

## V. Remaining limitations and blockers

- Stage 7 commits native writes and still returns `GEN_MIDI_ERR_RESTORE`. The UI cannot prove the destination is unchanged.
- Stack high-water is unmeasured. Target stack capacity is unknown.
- Free heap after `fw_operator_new` is unmeasured.
- Learn has no degree numeral and no matrix editor.
- With evolution 0, a phrase length that is a multiple of the register wraps to the same committed bytes as a fresh init. Reset still reinitializes; the interaction test uses phrase length 1 to see the bytes change.
- Full `test_tools.py` is blocked on the missing Play 1.6.0 stock image.
- Clang ASan did not link.

None of these removed a validated generator path. The 2 KiB extension reserve still holds.

## W. G5C.3 hardware-validation prerequisites

`13_g5c_3_handoff.md`. Boot a snapshot of this image. Leave `LINK_MAP` unset or point it at this image’s map. Confirm the four labs, the three engines, signed RotBy, named attribute bits, multi-step protection, one-shot Apply, and the restore lock. Do not change the native writer, the cave end, static RAM, or the portable 128-step entry points in order to make a hardware session pass.

## X. Physical hardware deployment status

No physical deployment. This image was not flashed, installed, or executed on a Play+ or any other device. Work stops at G5C.2.
