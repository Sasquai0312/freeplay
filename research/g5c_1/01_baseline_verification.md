# G5C.1 baseline verification

Working branch at the start of implementation: `cursor/g5c1-evolution-lab-f3f4`, created from G5B and still uncommitted at the time of this check. Parent commit:

`34dc6212bc3f8c6fca38deacca6f1e274e8f9314` — “G5B: add portable Turing, Markov, and mutation generators”.

Ancestry, from `git merge-base --is-ancestor` against that parent:

| Baseline | Commit | Result |
|---|---|---|
| G5B | `34dc621` | parent of this branch |
| G5A | `802c4de2fca77767d63ce7e5dd81a6b7b91030bb` | ancestor |
| G3C | `90ed2e6` | ancestor |
| G4A | `fb907d4` | ancestor |

The tree at `34dc621` was clean before the G5C.1 edits. No merge to `main`, no release tag, and no historical research artifact was replaced. The G5B portable engine sources were left in place; G5C.1 adds a session layer and a native page on top of them.

## Firmware input

Stock image named by `cfw/targets/playplus-1.3.0/target.json`:

`PlayPlus_v1.3.0.ppf` SHA-256 `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876`

That hash matches the target file. The G5A candidate `efd5a1a6fc2d67b05f41ae22e2d6f6c58d6bac822e14c3bfe61acf9a8e6fcb17` was not overwritten. It remains an uncommitted offline image under `/tmp/fp-g5a`. The v0.1 artifact hash `5dc172f7…4f26` was not touched. No `.ppf` is committed.

## Reported budgets, not treated as the link result

G5A linked baseline, from `research/g5a/07_memory_budget.md`:

| Resource | Used | Capacity | Left |
|---|---:|---:|---:|
| Code cave `0x6018ca40` | 13,874 | 16,384 | 2,510 |
| Extension `0x60190b00` | 49,836 | 65,536 | 15,700 |
| Static `cfw_ram` `0x20002700` | 256 | 256 | 0 |

G5B Thumb `-Os` object text+rodata, from the G5B size report: Turing 1,376, Markov 1,907, Mutation 2,324, combined 5,607, whole portable engine 11,814. The “10,093 bytes of extension headroom” figure was that G5A remainder minus 5,607. It is not the linked G5C.1 budget. The linked measurement is in `06_memory_and_linker_audit.md`.

## Build configuration that was already in the tree

`cfw/tools/build.py` compiles with clang `-Oz -ffreestanding -fno-builtin -fno-pic -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -fno-common -fshort-enums -std=c11 -Wall -Wextra -Werror`, target `thumbv7em-none-eabihf`, `-mcpu=cortex-m7`, hard float.

`cfw/tools/link.py` places every allocated section of the listed objects. It rejects writable and `SHT_NOBITS` sections, and it drops `.ARM.exidx`. The extension region is `0x60190b00` through `0x601a0b00` (65,536 bytes). The cave is `0x6018ca40` for 16,384 bytes. `CFW_SKIP_LLD=1` skips the ld.lld size cross-check. The link itself is `link.py`. The interpreter is `.venv/bin/python` because the system interpreter has no `pyelftools`.

Host regression at the G5B parent was already green (`make test` in `genesis/`). It was re-run after the G5C.1 sources and is recorded in `07_test_results.md`.
