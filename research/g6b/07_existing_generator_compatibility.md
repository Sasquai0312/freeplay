# Existing generator compatibility

| Situation | Behavior |
|---|---|
| New monophonic note on an empty step | Upsert, then locked None. A later read cannot turn it into a chord. |
| Replace a chord step with a monophonic note | Confirmed `REPLACE_REGION` writes the note and locked None. `EMPTY_ONLY` does not touch the step and offers replace. |
| Replace a monophonic step with a chord | Confirmed replace writes the root and the preview id. |
| Replace one chord with another | Confirmed replace writes the new id. Readback checks it. |
| Clear an occupied chord hole | Replace writes locked None and turns the step off. |
| Mutate a chord-bearing phrase | Rejected. See below. |
| Cancel a preview | No native write. |
| Failed transaction | Restore as in `03_parameter_lock_contract.md`. If velocity cannot come back, recovery stays set. |

`EMPTY_ONLY` is still the default. `REPLACE_REGION` runs only after the confirm screen and a second Apply. One-shot Apply still clears the preview. A stale fingerprint or a changed pattern id is rejected. An unrelated track is not in the snapshot and was unchanged in the native run (sample track case 27) and in the G3B session check.

Melody on a track that already held chords offered replace and left every chord byte in place (UI case 28). After the phrase was cleared, Melody wrote steps whose chord parameter read 0 with bit 10 set.

## Mutation

`phrase_chord` walks the phrase. A locked id other than 0, including on a step that is off, is chord-bearing. The lab toasts `Has chords` and returns before `generate_evo`. It does not call `gen_evo_discard`, so committed Turing and Markov state stay. A read failure toasts `Chord read` and also writes nothing.

Locked None is monophonic. Mutation may run.

This is not full chord-aware Mutation. Moving or rewriting a chord event is not implemented. The 100-cycle run visited Mutate and did not clear recovery, and the evolution phase, pending, and ready bytes at the end of `gen_evo` stayed 0.
