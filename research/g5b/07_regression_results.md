# Regression results

Command, from `genesis/`: `make test`.

- `tests/test_genesis`: **All Genesis tests passed.** This is the G2 suite plus the G5A portable capacity tests (`test_long_lengths`). PRNG, scales, Euclidean rhythm, melody, random walk, arpeggiator, chords, transformations, and the SMF writer are in that binary.
- `tests/test_evolution`: **All evolution tests passed.**

`gen_event` is unchanged. `sizeof(gen_event)` is 12. No second RNG was added.

## Evolution coverage

Turing: same seed, same state, varied seeds, lengths 1, 4, 8, 16, 32, and 64, phrase lengths 16, 32, 64, and 128, locked loop, 0% evolution, 100% evolution, rest 0 and 100, reset, copy independence, invalid state, invalid length, short buffer, pitch bounds, scale membership, monophonic output, and a long run that stays inside the 100-byte state.

Markov: preset weights, zero-weight cells, a single allowed transition, an all-zero row falling back to uniform, scale and range, empty and single-note learning, a known count fixture, repeated notes, out-of-scale notes, gaps, learned generation, overflow at 65535, lengths 16, 32, 64, and 128, short buffers, and monophonic output. The weighted draw uses seed 12345, weights 9 and 1 on degree 0 of a 7-degree scale, and 128 steps. The higher weight is selected often enough that `to1 * 100 > from0 * 75`, with more than 40 trials, and the lower weight still occurs.

Mutation: source bytes unchanged, deterministic repeat, zero-probability identity, pitch-only, rhythm-only, velocity-only, gate-only, protected attributes, protected steps including a portable step 70, insert capacity, removal, reverse, rotate, quantize, collision skip, empty, single note, a dense 64-step pattern, a 128-step portable pattern, invalid input, alias rejection, capacity error, and monophonic output.

## Property loops

Each generator runs seeds 1 through 100. Turing varies length, evolution, rest, and lock. Markov varies the four preset styles and rest on Dorian. Mutation varies pitch, displace, and velocity probability on an 8-note source. Each iteration checks ordering and grid timing, pitch and scale, monophonic validation, replay, and, for Turing and mutation, that the input state or source bytes do not change.

## Sanitizers

`make host-asan` uses the default `cc`, which is clang 18. The image does not contain `libclang_rt.asan`, so that make target fails at link.

The same sources were rebuilt with GCC 13.3 and `-fsanitize=address,undefined -fno-omit-frame-pointer`:

- `All Genesis tests passed`
- `All evolution tests passed`

No sanitizer diagnostic was reported. Clang still builds the unsanitized `-Werror` host tests.

## Not run

The native firmware test suite was not run. No Play firmware image was built. `cfw/tests/test_tools.py` was not run. Hardware was not used.
