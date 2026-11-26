# 06 — Memory Budget

Measured from `PlayPlus_FreePlay_01.md` + `llvm-size` (clang-18, `-Oz`).

## Regions

| Region | Used | Capacity | Remaining |
|---|---|---|---|
| Code cave | **13874** | 16384 | 2510 |
| Extension | **47864** | 65536 | 17672 |

G3A baseline ext **42648**. UI Δ ≈ **5216** bytes linked (design target ≤ 8 KiB).

## Objects

| Module | text |
|---|---|
| `genesis_ui.o` | **5293** |
| `genesis_adapt.o` | 3333 (unchanged role) |

## Other

- Additional static CFW RAM: **0** (no new `.bss`/`.data`).
- UI state: heap via `fw_operator_new` + `cfw_pages_genesis`.
- Preview / generate buffers: stack `gen_event[64]` + embedded `gen_preview`.
- No FreePlay feature removal.
