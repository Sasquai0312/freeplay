# Memory and linker audit

Offline link of the G5C.1 image. This is the linked consumption, not the G5B object-file estimate.

Command:

```
CFW_SKIP_LLD=1 .venv/bin/python cfw/tools/build.py \
  cfw/targets/playplus-1.3.0/target.json --out /tmp/fp-g5c1 --no-verify
```

Compiler: Ubuntu clang 18.1.3. Flags are the `CFLAGS` in `cfw/tools/build.py` (see `01_baseline_verification.md`). `CFW_SKIP_LLD=1` skips the ld.lld size cross-check only. Placement, relocation, and the cave fill are `cfw/tools/link.py`.

## Linked regions

From `/tmp/fp-g5c1/PlayPlus_FreePlay_01.md`:

| Region | Range | Used | Capacity | Left |
|---|---|---:|---:|---:|
| Code cave | `0x6018ca40..0x60190072` | 13,874 | 16,384 | 2,510 |
| Extension | `0x60190b00..0x6019f424` | 59,684 | 65,536 | 5,852 |
| Static `cfw_ram` | `0x20002700..0x20002800` | 256 | 256 | 0 |

Extension used is `0x6019f424 - 0x60190b00 = 0xE924 = 59684`. Left is `0x601a0b00 - 0x6019f424 = 0x16DC = 5852`. Delta against the G5A extension of 49,836 is **+9,848 bytes**. The cave end is the same address as G5A. Bytes outside the cave and the hook slots: 0. File size 4,098,017 → 4,265,888. Changed 12,897 bytes in 889 existing records. Inserted 3,731 records. Relocations: 2,364.

The G5B combined object estimate of 5,607 bytes is not this delta. The link also contains the UI growth, `transform.o`, `evolve.o`, alignment, and `-Oz` codegen.

## Object text + rodata, firmware `-Oz`, exidx excluded

`llvm-size -A` on `/tmp/fp-g5c1/rt1062_ext_*.o`. `.ARM.exidx.*` is listed in the objects and dropped by `link.py` (`_wanted` rejects `SHT_ARM_EXIDX`). `link.map` contains no `exidx` string.

| Object | text | rodata | exidx dropped | text+rodata |
|---|---:|---:|---:|---:|
| turing | 1,304 | 0 | 80 | 1,304 |
| markov | 1,770 | 5 | 80 | 1,775 |
| mutate | 2,028 | 0 | 80 | 2,028 |
| transform | 738 | 0 | 56 | 738 |
| evolve | 1,398 | 0 | 152 | 1,398 |
| five objects | | | | 7,243 |

UI object, same measurement: G5A `text 5962 + rodata 1071 = 7033`, G5C.1 `text 8138 + rodata 1490 = 9628`, delta **+2,595**.

7,243 + 2,595 = 9,838. The linked extension grew by 9,848. The remaining 10 bytes are alignment between sections. No other new object was added to the extension list.

## Initialized data, BSS, alignment

`link.py` raises `LinkError` if an allocated section is writable or `SHT_NOBITS`. The build completed, so the new objects contributed no `.data` and no `.bss`. Fixed-address sections that miss their alignment also raise. They did not. Cave fill remains the stock `0x00` policy. No region was relocated. The extension still starts at `0x60190b00` and stops before `0x601a0b00`.

## Symbols and branches

Undefined symbols of the new objects resolve to Genesis engine functions already in the extension (table in `02_native_architecture.md`). No `malloc`, no libm, no `__aeabi_*` divider. `gen_xform_gate` and its 64-bit divide live in `transform_gate.c`, which is not in `target.json`. Host tests still link that file.

The verify pass emulates the image and reports every branch in that suite resolved (43/43). See `07_test_results.md` for the three historical literal addresses that `verify.py` still flags. They are not new Evolution constants. `0x000F4240` (1,000,000, the ticks-per-step cap) and `0x000186A0` (100,000, the Markov pitch-search sentinel) are named in `KNOWN_CONSTANTS` and were not reported as errors on this image.

## Stack, own frames, firmware `-Oz -fstack-usage`

These are each function’s own frame, not a summed high-water mark.

| Function | Bytes | What the frame is |
|---|---:|---|
| `do_generate` | 1,120 | includes `gen_event storage[64]` (768) |
| `do_apply` | 624 | includes `gen_snap` (528), same 64-step snapshot as G5A |
| `cfw_genesis_render` | 120 | |
| `cfw_genesis_input` | 64 | |
| `evo_bind`, `read_source` | 48 each | |
| `gen_turing_generate` | 184 | |
| `gen_mutate` | 120 | |
| `gen_xform_mutate_pitch` | 552 | `int pitches[128]` |
| `gen_markov_generate` | 64 | |
| `gen_markov_learn` | 48 | scratch model is the heap `gen_evo.scratch`, not this frame |
| `gen_evo_turing_preview`, Markov preview, mutate preview | 40 each | |
| `gen_evo_markov_learn` | 24 | |
| `gen_evo_audit` | 32 | |

A mutation preview that reaches pitch mutation adds the `do_generate`, `gen_evo_mutate_preview`, `gen_mutate`, and `gen_xform_mutate_pitch` frames if those calls are not inlined: 1,120 + 40 + 120 + 552 = 1,832 bytes, plus smaller callees. That sum is an estimate from the `.su` files, not a measured stack watermark. The pitch table is the G5B transform, now resident because mutation is linked.

## Persistent and scratch state

| Store | Bytes | Where |
|---|---:|---|
| `gen_evo` | 1,096 | inside the one UI heap object: committed Turing 100, proposed Turing 100, model 292, pending 292, scratch 292, plus binds and flags |
| `evo_src[64]` | 768 | same object, native read buffer |
| `events[64]` | 768 | existing preview events used by Apply |
| `do_generate` storage | 768 | stack, discarded when generate returns |
| `gen_snap` | 528 | stack, inside `do_apply` |
| static `cfw_ram` | 256 | unchanged, full |

No second heap block. No caller-provided scratch beyond `gen_evo.scratch`, which learn already required and which now lives in the session object.
