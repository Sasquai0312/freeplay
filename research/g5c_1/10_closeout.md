# G5C.1 closeout

Primary decision: **EVOLUTION_NATIVE_INTEGRATION_VALIDATED**

No secondary blocker. The limitations in section T are recorded and do not fail the acceptance list. No hardware was used. G5C.2 was not started.

## A. Phase status

Turing, Markov (four presets and learned mode), and Mutation are one Evolution Lab on the existing Genesis page. Preview does not commit. Turing state and a learned model advance only after `genesis_midi_apply_ex` returns `GEN_MIDI_OK`, and only once per preview. Portable tests, the new session tests, and the native suites listed in `07_test_results.md` passed. The linked image fits. Unrelated firmware bytes outside the cave and the hook slots are unchanged.

## B. Git

Branch `cursor/g5c1-evolution-lab-f3f4`. Parent `34dc6212bc3f8c6fca38deacca6f1e274e8f9314`. The commit hash is the commit that adds this directory. Push and pull-request status are in the phase summary next to this file.

## C. Frozen baseline

Parent is G5B `34dc621`. G5A `802c4de`, G3C `90ed2e6`, and G4A `fb907d4` are ancestors. Stock SHA-256 `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876`. No tag, no merge to `main`, no historical artifact overwritten.

## D. Native architecture

One page, `LAB_EVO` after Arp. State is appended to `struct genesis_ui` (`sizeof` 3,116). Session object `gen_evo` is 1,096 bytes inside that allocation. New sources: `genesis/engine/evolve.c`, `genesis/include/genesis_evo.h`, `genesis/engine/transform_gate.c`, `genesis/tests/test_evo_session.c`. Edited: `cfw/src/genesis_ui.c`, `target.json`, `target.h`, `verify.py`, `genesis/Makefile`, `genesis/engine/transform.c`. `genesis_adapt.c` was not edited.

## E. Turing

Register length, phrase length, evolution, lock, density (as rest probability), scale, root, pitch range, seed, and reset are wired to the existing API. Preview writes `proposed` and leaves `committed` in place. Reset calls `gen_evo_turing_reinit` and does not write the pattern.

## F. Markov presets

Stepwise, Motif, Bassline, and Exploratory. Preset builds use `scratch` and do not replace the published model.

## G. Learned Markov

The native read adapter fills `evo_src`. Learn uses caller-owned `pending` plus `scratch`. Failure leaves `model` and the previous `pending` bytes unchanged. Generation does not mutate the model. Empty source is `GEN_ERR_EMPTY` (“Empty source”), with no preset substitution. `gap_ticks` is 0; out-of-scale notes still break the chain.

## H. Mutation

G5A read plus `gen_mutate`. Order, collision, and protection are unchanged. Source and output do not alias. A full grid does not accept insertion or displacement. Unsupported protection combinations return an error and publish nothing.

## I. Preview and Apply

Phases EMPTY, VALID, STALE, ERROR, APPLIED. Apply rejects a phase other than VALID before any native write. Success calls `gen_evo_apply_result(e, 1)` once. A second call returns `GEN_ERR_PARAM`. Mode change clears the proposal flags. Native rollback remains the existing snapshot restore.

## J. Native compatibility

External MIDI `type == 1 && inst == 0`. Phrase 1–64. Monophonic. Step grid. `gen_event` 12 bytes. Duration through `gen_native_duration_ok`. Note 6, velocity 0, duration 1. xorshift32 only. 65 and 128 are rejected on the native session.

## K. Preservation

Preview reads the pattern and writes `evo_src` / the preview buffer. Committed Turing state changes on reset (explicit) and on successful Apply. A failed Apply, a cancel, and a stale preview leave it unchanged. The published Markov model changes only when a learned preview is applied successfully.

## L–O. Tests

Portable `make test`: pass. GCC ASan+UBSan: pass. Clang ASan: did not link, not counted. Native: G3A 42/42, G3B 17/17, G3C 43/43, G5A 60/60, screen 23/23, tap 6/6, slicer 38/38, mono 6/6, thumb fields 3/3, verify emu 43/43. Session tests passed. Details in `07_test_results.md`.

## P–R. Memory

Cave 13,874 / 16,384, left 2,510. Extension 59,684 / 65,536, left 5,852. Delta vs G5A +9,848. Static `cfw_ram` 256/256. Heap object 3,116. `gen_evo` 1,096. `do_generate` stack 1,120. `do_apply` stack 624. `gen_xform_mutate_pitch` stack 552. No `.bss`, no `.data`, no malloc.

## S. Candidate

`/tmp/fp-g5c1/PlayPlus_FreePlay_01.ppf` SHA-256 `1e302709a828e6dad8f4b635f5c5fac5a5583e08b63b6e04930f05be02a6f28c`. Copy for download: `/opt/cursor/artifacts/PlayPlus_FreePlay_01_g5c1.ppf` (same hash). Report `/tmp/fp-g5c1/PlayPlus_FreePlay_01.md`. Not committed.

## T. Limitations

- `genesis_midi_apply_ex` restores from a snapshot on write failure. Diagnostic stage `GEN_FAIL_DURING_RESTORE` commits and returns `GEN_MIDI_ERR_RESTORE`. That is not a single atomic pattern write.
- `verify.py` still exits non-zero on `0x10003`, `0xffa50`, and `0x6e400002`.
- Negative rotate and multi-step protection are in the API and not on separate knobs.
- A fully occupied phrase cannot take insertion or displacement.
- Clang ASan cannot link in this environment.
- The whole `test_tools.py` file was not run.
- Evolution gate is clamped to 4..480. Density is an inverted rest probability. Turing and Markov velocity is V.min.

## U. G5C.2

See `09_g5c_2_handoff.md`. One page, same collision rules, 5,852 extension bytes left, no hardware in that phase unless a later phase authorizes it.

## V. Hardware

No flash, no install, no device execution. This candidate is an offline link only.
