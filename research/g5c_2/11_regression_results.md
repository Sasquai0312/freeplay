# Regression results

Every row below names a process that actually ran against the G5C.2 image `bff1c17fa56cdd4fc79e5b69a9ee5df4a85008a80cd6f13cfdc80f94d6c806ac`, or a host binary built from this branch. A run that used a stale link map or a wrong argument is listed under discarded and is not a candidate result.

Native Genesis harnesses used `VPLAY_SNAP=/tmp/fp-g5c2/PlayPlus_FreePlay_01.ppf.boot.snap`, booted from this image for 23,000 ms (`stopped: None`). `LINK_MAP` was unset for the passing native runs. The shell that first launched G3A still had `LINK_MAP=/tmp/fp-g5a/link.map`; that map is the discarded run.

## Passed

| Suite | Result | Exit | Log |
|---|---|---:|---|
| `cd genesis && make clean && make test` (clang 18.1.3, `-O2`) | Genesis, evolution (including `test_rotate_and_steps`), and evolution session | 0 | `/tmp/g5c2-host.log` |
| GCC 13.3 `make host-asan` (`-O1 -g -fsanitize=address,undefined`) | Same three suites, `sizeof gen_evo=1096` | 0 | `/tmp/g5c2-asan.log` |
| `cfw/tests/test_tools.py TestThumbFields` | 3/3 | 0 | `/tmp/g5c2-native.log` |
| `research/g1b/diagnostics/midi_mono_validate.py` | 6/6 | 0 | `/tmp/g5c2-native.log` |
| `research/g3b/diagnostics/g3b_ui_validate.py` | 17/17 | 0 | `/tmp/g5c2-native.log` |
| `research/g3a/diagnostics/g3a_integrate.py` | 42/42 | 0 | `/tmp/g5c2-native2.log` |
| `research/g3c/diagnostics/g3c_e2e_validate.py` | 43/43 | 0 | `/tmp/g5c2-native2.log` |
| `research/g5a/diagnostics/g5a_length_validate.py` | 60/60, 0 failed | 0 | `/tmp/g5c2-native2.log` |
| `cfw/tests/vdev_screen.py` | 23/23 | 0 | `/tmp/g5c2-native2.log` |
| `cfw/tests/vdev_tap.py` | 6/6 | 0 | `/tmp/g5c2-tap.log` |
| `cfw/tests/vdev_slicer.py` | 38/38 | 0 | `/tmp/g5c2-native2.log` |
| `cfw/tools/verify.py` (JSON, then the `.ppf`) | emu 43/43, Result PASS | 0 | `/tmp/g5c2-verify.log` |
| `research/g5c_2/diagnostics/literal_check.py` | three expected literals | 0 | (stdout) |
| `research/g5c_2/diagnostics/g5c2_interact.py` | 76/76 | 0 | `/tmp/g5c2-interact.log` |

Host sizes from the clang run: `turing_cfg=28 turing=100 markov_cfg=32 markov_model=292 mutate_req=56 event=12 gen_evo=1096`.

G3C still reads the lockout toast `Recovery required` after unshifted Back, close, and reopen. The restore toast on this image is `Pattern may differ`. Shift+Back is the acknowledge.

G3B still reaches Arp in one Top press from Euclid. Evolution is the next lab.

Screen and slicer screenshots are under `/tmp/fp-g5c2-screen` and `/tmp/fp-g5c2-slicer`. They are emulator framebuffers. G3A, G3B, and G3C rewrote historical shots under `research/`; those trees were restored with `git checkout` and are not part of this change.

## Discarded

| Run | Why it is not a result |
|---|---|
| First G3A, G3C, and G5A in `/tmp/g5c2-native.log` | `LINK_MAP` still pointed at `/tmp/fp-g5a/link.map`, so `genesis_midi_validate_track` was called at `0x601995c5`. Unicorn reported `UC_ERR_INSN_INVALID`. The same three suites passed after `unset LINK_MAP`. |
| First `vdev_tap.py` invocation in `/tmp/g5c2-native2.log` | The command passed `--snap`. That script accepts only the `.ppf` and loads `<VPLAY_DIR>/<basename>.boot.snap` itself. Exit 2, no checks ran. The rerun without `--snap` is the 6/6 row above. |
| An earlier screen process sharing `/tmp/fp-g5c2` with another emulator | Stopped before it finished so the card image was not shared. The 23/23 row is the later solo run. |

## Blocked

`cfw/tests/test_tools.py` as a whole was not run. It still requires `stock_firmware/PolyendPlay_v1.6.0.ppf`, which is not in this tree. No other firmware was substituted. `TestThumbFields` is the only class from that module that ran.

## Not a pass

Clang 18.1.3 `make host-asan` was not executed on this branch. G5B and G5C.1 already recorded that this compiler cannot link `libclang_rt.asan_static-x86_64.a` or `libclang_rt.asan-x86_64.a`. The GCC ASan and UBSan run is the sanitizer result.
