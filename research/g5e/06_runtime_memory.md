# Runtime memory

Stack frames from the optimized objects, compared with the Alpha 1 objects:

| Function | Alpha 1 | Optimized |
|---|---:|---:|
| `do_generate` | `0x43c` | `0x43c` |
| `do_apply` | `0x24c` | `0x24c` |
| `cfw_pages_render` | `0x280` | `0x280` |
| `cfw_genesis_render` | `0x54` | `0x5c` |
| `gen_mutate` | inside the extension function | `0x54` |
| `genesis_midi_apply_ex` |  | `0x244` |

`do_generate` still builds the preview in a stack buffer and copies the events into the UI object. That copy is the exact-preview buffer Apply writes. Removing it would alias the generator output with the committed preview. It was left in place. No static buffer was added. Static CFW RAM is still 256 of 256.

The UI object layout is unchanged: length at offset 8, `diag_fail_at` at 32, page at 33, native length at 34, seed at 36. `gen_evo` is still 1,096 bytes. The allocation size checked by the probe remains `0xC34` (3,124).

The runtime probe on the production image sampled two previews. Both stopped at the same watermark: entry `0x2005fef0`, minimum `0x2005f6a0`, a drop of 2,128 bytes, 2,400 bytes below the reset stack pointer `0x20060000`, at `0x6019a260`. That is one sample path for a Turing preview and one for a Mutation preview. It is not a sweep of every call path. The probe also saw one UI allocation of 3,124 bytes, a stable pointer across 100 open/generate/cancel/close cycles, and the recovery flag set after a restore error. These numbers are Unicorn samples. Physical Play+ stack and heap headroom remain unknown.
