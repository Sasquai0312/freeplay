# 07 — Linked Memory Budget

Measured from `cfw/build/playplus-1.3.0/PlayPlus_FreePlay_01.md` + `llvm-size` on objects (clang-18, `-Oz`).

## Regions

| Region | Used | Capacity | Remaining |
|---|---|---|---|
| Code cave | **13738** | 16384 | 2646 |
| Extension | **42648** | 65536 | 22888 |

G1B baseline: cave 13719 / ext 35914. Genesis lives entirely in **ext** (Δ = **6734** bytes linked).

## Object text (not equal to linked contribution)

| Module | text bytes |
|---|---|
| event | 572 |
| random | 154 |
| scales | 599 |
| rhythm | 678 |
| melody | 854 |
| arpeggiator | 848 |
| genesis_adapt (+diag + wrappers) | 3333 |
| genesis_aeabi | 110 |
| **Sum** | **7148** |

Request-wrapper contribution is small (struct pass-through); diagnostic fixture/generate paths dominate adapter growth.

Linked overhead (headers, alignment, rodata strings, outline) explains sum vs region Δ.

## Other

| Item | Value |
|---|---|
| Static CFW RAM | No new `.bss`/`.data` (linker forbids); snap/preview caller- or heap-owned |
| Max event buffer | 64 (`GEN_SNAP_MAX`) |
| Max snapshot | 64 steps × native step record |
| Heap | `fw_operator_new` for diagnostic buffers in vdev harness |
| Excluded from link | chords, transform, SMF exporter |
| Production headroom (ext) | ~23 KiB |

`ld.lld` cross-check skipped (same clang-18 limitation as G1A/G1B; in-process monkeypatch only).
