# 02 — Baseline Validation

Environment: Ubuntu, clang 18.1.3, Python 3.12 venv with `requirements.txt`, ld.lld/llvm-objcopy present. Branch `cursor/g1-midi-param-discovery-f3f4` @ G0 commit `8bfc72b` + G1 docs.

## Host tests (no stock PPF required)

| Test | Command | Result | Class |
|---|---|---|---|
| Thumb field encode/decode | `.venv/bin/python cfw/tests/test_tools.py TestThumbFields -v` | 3/3 OK | VERIFIED_HOST_TEST |
| clang Thumb freestanding smoke | `clang -target thumbv7em-none-eabihf -mcpu=cortex-m7 -mfloat-abi=hard -ffreestanding -nostdlib -c …` | Exit 0 | VERIFIED_HOST_TEST |
| Deps import | `import unicorn,capstone,numpy,PIL` | OK | VERIFIED_HOST_TEST |

## Tests requiring stock firmware (blocked)

| Test | Command | Result | Class |
|---|---|---|---|
| PPF unit tests | `test_tools.py TestPPF` | FileNotFoundError `PlayPlus_v1.3.0.ppf` | BLOCKED |
| Full FreePlay Play+ build | `build.build('cfw/targets/playplus-1.3.0/target.json')` | Not run — no input PPF | BLOCKED |
| Virtual Play boot | `vdev_*.py` | Not run | BLOCKED |
| Menu navigation / pads / encoder | vdev flows | Not run | BLOCKED |
| Sample step editing / undo | `vdev_slicer` / `vdev_startend` | Not run | BLOCKED |

## Upstream vs experimental failure distinction

All failures above are **upstream-environment blockers** (missing proprietary stock firmware), not regressions introduced by G1. G1 added only `research/g1/` documentation. Class: VERIFIED_HOST_TEST.

## Published build identity (without local rebuild)

From `patches/PlayPlus.patch.json` (VERIFIED_SOURCE):

| Field | Value |
|---|---|
| Target | `playplus-1.3.0` |
| Input SHA-256 | `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` |
| Output SHA-256 | `ccbbf0358203acd9ad2bfa1b69d185230f5f026cba3fd3d1365ff0e0d6105670` |
| Output name | `PlayPlus_FreePlay_01.ppf` |

Local rebuild to match that output hash was **not** performed (blocked).

## Conclusion

Host toolchain is healthy. Virtual baseline and sample-edit verification remain **BLOCKED** pending firmware.
