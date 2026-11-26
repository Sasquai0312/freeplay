# Runtime memory audit

Frames are the compiler’s `-fstack-usage` output for the firmware flags (`-Oz`, Thumb, cortex-m7). They are each function’s own frame. They are not a measured high-water mark. The Play+ stack ceiling is not in this tree, so the sums below are estimates of nesting, not proof that the stack fits.

## Own frames that did not grow

| Function | Bytes |
|---|---:|
| `do_generate` | 1,120 |
| `do_apply` | 624 |
| `genesis_midi_apply_ex` | 616 |
| `genesis_midi_validate_events` | 896 |
| `gen_xform_mutate_pitch` | 552 |
| `gen_turing_generate` | 184 |
| `gen_mutate` | 120 |
| `gen_markov_generate` | 64 |
| `gen_evo_turing_preview` / `gen_evo_markov_preview` / `gen_evo_mutate_preview` | 40 |
| `gen_evo_markov_learn` | 24 |
| `gen_markov_learn` | 48 |
| `genesis_midi_restore` / `genesis_midi_snapshot` | 32 |
| `read_source` | 48 |
| `cfw_genesis_render` | 120 |
| `cfw_genesis_input` | 64 |

`generate_evo` is inlined into `do_generate`. The 1,120 byte frame already contains that body. `do_generate` and `do_apply` are the same sizes G5C.1 reported.

## Estimated nesting, if the callees’ frames sit on top of the caller

These are additions of the own frames. Inlining and register allocation can make the real peak smaller. They were not measured with a watermark.

| Path | Estimate |
|---|---:|
| Turing preview | 1,120 + 40 + 184 = 1,344, plus small helpers (`fill` 24, `draw_cell` 32) |
| Markov preset preview | 1,120 + 40 + 64 + `place` 48 ≈ 1,272 |
| Markov Learn | 1,120 + 24 + 48 ≈ 1,192, then the preview path above. Learn returns before preview, so they do not add. |
| Mutation preview | 1,120 + 40 + 120 + 552 = 1,832 |
| Apply, during event validation | 624 + 616 + 896 = 2,136 |
| Apply, during snapshot restore | 624 + 616 + 32. Restore is not nested under validation. |
| Render | 120 + `text_f` 48 = 168 |
| Input | 64 |
| Source read, called from generate | 1,120 already includes the inlined generate; `read_source` adds 48 |

## Heap

One object, `fw_operator_new(sizeof genesis_ui)`.

| Build | `sizeof genesis_ui` | `sizeof gen_evo` |
|---|---:|---:|
| G5C.1 | 3,116 | 1,096 |
| G5C.2 | 3,124 | 1,096 |

The extra 8 bytes are `pstep[8]`. `src_n` occupies a byte that would otherwise have been padding in front of `gen_evo` (offset 2,028, 4-byte aligned). Clang `-fdump-record-layouts` with the firmware ABI (`-fshort-enums`) reports `sizeof=3124`.

A successful link does not say the heap has room. This phase did not find a firmware symbol for the operator-new arena’s free bytes. The object is 8 bytes larger than the G5C.1 object that already allocated on the virtual device. No second allocation was added.

## Aliasing

Mutation still reads `evo_src` on the heap and writes the preview into `storage[64]` on `do_generate`’s stack, then copies into `events`. The source buffer is not the output buffer. The identity toast compares them after the copy. Preview drawing reads `events`, `prev`, and `pstep`. It does not store back into `evo_src` or the native pattern.

No speculative rewrite of the 552-byte pitch scratch was done. That array is the engine’s, and changing it would change the validated transform.
