# Regression results

Candidate image: `/tmp/fp-g5c3/PlayPlus_FreePlay_01.ppf`.

Virtual-device runs used boot snapshot `/tmp/fp-g5c3/PlayPlus_FreePlay_01.ppf.boot.snap` (23 s, `stopped: None`). `LINK_MAP` was unset. The chain log is `/tmp/g5c3-native.log`. It finished at 5:50 PM America/Phoenix on Thursday, November 26, 2026, exit 0.

## Passed

| Suite | Result | Evidence |
|---|---|---|
| Portable Genesis, evolution, session | pass | `make test` in `genesis/`, GCC 13.3, exit 0. `gen_evo` 1,096 |
| Same three suites, GCC ASan and UBSan | pass | `/tmp/g5c3-asan.log`. `gcc -fsanitize=address,undefined -O1` |
| Thumb field codec | 3/3 | `TestThumbFields`, `THUMB:0` |
| MIDI mono adapter | 6/6 | `MIDI:0` |
| G3B UI | 17/17 | `G3B:0` |
| G3A native adapter | 42/42 | `G3A:0` |
| G3C Apply workflow | 43/43 | `G3C:0`. Unshifted Back and reopen keep the lock |
| G5A extended length | 60/60 | `G5A:0` |
| Screen | 23/23 | `SCREEN:0` |
| Tap tempo | 6/6 | `TAP:0`. Invoked without `--snap` |
| Slicer | 38/38 | `SLICER:0` |
| Evolution interaction | 76/76 | `TOTAL 76 FAIL 0`, `INTERACT:0` |
| Firmware verifier | pass | `/tmp/g5c3-verify.log`, exit 0, emulator 43/43, `PASS` |
| Heap and stack probe | 11/11 | `/tmp/g5c3-runtime3.log`. Private `VPLAY_DIR=/tmp/fp-g5c3-rt3` |

The interaction harness was updated for the Alpha 1 lock. Shift+Back must keep `recovery` set and toast `Recovery required`. The rest of that session clears the flag only by calling `cfw_genesis_set_recovery(0)`. That symbol is not a menu row.

G3A, G3B, and G3C rewrote historical JSON and screenshots under `research/g3a`, `research/g3b`, and `research/g3c` while they ran. Those trees were restored to the committed copies after the suites passed. The passing counts are the log lines above.

## Interaction coverage on this image

The 76 checks include Melody, Euclidean, Arp, and Evolution; Turing length, probability, lock, reset, and one-shot advance; Markov Step, Motif, Bass, Exploratory, and Learn; empty-source Learn rejection; pitch, rhythm, velocity, and duration mutation; signed rotation both ways; reversal; attribute protection and protected steps across preview pages; 16, 32, and 64-step preview and Apply; rejection past the live native length; regenerate, cancel, second Apply refusal, stale preview, track and pattern changes, and the recovery lock through leave and Shift+Back.

## Diagnostic exposure

`diag_fail_at` is byte offset 32 of the UI object. No menu row writes it. `do_apply` clears it after the call. The runtime probe set it to 7 with a harness poke; after Apply the byte was 0 and `recovery` was 1. `genesis_diag_fixture` is a linked symbol and is not a menu item. Sample tracks are rejected by `genesis_midi_validate_track`. While Genesis is open, knobs and non-select pads are swallowed. The production controls do not reach failure injection, fixture writes, arbitrary memory writes, or unsupported track-type edits.

## Unavailable, reported separately

| Suite | Why it was not run |
|---|---|
| Full `cfw/tests/test_tools.py` | Needs `stock_firmware/PolyendPlay_v1.6.0.ppf`, which is not in this tree. `TestThumbFields` was run on its own. |
| Clang `make host-asan` | The link fails looking for `libclang_rt.asan`. GCC ASan and UBSan is the sanitizer result. |

Earlier runtime logs `/tmp/g5c3-runtime.log` and `/tmp/g5c3-runtime2.log` stopped in the probe harness (the lab walked off Evolution, so Mutation cycles never started). They are not firmware failures. The cited probe is `runtime3`.
