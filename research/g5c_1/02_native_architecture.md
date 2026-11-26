# Native architecture

One Evolution Lab page sits on the existing Genesis page. It is `LAB_EVO = 3`, after Melody (0), Euclid (1), and Arp (2). The top button still cycles Melody → Euclid → Arp in one tap each, then Evo, then back to Melody. There is not a second screen object and not a second heap block.

## Where the state lives

`struct genesis_ui` is still the single `fw_operator_new` object behind `cfw_pages_genesis`. New fields are appended after `events[64]`. The harness offsets are pinned by `_Static_assert`:

| Field | Offset |
|---|---:|
| `len` | 8 |
| `diag_fail_at` | 32 |
| `page` | 33 |
| `native_len` | 34 |
| `seed` | 36 |

Clang `-fdump-record-layouts` for the firmware ABI (`-fshort-enums`, Thumb Cortex-M7) reports `sizeof(struct genesis_ui) == 3116`. The G5A object ended at the preview event array (offset 1232). The appended Evolution state is 1,884 bytes:

| Piece | Offset | Bytes |
|---|---:|---:|
| `evo_src[64]` | 1232 | 768 |
| mode and mutation knobs | 2000 | 19, then pad |
| `gen_evo` | 2020 | 1,096 |
| total object | 0 | 3,116 |

`gen_evo` (host and firmware, 1,096 bytes) holds the committed Turing register, the proposed Turing register, the published Markov model, a pending learned model, and learn scratch. No malloc, no file-scope mutable state, and no `.bss`. `copy_pod` is a word copy, the same helper the portable engine already uses.

Static `cfw_ram` stays `0x20002700..0x20002800` (256/256). The linker still raises `LinkError` on a writable or `SHT_NOBITS` section, so a new global would have failed the link.

## Files

| File | Role |
|---|---|
| `genesis/include/genesis_evo.h` | Session API. Native length ceiling 64. Phases EMPTY, VALID, STALE, ERROR, APPLIED. |
| `genesis/engine/evolve.c` | Session implementation. Calls the G5B generators. Does not change their algorithms. |
| `genesis/engine/transform_gate.c` | `gen_xform_gate` only, so the firmware link of `transform.c` does not pull a 64-bit divider. Host tests still link it. Not listed in `target.json`. |
| `genesis/engine/transform.c` | Gate body removed. Reverse, rotate, velocity, pitch, remove, quantize stay. |
| `genesis/tests/test_evo_session.c` | Host session tests. Binary is gitignored. |
| `cfw/src/genesis_ui.c` | One page, mode selector, preview, cancel, reset, apply. |
| `cfw/targets/playplus-1.3.0/target.json` | Extension objects: `transform.c`, `turing.c`, `markov.c`, `mutate.c`, `evolve.c`. Not `transform_gate.c`. Not `chords.c`. |
| `cfw/targets/playplus-1.3.0/target.h` | Version string `1.3.0-FreePlay0.2b` so the candidate is distinct from G5A `0.2a`. |
| `cfw/tools/verify.py` | Names the two new integer literals the Evolution objects emit. |
| `genesis/Makefile` | Builds and runs `tests/test_evo_session` with the existing engine tests. |

`genesis_adapt.c` was not edited. Destination checks, the 12-byte `gen_event` layout, note index 6, velocity index 0, duration index 1, and `genesis_midi_apply_ex` are the G5A adapter.

## Navigation and editing

Existing input, render, toast, and parameter-index paths are reused. New parameter indices are appended after `P_PRESET`, so Melody, Euclid, and Arp indices are unchanged. Visible controls depend on the lab and, inside Evo, on the mode. Shared Lab, Track, Length, Seed, Root, Scale, Gate, V.min, and V.max stay on screen. Click generates. Shift+Click mixes a new seed and generates, which changes the Turing register signature and therefore reinitializes the register on the next Turing preview. Leaving the page and cycling labs both call `invalidate`, which discards the proposed session state and clears the preview.

## What the firmware link actually contains

Undefined symbols in the firmware-flag objects, from `llvm-nm -u`:

| Object | Undefined |
|---|---|
| turing | `gen_buf_push`, `gen_rng_bound`, `gen_rng_seed`, `gen_scale_contains` |
| markov | `gen_buf_push`, `gen_rng_bound`, `gen_rng_seed`, `gen_scale_degree_count`, `gen_scale_degree_to_midi` |
| mutate | `gen_buf_init`, `gen_buf_sort`, `gen_buf_validate`, `gen_rng_*`, `gen_scale_quantize`, `gen_xform_reverse`, `gen_xform_rotate`, `gen_xform_velocity` |
| transform | `gen_buf_sort`, `gen_rng_bound`, `gen_rng_range`, `gen_scale_contains`, `gen_scale_quantize` |
| evolve | the five `gen_turing_*` / `gen_markov_*` / `gen_mutate` entry points and `gen_buf_validate` |

No `malloc`, no libm, no `__aeabi_uldivmod`, no new operating-system symbol. RNG remains `gen_rng_*` (xorshift32). Seed 0 is rewritten to 1 at the UI fill site; the engine also remaps 0 to `0xA5A5A5A5`.
