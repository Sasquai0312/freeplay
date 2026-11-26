# Handoff to G5C.3

G5C.2 is an offline UI candidate. Do not start hardware validation from this note alone, and do not flash this image as part of G5C.2.

## Candidate

| | |
|---|---|
| Branch | `cursor/g5c2-evolution-ui-f3f4` |
| Image | `/tmp/fp-g5c2/PlayPlus_FreePlay_01.ppf` |
| SHA-256 | `bff1c17fa56cdd4fc79e5b69a9ee5df4a85008a80cd6f13cfdc80f94d6c806ac` |
| Version | `1.3.0-FreePlay0.2c` |
| Boot snapshot used here | `/tmp/fp-g5c2/PlayPlus_FreePlay_01.ppf.boot.snap` (23,000 ms, `stopped: None`) |

Boot a new snapshot from the image under test. A snapshot from G5C.1 or G1B faults with `UC_ERR_READ_UNMAPPED` or executes the wrong bytes. Leave `LINK_MAP` unset, or set it to this image’s `link.map`. A leftover G5A map calls `genesis_midi_validate_track` at `0x601995c5`, which is not this image, and Unicorn reports `UC_ERR_INSN_INVALID`.

## What hardware validation has to do

1. Confirm the panel is the 320×192 Genesis panel and that Top still walks Melody, Euclid, Arp, Evo.
2. On Turing, confirm register length and phrase length are different controls, Lock is visible, Reset does not write notes, and a repeated preview matches.
3. On Markov, confirm the four presets and Learn, including an empty pattern (`No source notes`) and that a learned model appears on the pattern only after Apply.
4. On Mutation, confirm a negative RotBy, the P/R/V/D attribute text, more than one protected step using the step pads, and `Rev/rot blocked` when reverse or rotate meets protection.
5. Apply once, then try Apply again and confirm it does not write the same preview twice.
6. Do not treat a restore error as “nothing was written”. If Apply reports that the pattern may differ, inspect the track before generating again. Shift+Back is the on-device acknowledge. It re-reads the track. It does not prove the write was atomic.
7. Stay within pattern lengths 1, 16, 32, and 64. The knob does not offer 128.

## Do not change in order to make that pass

- G5B generator behavior, collision rules, and protection semantics.
- The native Apply writer in `genesis_adapt.c`. Stage 7 still commits and returns `GEN_MIDI_ERR_RESTORE`.
- Static `cfw_ram` (256 bytes, full).
- The code-cave end `0x60190072`.
- Portable 128-step entry points. The native session still rejects 65 and 128.

## Known limits to carry forward

- Extension left: 3,944 of 65,536. Prefer to keep at least 2,048.
- Heap object is 3,124 bytes. The free-heap figure was not measured. Do not add another object without a measurement.
- Stack high-water was not measured. The largest estimated nesting is Apply at about 2,136 bytes and mutation preview at about 1,832, from own-frame sums. The target stack size is unknown.
- Learned models have no on-screen degree count and no matrix editor.
- A phrase whose length is a multiple of the Turing register, with evolution 0, returns the register to the same bytes as a fresh init. Reset is still a real reinit; use a phrase length that does not wrap if the test needs to see the bytes change.
- `test_tools.py` as a whole still needs `stock_firmware/PolyendPlay_v1.6.0.ppf`, which is not in the tree. Do not substitute another firmware.
- Clang `make host-asan` still cannot find `libclang_rt.asan`. GCC ASan/UBSan is the run that executed.
