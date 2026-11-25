# 01 — Upstream Baseline

**Upstream:** https://github.com/sloppyjoo/freeplay  
**Pinned commit:** `5835b33745b5b6eca92466585f79ccb00fb28c0b`  
**Commit subject:** `FreePlay 0.1 beta` (2026-11-06)  
**Classification:** VERIFIED_SOURCE (git clone + `git rev-parse`)

This workspace fork (`Sasquai0312/freeplay`) matches upstream byte-for-byte at that commit (`diff -rq` excluding `.git`: no differences).

## Releases and tags

| Item | Finding | Class |
|---|---|---|
| Git tags | None | VERIFIED_SOURCE |
| GitHub Releases | Empty list | VERIFIED_SOURCE (API) |
| Documented version | `0.1 beta` / `CFW_VERSION_STRING` `1.3.0-FreePlay0.1b` | VERIFIED_SOURCE (`CHANGELOG.md`, `target.h`) |

## Supported devices and firmware

| Device | Stock firmware required | FreePlay status | Class |
|---|---|---|---|
| Play+ | `PlayPlus_v1.3.0.ppf` (SHA256 `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876`) | Supported; author-tested on hardware | VERIFIED_SOURCE (`target.json`, README) |
| Original Play, RT1062 | `PolyendPlay_v1.6.0.ppf` (SHA256 `e32ce373f67de3cdec531756f2c627f53624b83e11f3f80d5a350ddfcf65a163`) | Built; **only virtual** testing claimed | VERIFIED_SOURCE |
| Original Play, K66 | Same PPF, separate image | Incomplete / not in release | VERIFIED_SOURCE (README, CONTRIBUTING) |

**OWNER_REPORTED:** Play+ owner has FreePlay installed and functioning.

## Repository structure

| Path | Role |
|---|---|
| `cfw/src/` | Freestanding C features |
| `cfw/targets/` | Per-firmware addresses (`symbols.ld`, `target.h`, `target.json`) |
| `cfw/tools/` | Build, link, verify, disasm, ppf |
| `cfw/vdev/` | Unicorn virtual Play |
| `cfw/tests/` | Toolchain + vdev tests |
| `cfw/presets/` | CC preset CSV |
| `patches/` | Published patch JSON (FreePlay bytes only) |
| `patcher/` | Browser + Python patcher |
| `index.html` | Installer |
| `docs/` | DEVELOPING, FIRMWARE_NOTES, TESTING |

## Licensing and redistribution

| Topic | Finding | Class |
|---|---|---|
| FreePlay code | MIT (`LICENSE`) | VERIFIED_SOURCE |
| CC presets | CC BY-SA 4.0 (MIDI Guide derivative) | VERIFIED_SOURCE |
| Polyend firmware | **Not included**; must not be published | VERIFIED_SOURCE (LICENSE, CONTRIBUTING) |
| Patch distribution | Patch JSON / installer only; finished `.ppf` forbidden in repo | VERIFIED_SOURCE |

## Build dependencies

| Dependency | Purpose | Class |
|---|---|---|
| Python ≥ 3.8 | Build / tests / patcher | VERIFIED_SOURCE |
| `clang` | Cross-compile Thumb | VERIFIED_SOURCE |
| `ld.lld` / `llvm-objcopy` | Optional cross-check | VERIFIED_SOURCE |
| `capstone`, `pyelftools`, `unicorn`, `numpy`, `pillow` | `requirements.txt` | VERIFIED_SOURCE |
| `mkfs.fat`, `mtools` | Virtual SD cards | VERIFIED_SOURCE |
| Stock `.ppf` locally | Required to build / vdev | VERIFIED_SOURCE; **BLOCKED** in this environment (files absent) |

## Compiler and target architecture

Play+ / Play RT (`target.json`):

```
-target thumbv7em-none-eabihf -mcpu=cortex-m7 -mfloat-abi=hard
```

CPU: NXP i.MX RT1062 Cortex-M7. Freestanding: no libc, no writable sections, avoid `memcpy`/jump-table `switch` (`docs/DEVELOPING.md`). Class: VERIFIED_SOURCE.

## Memory budget (Play+ 1.3.0)

| Region | Range | Size | Class |
|---|---|---|---|
| Cave (in-image unused) | `0x6018ca40`–`0x60190a40` | 16 KiB | VERIFIED_SOURCE |
| Ext (appended HEX) | `0x60190b00`–`0x601a0b00` | 64 KiB | VERIFIED_SOURCE |
| Scratch RAM `cfw_ram` | `0x20002700`–`0x20002800` | 256 B (full) | VERIFIED_SOURCE |
| Larger state | Heap via `fw_operator_new`, once per boot | Bounded by RT heap | VERIFIED_SOURCE / INFERRED limit |

## Test infrastructure

| Layer | Entry | Needs stock PPF? |
|---|---|---|
| Toolchain unit tests | `python cfw/tests/test_tools.py` | Yes for PPF tests; Thumb encode tests no |
| Linker cross-check | `lld_crosscheck.py` | No PPF; needs full object graph |
| Virtual device | `cfw/tests/vdev_*.py` | Yes + built image + `link.map` |
| Emulated slice world | `emu_slice.py` | Yes + build artifacts |

## Firmware patching procedure (summary)

1. User obtains official PPF from Polyend.  
2. Browser (`index.html` + `patcher/freeplay.js`) or `patcher/freeplay_patch.py` applies `patches/*.patch.json`.  
3. Output named `PlayPlus_FreePlay_01.ppf` / `PolyendPlay_FreePlay.ppf`.  
4. Copy FreePlay + official file to SD `Firmware/`; update via Settings.  
5. Installer verifies input/output SHA256.  

Class: VERIFIED_SOURCE (README).

## Virtual hardware

Unicorn ARMv8 AArch32 Thumb model of RT1062; boots real firmware; HLE for SD, USB stubs, audio clock, input injection (`cfw/vdev/`). Not cycle-accurate. Class: VERIFIED_SOURCE.

## Known limitations and recovery

- K66 unsupported; Play RT untested on real hardware.  
- LFOs / Sustain / Release: sample tracks only.  
- Recovery: flash stock via Settings, or SD + rear reset with only official PPF in `Firmware/`.  
Class: VERIFIED_SOURCE (README).

## G0 host reproduction results

| Command | Result | Class |
|---|---|---|
| `git clone` upstream; compare to workspace | Identical at `5835b33` | VERIFIED_HOST_TEST |
| `pip install -r requirements.txt` | OK (unicorn 2.1.4, etc.) | VERIFIED_HOST_TEST |
| `clang` Thumb freestanding smoke compile | Exit 0 | VERIFIED_HOST_TEST |
| `test_tools.py` Thumb field tests | 3/3 OK | VERIFIED_HOST_TEST |
| `test_tools.py` PPF tests | ERROR: missing stock PPF | BLOCKED |
| `test_tools.py` LinkerAgainstLld | FAIL: `undefined symbol cfw_ext_ok` when linking `slice.c` alone | VERIFIED_HOST_TEST (tool present; test needs fuller link set) |
| `cfw/build_all.py` | Not run — no stock firmware | BLOCKED |
| `vdev_*.py` | Not run — no stock firmware / built image | BLOCKED |
