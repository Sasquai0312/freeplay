# Resource measurements

Target: about 5632 bytes of new Thumb code and read-only data, combined. Per-module figures of 1536, 2048, and 2048 were optimization targets.

Compiler: `clang -Os -target thumbv7em-none-eabihf -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -ffreestanding -fno-builtin -ffunction-sections -fdata-sections`. `make thumb-size` compiles every engine module and `genesis/tools/thumb_size.py` sums `.text`, `.rodata`, and `.data` from `llvm-size -A`. `.ARM.exidx`, comments, and notes are excluded. `arm-none-eabi-size` is not installed here; the Python sum is the measurement.

## New objects

| Object | text | rodata | data | bss | text+rodata |
| --- | ---: | ---: | ---: | ---: | ---: |
| `turing.o` | 1376 | 0 | 0 | 0 | 1376 |
| `markov.o` | 1902 | 5 | 0 | 0 | 1907 |
| `mutate.o` | 2324 | 0 | 0 | 0 | 2324 |
| **Combined** | **5602** | **5** | **0** | **0** | **5607** |

5607 is under the 5632 target. The whole portable engine, old modules included, sums to 11814 bytes by the same rule. That total is still not a linked firmware size.

Undefined symbols in the three objects are Genesis functions only: buffer, RNG, scale, and `gen_xform_reverse`, `gen_xform_rotate`, `gen_xform_velocity`. There is no `__aeabi_memcpy`, no libm, and no allocator. Struct copies go through a word loop so the Thumb compiler does not emit a memcpy helper.

`.ARM.exidx` is present per function because unwind tables were not disabled. Existing G2 objects are built with the same flags. A link that drops unwind tables, as the current engine objects expect, does not have to pay those 8-byte entries. Do not add them into the 5607 figure, and do not assume a future link will drop them until G5C checks the map.

## Caller-owned sizes

Host `sizeof` and a Thumb `.data` dump of the same `sizeof` constants match.

| Object | Bytes |
| --- | ---: |
| `gen_turing_cfg` | 28 |
| `gen_turing` | 100 |
| `gen_markov_cfg` | 32 |
| `gen_markov_model` | 292 |
| `gen_mutate_req` | 56 |
| `gen_event` | 12 |
| `gen_rng` | 4 |

Worst-case output is one event per requested step: 64 events (768 bytes) for a native destination, 128 events (1536 bytes) for a portable buffer. Mutation needs capacity of at least `dest_steps` events even when the result is shorter.

Scratch: Turing needs the `next` state (100 bytes) during preview. Markov learn needs a second model (292 bytes) for the duration of the call. Mutation has no caller scratch.

## Stack

`clang -fstack-usage` on the Thumb `-Os` objects. These are each function’s own frame, not the sum of its callees.

| Function | Frame bytes |
| --- | ---: |
| `gen_turing_generate` | 184 |
| `gen_turing_seed` / `gen_turing_reset` | 40 |
| `check_cfg` / `draw_cell` | 32 |
| `fill` | 24 |
| `note_budget` | 8 |
| `gen_markov_generate` | 64 |
| `gen_markov_learn` / `place` / `pitch_of` | 48 |
| `gen_markov_preset` | 40 |
| `degree_of` | 32 |
| `gen_mutate` | 120 |

The 184-byte Turing frame holds the proposed state. If the Markov calls are not inlined, generate + place + pitch_of + degree_of is about 192 bytes of frames. Mutation’s 120-byte frame includes the occupancy masks. Existing `gen_buf_*` and `gen_xform_*` frames sit under those calls and were not re-measured here.

G5A’s linked image remains the baseline for firmware: code cave 13874 / 16384, extension 49836 / 65536 (15700 bytes left), static `cfw_ram` 256. G5B does not claim those numbers moved.
