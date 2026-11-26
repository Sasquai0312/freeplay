# Handoff to G5C.2

G5C.1 is the functional integration. G5C.2 is visual refinement and control usability. Do not start it from this phase’s closeout, and do not flash the candidate.

## Keep

- One Evolution page. Do not split Turing, Markov, and Mutation into three screen implementations.
- The session in `genesis/engine/evolve.c`: preview does not commit, Apply commits once, cancel discards the proposal, reset does not write notes.
- G5B generator behavior, collision rules, and protection semantics. A full grid still cannot take an insertion or a displacement.
- Native limits: external MIDI `type == 1 && inst == 0`, phrase 1–64, monophonic, step-aligned, `gen_event` 12 bytes, xorshift32, no new static RAM, no malloc, no float.
- Portable 128-step entry points. The native session must keep rejecting 65 and 128.
- The existing Apply writer. It restores from a snapshot on failure, except diagnostic stage 7 (`GEN_FAIL_DURING_RESTORE`), which commits and returns `GEN_MIDI_ERR_RESTORE`. Do not describe that as a new atomic transaction.

## Usability that G5C.1 left on the existing one-knob page

- Negative rotate is in the API and not on the knob. The knob is 0..len−1.
- Step protection arms one step, or none. The API bitset can hold more. A knob value whose step is outside the current phrase is ignored.
- Attribute protection is one value 0–15 (pitch, rhythm, velocity, duration), not four switches.
- Density is `100 - dens` rests, hidden on Mutate. Turing and Markov velocity is V.min. V.max still limits mutation velocity.
- Evolution gate is clamped to 4..480 ticks. The other labs are not.
- Learned mode uses `gap_ticks = 0`. Out-of-scale notes still break the chain. An empty source toasts “Empty source” and does not fall back to a preset.
- `evo_bind` hashes mode, seed, lengths, track, harmony, density, gate, V.min, the Turing knobs, Markov style, the pattern id, and each source step. Mutation probabilities are dropped by `adjust` → `invalidate` rather than by a bind field.
- The page reuses the current display size and the existing label/value rows. It is readable enough to generate, preview, and apply. It is not a finished layout.

## Memory G5C.2 has to stay inside

Extension `0x60190b00..0x6019f424` uses 59,684 of 65,536. **5,852 bytes remain.** Cave is unchanged at 13,874 / 16,384 (2,510 left). Static `cfw_ram` is full. The UI heap object is 3,116 bytes. The deepest new stack contributor on a mutation is `gen_xform_mutate_pitch` at 552 bytes (`int pitches[128]`), on top of `do_generate` at 1,120.

## Tests G5C.2 should keep green

`cd genesis && make test`, including `tests/test_evo_session`. GCC ASan/UBSan is the sanitizer that links in this environment. Clang `make host-asan` still cannot find `libclang_rt.asan`. Native suites must boot a snapshot of the image under test. A snapshot from an older image faults with `UC_ERR_READ_UNMAPPED`.
