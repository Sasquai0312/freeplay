# Mutation Lab

`gen_mutate` reads `src` and writes a different caller-owned buffer. `out->count` must be 0. If `out->events` aliases `src->events`, the call returns `GEN_ERR_PARAM` before any write. An empty source returns success with count 0.

The request is 56 bytes, including a 16-byte step mask. There is no caller-owned scratch buffer.

## Order

1. Validate. No output writes.
2. If reverse or rotate is requested while rhythm is protected, or while any step bit in `0 .. dest_steps-1` is set, return `GEN_ERR_UNSUPPORTED`. Nothing is mutated.
3. If the source is non-empty and `out->capacity < dest_steps`, return `GEN_ERR_CAPACITY` before writing. A removal that yields no events still requires that capacity. The monophonic ceiling is one event per destination step.
4. For each source note, in order: rhythmic displacement, removal, insertion, then quantize, octave, pitch, velocity, and gate. A removed note is not reshaped and does not spawn an insert.
5. Sort, then `gen_xform_reverse` if requested, then `gen_xform_rotate`.

Reverse, rotate, octave (`-2..2`), and scale quantize are unconditional. The probabilities do not gate them. Probability 0 with those four controls off copies the source onto the step grid.

Random draws for one note finish before the next note is visited.

## What each control does

- **Displace.** Intensity 0 or probability 0 consumes no roll. Otherwise the step moves by a nonzero amount in `[-intensity, intensity]`. A move that would leave the region, land on an occupied step, or land on a protected step is skipped. Occupancy updates immediately, first note to last. Notes are not overwritten.
- **Remove.** Same test as `gen_xform_remove`: `gen_rng_bound(100) < remove_prob`. Protected steps and `GEN_MUT_PROTECT_RHYTHM` are ineligible and do not consume a roll.
- **Insert.** The first free unprotected step at or after the next step, wrapping through `dest_steps`. No free step means the insert is skipped, including a full pattern with insert probability 100. The new pitch is the parent pitch plus two semitones, then `gen_scale_quantize` into range. Ties in quantization prefer the lower note. Duration, velocity, and channel come from the parent after its attribute passes.
- **Pitch.** With probability, add a nonzero semitone delta of ±intensity, then quantize into range. Intensity 0 consumes no roll.
- **Velocity.** `gen_xform_velocity` on that one event with delta `[-intensity, intensity]`, then clamp into `[vel_min, vel_max]`.
- **Gate.** Resample duration uniformly in `[gate_min, gate_max]`. Both bounds must pass `gen_native_duration_ok` when gate probability is nonzero.
- **Quantize.** Unconditional `gen_scale_quantize`. Notes outside the pitch bounds are pulled in. With quantize off, an out-of-range note returns `GEN_ERR_PITCH` before any write.
- **Octave.** Unconditional ±12 times `octave`. Every unprotected note, after an optional quantize, must land inside the pitch bounds, or the call returns `GEN_ERR_PITCH` before writing.

## Protection

Attribute bits: pitch `0x01`, rhythm `0x02`, velocity `0x04`, duration `0x08`. A protected attribute is left unchanged and does not consume a roll.

`protect_steps` bit `i` protects step `i`. Bits 0–63 are the native mask (destinations up to 64). Bits 64–127 are the portable extension and use the same array. Protection is on the original step index. A protected step’s event is not altered. Nothing else may move onto it. An empty protected step stays empty. Reverse and rotate are rejected for the whole request if any in-range bit is set, so a later geometric pass cannot move a protected step.

A note whose pitch is protected may sit outside the mutation pitch bounds. It still has to be a MIDI note `0..127`.

## Source cases

| Input | Result |
| --- | --- |
| Empty source | `GEN_OK`, count 0 |
| Sparse source | Free steps can receive a displace or an insert |
| Full source | Displace and insert that need a free step are skipped |
| Duplicate timestamps or overlapping durations | `GEN_ERR_MONO` before writing |
| Notes off the step grid, or a gate longer than one step | `GEN_ERR_TIMING` or `GEN_ERR_MONO` before writing |
| Channel | Copied from the source. The lab does not write a native MIDI channel |
| Failure after validation | `out->count` is set back to 0. The source bytes are unchanged |

Rotation uses the existing `gen_xform_rotate` span: each event end is snapped up to the step grid. That span is the source material’s span, which matches `dest_steps` when the source already fills the region.
