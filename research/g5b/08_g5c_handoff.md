# G5C integration handoff

G5B stops at the portable API. G5C may call these functions from the existing preview and Apply path. It should not add a second sequencer writer.

One Evolution Lab screen is the size-conscious UI: a mode control selects Turing, Markov, or Mutation, and the shared page draws only that mode’s knobs. Three separate screens would duplicate the preview, Apply, and length widgets already added in G5A.

## Native event rules the generators already meet

When the caller asks for a native-compatible phrase:

- Step count is 1–64. Portable callers may still pass up to 128. The engine will not refuse 128, and G5C must not offer 128 on device.
- Events start on `step * ticks_per_step` and stay inside the region.
- Gate passes `gen_native_duration_ok`: ticks in `1 .. ticks_per_step`, native duration `(ticks * 60 + tps/2) / tps` in `1 .. 3840`. At 480 ticks per step the smallest passing gate is 4.
- One note starts on a step. Durations do not overlap. `gen_buf_validate(buf, 1)` holds.
- Notes are `0..127` and inside the requested scale and pitch bounds. Velocity is `1..127`.
- The event `channel` field is informational. Native Apply still does not write a MIDI channel. External MIDI remains `type == 1 && inst == 0`. Note index 6, velocity index 0, duration index 1. Those indices are not changed here.

G5C can Apply the buffer as generated. It should not quantize, truncate, or drop events to make them fit. If the user asks for more notes than the track length, ask for fewer steps before calling the generator.

## Ownership

| Object | Bytes | Who allocates | Lifetime |
| --- | ---: | --- | --- |
| `gen_turing` committed | 100 | UI / static | Across preview and Apply |
| `gen_turing` proposal | 100 | Stack for the call | Discard by not copying it back |
| `gen_markov_model` | 292 | UI / static | Until style or Learn changes |
| Learn scratch model | 292 | Stack or static, only during Learn | Not required to generate |
| `gen_mutate_req` | 56 | Stack | One call |
| Output events | 12 × steps | Existing preview buffer | Until Apply or discard |

`gen_turing_generate(src, cfg, out, next)` reads `src` and writes `next`. Preview uses a copy, or simply passes the committed state as `src` and ignores `next` until Apply succeeds. Commit is `gen_turing_copy` of `next` onto the committed slot after Apply returns success. Regenerate from the committed slot repeats the same phrase.

Markov generate does not modify the model. Mutation does not modify the source. The native read adapter from G5A remains the way to obtain that source. G5B does not read the sequencer.

## Recommended starting values

The structs have no hidden defaults. Zero-fill is not a valid Turing or Markov configuration, because gate 0 fails the native duration check. A zero-filled mutation request is an identity copy once `dest_steps`, `ticks_per_step`, scale, root, and pitch bounds are set.

| Control | Suggested start |
| --- | --- |
| Root / scale | C, natural minor |
| Pitch range | 48–72 |
| Steps | The track’s `TRK_VT_LENGTH` (1–64) |
| Ticks per step / gate | 480 / 360 |
| Velocity | 100 |
| Seed | 1 |
| Turing length / evolve / rest / lock | 8 / 0 / 0 / off |
| Markov style / degree / octave / rest | Stepwise / 0 / on / 0 |
| Mutation probabilities | 0 |
| Mutation intensity | 2 |
| Mutation velocity bounds | 1–127 |
| Mutation gate bounds | 240–360, used only if gate probability is nonzero |

## Display labels

Shared: Root, Scale, Low, High, Steps, Seed, Velocity, Gate.

Turing: Length, Evolve, Rest, Lock.

Markov: Style (Stepwise, Motif, Bassline, Exploratory, Learned), Degree, Octave, Rest. A Learn action, not a knob, fills the model from the current track.

Mutation: Pitch, Rhythm, Velocity, Gate, Insert, Remove, Quantize, Octave, Reverse, Rotate, Intensity. Protect toggles for pitch, rhythm, velocity, duration, and the selected step.

## Errors to surface

| Code | Meaning on this page |
| --- | --- |
| `GEN_OK` | Preview is ready |
| `GEN_ERR_PARAM` | A knob is out of range, the buffer was not cleared, or mutation aliased the source |
| `GEN_ERR_PITCH` | Range or octave cannot hold the notes |
| `GEN_ERR_TIMING` | Gate or grid is illegal |
| `GEN_ERR_CAPACITY` | Preview buffer shorter than the step count |
| `GEN_ERR_UNSUPPORTED` | Reverse or rotate was asked while a step or the rhythm is protected |
| `GEN_ERR_MONO` | Source notes overlap or share a step |
| `GEN_ERR_EMPTY` | Learn was given no notes |
| `GEN_ERR_OVERFLOW` | A learned weight is already 65535 |

## Unsupported

Aliased mutation buffers. Reverse or rotate together with step or rhythm protection. Native 128-step patterns. Polyphony. Multi-track Apply. Writing the MIDI channel. A gate longer than one step. Real-time scheduling. The portable engine will still generate a 128-step buffer if asked; the device UI should not ask.

## Complexity

The generators fill a `gen_buf` the existing Apply path already consumes. The new native work is the shared page, the three parameter blocks, the Turing commit after a successful Apply, and the Markov model slot. Learned mode can ship in the same page: the model is 292 bytes of RAM plus the 1907-byte Markov object already measured, not a second algorithm.

Do not treat 5607 bytes of object code as free extension space. G5A left 15700 bytes in the extension and 2510 bytes in the code cave. UI and the call glue land on top of the 5607, and `.ARM.exidx` must be checked in the link map.
