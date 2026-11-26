# State and preview contract

The portable engine does not know about the native preview or Apply transaction. G5C keeps that transaction. These generators only make the copy, the proposal, and the discard explicit.

## Turing

`gen_turing_generate` takes an explicit source state. It does not write that state. It appends events to the caller’s buffer and writes the proposed machine into `next`.

Repeated Generate and Regenerate use the state the caller passes. Calling generate twice with the same source and the same playback configuration produces the same events and the same `next`. The function does not keep a hidden preview.

Discard is dropping `next`. Commit, after a successful Apply, is copying `next` over the committed `gen_turing`. Until that copy, the next Generate still starts from the committed state.

`gen_turing_copy` is a word copy of the 100-byte state. A copied machine does not share cells or RNG state with the original.

`gen_turing_reset` rebuilds the register from the configuration stored by the last successful init or seed. It does not depend on how far playback has walked.

## Markov

The model is caller-owned and `const` during generate. There is no playback cursor inside the model. A preview is another call with the same model and configuration. Discarding the events leaves the model as it was. Learning publishes into the destination model only after the scratch count succeeds, so a failed learn is itself a discard.

## Mutation

There is no generator state beyond the request and the seed inside it. The source pattern is immutable. A preview is the destination buffer. Discard is ignoring that buffer. Nothing in the call needs to be committed back into the source.

## What G5C should hold

- One committed `gen_turing` (100 bytes) if the Turing page can evolve across Apply.
- One `gen_turing` on the stack, or a second static slot, for `next` during preview. Do not point both at the committed object.
- One `gen_markov_model` (292 bytes) for the selected style or the learned table, plus a second 292-byte model only while Learn is running.
- One `gen_mutate_req` (56 bytes). No extra scratch.
- The existing preview event buffer. Capacity must be at least the requested step count, and at least `dest_steps` for mutation, up to 64 for a native destination.

Apply stays the only writer of the native sequencer. A discarded preview must not copy `next` and must not call the native writer.
