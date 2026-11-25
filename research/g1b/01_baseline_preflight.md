# 01 — Baseline Preflight

## Firmware

| Field | Value |
|---|---|
| Path | `/workspace/PlayPlus_v1.3.0.ppf` |
| Size | 4098017 |
| SHA-256 | `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` |
| Match | **YES** |

## Build

Same clang-18 procedure as G1A (optional `ld.lld` cross-check skipped via in-process monkeypatch; FreePlay sources unmodified).

| Step | Result |
|---|---|
| Stock SHA gate | Pass |
| Output | `cfw/build/playplus-1.3.0/PlayPlus_FreePlay_01.ppf` |
| `emu_slice` | **43/43** |
| `verify.py` literals | Unknown-literal noise under clang-18 (non-blocking) |

## Regression reproduction vs G1A

| Suite | G1A | G1B | Delta |
|---|---|---|---|
| Play+ PPF identity/patch/uncovered | 3/3 | 3/3 | Same |
| Thumb field unit tests | OK | OK | Same |
| `emu_slice` | 43/43 | 43/43 | Same |
| `vdev_screen` | 23/23 | 23/23 | Same |
| `vdev_slicer` | 38/38 | 38/38 | Same |
| `vdev_tap` | 6/6 | 6/6 | Same |
| `vdev_menutree` walk | Incomplete | Depth-2 timed out (~180s, no tree written) | Still incomplete |

Virtual boot: snapshot at 23000 ms device time (`/tmp/fp-g1b/img.ppf.boot.snap`).

## `rebuild/export/functions.tsv` stub

| Question | Finding |
|---|---|
| What reads it? | `cfw/vdev/boot.py` `fnnames()` only |
| When? | `stage.py` progress labels during boot |
| Symbol resolution? | **No** — not used by linker, CFW, or step APIs |
| MIDI functionality? | **No** |
| Diagnostics? | Cosmetic PC→name only |
| Verified native? | **Do not** treat stub names as verified symbols |

Stub remains gitignored under `rebuild/`.

## Compiler / layout

| Item | Value |
|---|---|
| Compiler | Ubuntu clang 18.1.3 |
| Local FreePlay output SHA | Differs from published installer (expected) |
| Linked cave-ish (`< 0x60190b00`) | 13719 bytes |
| Linked ext (`≥ CFW_EXT_BASE`) | 35914 bytes |
