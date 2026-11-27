# Resource budget

Baseline before G6B: extension 56,136 of 65,536 (free 9,400), reserve 2,048, feature budget 7,352, cave 15,910 of 16,384, static CFW RAM 256 of 256, UI object 3,124, `gen_evo` 1,096.

Production image `1.3.0-FreePlay0.2f`:

| Resource | Value |
|---|---|
| Extension | `0x60190b00` .. `0x6019f870` (60,784 bytes) |
| Extension free | 4,752 |
| Feature budget after the 2,048 reserve | 2,704 |
| Growth vs 56,136 | 4,648 |
| Cave | `0x6018ca40` .. `0x60190866` (15,910). Free 474. No new cave hook. |
| Static `cfw_ram` | `0x20002700`, still the existing 256 bytes |
| UI object | 3,200 (`0xC80`), plus 76 |
| `gen_evo` | 1,096 |
| `gen_native_step` / snap | 8 / 528 |
| `gen_preview` | 344 |
| `do_apply` stack subtract | `0x24c` |
| `genesis_midi_apply_ex` stack subtract | `0x254` |
| `do_generate` stack subtract | `0x43c` |

The 2,560-byte planning target was missed. The reserve was not spent to chase it. `param_visible` is `noinline` and `optnone`. At `-Oz` the same comparisons folded into the constants `0x18380` and then `0x18300`, and the firmware verifier rejects a pool literal in `0x10000`..`0xFFFFF` as an unknown low address. The comparisons are unchanged. That choice adds 584 bytes over the first 60,200-byte link.

Diagnostic extension ends at `0x6019fbb0` (61,616 bytes), 832 bytes above production, because `genesis_diag_chord_apply` and the other diagnostic keeps stay in that link only. Diagnostic verifier result: PASS, emulated tests 43/43.

Host Thumb sizes at `-Os`: `chordprog.o` 1,204 bytes text+rodata+data. `chords.o` remains 1,376 and is not in the firmware link.

On the final production image the runtime probe passed 12/12. It opened, generated, cancelled, and closed 100 times (34 Turing, 33 Markov, 33 Mutate). The UI allocation stayed one 3,200-byte object. The emulator stack sample dropped 2,128 bytes from `0x2005fef0` to `0x2005f6a0`, 2,400 below the reset point, at `0x6019a68c`. That sample is not physical headroom. Heap use on device was not measured.

An earlier production session, before `param_visible` was marked `optnone`, walked labs 0..6 across 100 cycles. Phase, pending, and ready stayed 0. An 8-byte compare also covered `arm_ms`, which changes on reopen; the assertion now stops at the 5-byte tail of `gen_evo`. Those five bytes were `0000000000` before and after. The final image's Lab+ walk is the interaction suite, which reaches Chords and wraps back to Melody.
