# Turing Machine

This is a looping shift-register-style sequencer written for Genesis. It is not a reproduction of any commercial module.

## Register

A cell is one byte. Values `0..127` are MIDI notes. `GEN_TURING_REST` (`0xFF`) is a rest. Rests live in the register, so a locked loop and a 0% evolution loop repeat pitch and rest together. Cells past `length` are zero. Length is 1–64. A phrase may be 1–128 steps; the register repeats until the phrase ends.

`gen_turing` is 100 bytes: the stored configuration (28), the xorshift state (4), 64 cells, length, position, and a ready flag. Host and Thumb sizes match.

## Drawing a cell

`rest_prob` 0 draws a scale pitch and does not roll for rest. `rest_prob` 100 stores a rest and does not roll for pitch. Any other probability rolls rest first, then a pitch. The pitch is the `k`-th scale member between `pitch_min` and `pitch_max`, chosen with `gen_rng_bound`. There is no 128-wide pitch table on the stack.

## Evolution

The cell is read, then the register may change, then the position advances by one modulo `length`.

- Evolution 0, or `locked` nonzero: the cell is not rewritten and no random number is consumed.
- Evolution 100 and not locked: every advance stores a new draw. The new value may equal the old one.
- Otherwise a draw replaces the cell when `gen_rng_bound(100) < evolve_prob`.

`gen_turing_reset` refills from the stored configuration, so the initial register returns. `gen_turing_seed` replaces the stored seed and refills. A failed `gen_turing_init` does not clobber a state that was already valid: configuration checks finish before the write.

## Output

Events start on `step * ticks_per_step`, use the configured velocity and channel, and use a gate that passes `gen_native_duration_ok` (gate is at most one step, native duration 1–3840). Output is monophonic because one cell sounds per step and the gate does not cross the next step.

Capacity is checked before any event is written and before `next` is published. An evolving register that can still produce notes needs room for `steps` events, because a rest cell may become a note. The exception is evolution 100 with rest probability 100: after its first visit a cell is a rest, so the sounding budget is `min(steps, length)`. A frozen register is unrolled and only the cells that are already notes are counted. A short buffer returns `GEN_ERR_CAPACITY` and leaves `next` unchanged.

A non-NULL playback configuration may change steps, gate, timing, velocity, evolution, rest, and lock for that call. It is not stored. Scale, root, pitch bounds, and register length must match the initialized register.
