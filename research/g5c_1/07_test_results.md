# Test results

Nothing in this file was counted as a pass if it did not run. The full `cfw/tests/test_tools.py` module was not run: it still needs `stock_firmware/PolyendPlay_v1.6.0.ppf`, which is not in this tree. Its thumb-field class was run on its own.

An earlier G3A launch loaded `/tmp/fp-g1b/img.ppf.boot.snap` because the harness falls back to that file when the work directory has no snapshot. That process raised `UC_ERR_READ_UNMAPPED`. It is not a result for this image. The numbers below use `/tmp/fp-g5c1/PlayPlus_FreePlay_01.ppf.boot.snap`, booted from this candidate (`stopped: None` at 23,000 ms). `VPLAY_SNAP` pointed every Genesis harness at that file.

## Host

`cd genesis && make clean && make test` (clang 18.1.3, `-O2`, no sanitizer):

- `All Genesis tests passed`
- `All evolution tests passed`
- `All evolution session tests passed`
- printed sizes: `turing_cfg=28 turing=100 markov_cfg=32 markov_model=292 mutate_req=56 event=12 gen_evo=1096`

`tests/test_evo_session` covers the G5C.1 list:

| Area | Checks |
|---|---|
| Length | native 1, 16, 32, 64 accepted; 0, 65, 128 rejected |
| Turing | repeated identical previews, committed bytes unchanged, zero-evolution cells, lock, cancel, failed Apply, one successful advance, second Apply refused, stale Apply refused, mode switch does not commit, reset is deterministic and leaves the phase EMPTY |
| Markov | all four presets stay in scale and do not publish a model; learned generation before learn is `GEN_ERR_EMPTY`; learn does not modify the source or the published model; generation does not change the pending hash; failed Apply keeps the model; successful Apply publishes once; empty and unordered sources keep the previous model; 128 steps rejected |
| Mutation | zero probability is identity; pitch protection; a full 8-step grid does not move under displace 100; step 0 stays frozen; reverse plus rhythm protection is `GEN_ERR_UNSUPPORTED` with an empty output; capacity leaves the output empty; aliasing is `GEN_ERR_PARAM`; source bytes unchanged; 65 steps rejected |
| Audit | good event, bad note, bad velocity, duration 3, off-grid tick, 128-step phrase |

## Sanitizers

GCC 13.3.0, `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`, same three binaries: all three suites passed, including `sizeof gen_evo=1096`.

Clang 18.1.3 `make host-asan` compiles and then fails to link:

```
cannot find libclang_rt.asan_static-x86_64.a
cannot find libclang_rt.asan-x86_64.a
```

That is the same missing compiler-rt archive reported in G5B. The clang sanitizer run did not execute. The GCC sanitizer run did.

## Native virtual device

Image `/tmp/fp-g5c1/PlayPlus_FreePlay_01.ppf`. Log `/tmp/g5c1-native.log`.

| Suite | Result | Exit |
|---|---|---|
| `cfw/tests/test_tools.py TestThumbFields` | 3/3 | 0 |
| `research/g3a/diagnostics/g3a_integrate.py` | 42/42 | 0 |
| `research/g1b/diagnostics/midi_mono_validate.py` | 6/6 | 0 |
| `research/g3b/diagnostics/g3b_ui_validate.py` | 17/17 | 0 |
| `research/g3c/diagnostics/g3c_e2e_validate.py` | 43/43 | 0 |
| `research/g5a/diagnostics/g5a_length_validate.py` | 60/60, 0 failed | 0 |
| `cfw/tests/vdev_screen.py` | 23/23 | 0 |
| `cfw/tests/vdev_tap.py` | 6/6 | 0 |
| `cfw/tests/vdev_slicer.py` | 38/38 | 0 |
| `verify.py` emulated slice (`emu` inside verify) | 43/43 | see below |

G3B still reaches Arp in one top-button tap from Euclid. Evo is the next lab, so the existing Melody → Euclid → Arp checks still pass. G5A’s 60 checks include lengths 32 and 64, the knob clamp back to 16, preview pages, and the failure stages.

`verify.py` (target JSON, then the `.ppf`) reports checksums, hook placement, and emu 43/43, then **FAIL** on three literals that are not in the allow-list:

- `0x6018e67c` literal `0x10003`
- `0x60193afc` literal `0xffa50`
- `0x6019bfc0` literal `0x6e400002`

Those three values are the same findings G5A recorded. The address of `0x6e400002` moved because the extension grew. `0x000F4240` and `0x000186A0` from the Evolution objects are named in `KNOWN_CONSTANTS` and are not in the error list. The verify process exit is non-zero because of the three historical literals. The emulator tests inside that process passed.

Harnesses rewrote screenshots and JSON under `research/g3a`, `research/g3b`, and `research/g3c`. Those files were restored with `git checkout` and are not part of this change. Slicer and screen shots were written under `/tmp/fp-g5c1-slicer` and `/tmp/fp-g5c1-screen`.
