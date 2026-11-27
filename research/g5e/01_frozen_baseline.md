# Frozen baseline

Reproduced from `cursor/g5c3-alpha1-hardware-preflight` at `6e28718856c25af9d1c9a30a26a438e4fe7886a6`, before this phase changed sources.

| Item | Value |
|---|---|
| Version | `1.3.0-FreePlay0.2d` |
| Image | `PlayPlus_FreePlay_Genesis_v0_2_Alpha1.ppf` |
| Size | 4,271,087 bytes |
| SHA-256 | `39a061780b16832de063d30a248233405be40378e2dffe9833ac15b8bf193228` |
| Stock | `PlayPlus_v1.3.0.ppf` |
| Stock SHA-256 | `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` |
| Compiler | clang 18.1.3, `-Oz`, `thumbv7em-none-eabihf`, cortex-m7, hard float |
| Linker | `cfw/tools/link.py` (`CFW_SKIP_LLD=1`) |

A fresh `-Oz` link of that unmodified tree measured:

| Region | Capacity | Used | Free |
|---|---:|---:|---:|
| Extension | 65,536 | 61,536 | 4,000 |
| Code cave | 16,384 | 13,874 | 2,510 |
| Static CFW RAM | 256 | 256 | 0 |

Protected extension reserve 2,048. Expansion budget before this phase: 1,952. UI object 3,124. `gen_evo` 1,096. `do_generate` stack frame `0x43c`. `do_apply` stack frame `0x24c`. Physical stack and heap were not measured.

The optimized image is a different build (`1.3.0-FreePlay0.2e`). It does not replace the Alpha 1 file or the v0.1 file.
