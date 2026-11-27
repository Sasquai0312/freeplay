# Stack and heap preflight

Frames are clang `-fstack-usage` with the firmware flags (`-Oz`, Thumb, Cortex-M7). They are each function’s own frame. Sums are estimates of nesting. The Play+ stack ceiling is not in this tree. An emulator SP sample is not that ceiling.

## Compiler frames

These match the G5C.2 sizes. This phase did not shrink them.

| Function | Bytes |
|---|---:|
| `do_generate` | 1,120 |
| `do_apply` | 624 |
| `genesis_midi_apply_ex` | 616 |
| `genesis_midi_validate_events` | 896 |
| `gen_xform_mutate_pitch` | 552 |
| `gen_turing_generate` | 184 |
| `gen_mutate` | 120 |
| `cfw_genesis_render` | 120 |
| `gen_markov_generate` | 64 |
| `cfw_genesis_input` | 64 |
| `gen_evo_*_preview` | 40 |
| `gen_evo_markov_learn` | 24 |
| `genesis_midi_snapshot` / `genesis_midi_restore` | 32 |
| `cfw_edit_begin` | 16 |

Estimated nesting, adding those frames when the callees are live together:

| Path | Estimate |
|---|---:|
| Turing preview | 1,120 + 40 + 184 = 1,344, plus small helpers |
| Markov preset preview | 1,120 + 40 + 64 + 48 ≈ 1,272 |
| Markov Learn | 1,120 + 24 + 48 ≈ 1,192, then a separate preview |
| Mutation preview | 1,120 + 40 + 120 + 552 = 1,832 |
| Apply during validation | 624 + 616 + 896 = 2,136 |

`generate_evo` is inlined into `do_generate`. Snapshot restore (32) is not nested under validation.

## Native callees on that path

Disassembled from the Alpha 1 image. The listed frame is the prologue only. Functions they call are not included.

| Function | Address | Prologue |
|---|---|---|
| `fw_operator_new` | `0x600aab64` | Tail branch to `0x600ad664`. No frame in the thunk. |
| `fw_undo_push` | `0x600ecfb8` | `push {r3-r7, lr}` (24 bytes), then further calls. |
| `fw_toast_show` | `0x600d99c0` | `push {r4, lr}` (8 bytes), then a call. |
| `fw_step_toggle` | `0x600de6f0` | `push {r4-r7, lr}` plus `sub sp, #12` (32 bytes), then calls. |
| `fw_audio_irq_off` | `0x600b1114` | Store and return. No stack subtract. |

`genesis_midi_upsert` locks, calls `fw_step_toggle` and a step vtable, then unlocks. Those native frames sit on top of the CFW frames only while the PC is in the native function. The extension-only sample below does not include that extra depth.

## Emulator sample

Reset vector SP is `0x20060000`. During one Turing preview and one Mutation preview, a code hook over the extension recorded SP at CFW instructions.

| Sample | SP at `do_generate` entry | Lowest SP seen | Drop | Deepest address |
|---|---|---|---|---|
| Turing preview | `0x2005fef0` | `0x2005f6a0` | 2,128 | `0x6019b37a` `genesis_midi_ticks_to_step` |
| Mutation preview | `0x2005fef0` | `0x2005f6a0` | 2,128 | `0x6019b37a` `genesis_midi_ticks_to_step` |

The hook does not run inside native callees, so this drop is the CFW stack while CFW code is executing. Both previews bottomed in `genesis_midi_ticks_to_step`, which `genesis_midi_validate_events` uses. That matches a `do_generate` frame (1,120) plus the validator (896) more closely than the generator-only sums. The pitch-transform frame is not stacked on top of the validator. The virtual device did not fault. No corrupted return was observed.

The lowest sampled SP is 2,400 bytes below the reset SP. That is not remaining headroom. Other DTCM objects, including the UI allocation below, sit in the same RAM, and the firmware’s stack limit was not found.

## Heap

`state()` calls `fw_operator_new` once when the slot is empty or the magic does not match. The image does `movw r0, #0xc34` (3,124) and branches to `fw_operator_new`. If the pointer is null, `state()` returns 0. `cfw_genesis_enter` returns without writing the sequencer.

The pointer is stored in the knob-page heap object (`cfw_pages_genesis`). Leaving Genesis clears `lit` and `scr`. It does not free the object. The next open reuses it. Preview cancel and Apply errors do not allocate.

On the virtual device, `research/g5c_3/diagnostics/preflight_runtime.py` (`/tmp/g5c3-runtime3.log`):

- First open allocated one 3,124-byte object at `0x2004d670`. Other `fw_operator_new` sizes in that boot were menu and page objects (4 through 92).
- 100 cycles of open, generate, cancel, close: 34 Turing, 33 Markov preset, 33 Mutation. Each cycle left the native notes unchanged.
- The UI pointer was still `0x2004d670`. The 3,124-byte size was requested once. Magic was intact.

The object sits in DTCM (`0x20000000`, 512 KiB mapped). Its end is `0x2004e2a4`. No symbol in this tree reports the operator-new arena’s free bytes. A successful allocation is not a measurement of free heap. Physical free heap is unverified.
