# G5B — Portable Evolution engine

Decision: **EVOLUTION_ENGINE_VALIDATED**

Three portable generators are implemented, tested, and measured. None of them is linked into firmware.

- Turing Machine: a 1–64 cell register of scale pitches and rests. Lock and 0% evolution repeat the register. Generate advances a caller-owned copy and leaves the committed state untouched.
- Markov Melody Lab: a first-order chain on scale degrees (at most 12), with four heuristic styles and a caller-owned learned model.
- Mutation Lab: one request that reuses `gen_xform_reverse`, `gen_xform_rotate`, and `gen_xform_velocity`. The source buffer is not written.

`gen_event` is still 12 bytes. The only random source is the existing xorshift32 (`gen_rng`). Seed 0 still remaps to `0xA5A5A5A5`.

Freestanding Thumb `-Os` code plus read-only data for the three new objects is **5607 bytes** (target about 5632). `.data` and `.bss` are 0. This is an object measurement, not a linked extension size.

Host tests, including the G2 suite and the G5A length tests, pass. GCC address and undefined-behavior sanitizers pass. Twelve Standard MIDI demonstrations parse with balanced note-on and note-off counts. Structural checks do not establish how the files sound.

Parent commit is G5A `802c4de`. No release tag was created. Native firmware was not rebuilt and was not installed.
