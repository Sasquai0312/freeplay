# Resources and remaining limits

Numbers below are the G5C.3 measurements of the image the owner installed. This phase did not re-link or re-measure them.

| Region | Span | Used | Capacity | Left |
|---|---|---:|---:|---:|
| Code cave | `0x6018ca40..0x60190072` | 13,874 | 16,384 | 2,510 |
| Extension | `0x60190b00..0x6019fb60` | 61,536 | 65,536 | 4,000 |
| Static `cfw_ram` | `0x20002700..0x20002800` | 256 | 256 | 0 |

`genesis_ui` is 3,124 bytes. `gen_evo` is 1,096. Extension reserve is 4,000 bytes, above the 2,048 floor.

Compiler frames and the virtual-device stack sample are in `research/g5c_3/03_stack_heap_preflight.md`. Nested estimates: Turing preview about 1,344 bytes, Markov preset about 1,272, Markov Learn about 1,192 then a separate preview, mutation preview about 1,832, Apply during validation about 2,136. On the virtual device, one Turing preview and one Mutation preview each dropped 2,128 bytes inside CFW code and did not fault. That sample excludes native callees. The Play+ stack ceiling was not measured.

On the virtual device, one 3,124-byte UI object was allocated at `0x2004d670` and reused across 100 open, generate, cancel, and close cycles. Free bytes in the `fw_operator_new` arena were not read from a firmware symbol.

## Still open after the owner’s general confirmation

- Physical stack capacity
- Physical free heap
- A `GEN_MIDI_ERR_RESTORE` return can leave the destination fully written or partly written. Apply stays blocked until the next power-on and does not undo the notes. The owner did not report seeing this state.
- Full `cfw/tests/test_tools.py` still needs `stock_firmware/PolyendPlay_v1.6.0.ppf`
- Clang `make host-asan` still cannot link `libclang_rt.asan`

The owner’s sentence does not close these measurements.
