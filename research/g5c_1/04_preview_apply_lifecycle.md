# Preview and Apply lifecycle

The session phase is the preview lifecycle. Names match the requested set.

| Phase | Meaning |
|---|---|
| `GEN_EVO_EMPTY` | No preview. Reset, cancel, mode change, and `invalidate` land here unless the phase is already APPLIED. |
| `GEN_EVO_VALID` | `publish` accepted a non-empty buffer that passed `gen_evo_audit`. `preview_bind` equals the bind captured at preview. |
| `GEN_EVO_STALE` | `gen_evo_set_live` saw a different bind while the phase was VALID. |
| `GEN_EVO_ERROR` | The last preview failed. `proposed_ok` is cleared. |
| `GEN_EVO_APPLIED` | A successful native Apply already committed this preview. A second `gen_evo_apply_result(e, 1)` returns `GEN_ERR_PARAM` and does not copy the proposed register again. |

`publish` rejects a zero-length buffer as `GEN_ERR_EMPTY` and rejects an audit failure after clearing `out->count`. A failed preview therefore cannot be applied as a partial buffer.

## What a preview is bound to

`evo_bind` is an FNV-1a over mode, seed, phrase length, native length, track, root, scale, pitch ends, density, gate, V.min, the Turing knobs, Markov style and octave motion, `pattern_now`, and a fresh `genesis_midi_read_step` of each step in the phrase. `do_apply` recomputes it and calls `gen_evo_set_live` before any native write. A changed source or destination makes the phase STALE. Apply then toasts “Stale preview”, calls `invalidate`, and returns `GEN_MIDI_ERR_BUSY` without writing.

Knob edits go through `adjust`, which calls `invalidate` and therefore `gen_evo_discard`. Mutation probabilities are covered by that path. They are not separate fields inside `evo_bind`. A discarded preview has phase EMPTY, and Apply rejects it.

The preview is also the existing `gen_preview` plus `g->events[]`. `do_generate` copies the engine buffer into `g->events` only after `genesis_midi_preview_build` succeeds. The fail path discards the session, clears `n_ev`, and sets `ST_ERR`. A native preview that fails after a successful learn drops `pending_ok`, so the learned weights are not published.

## Apply order

For the Evolution lab, `do_apply` does this:

1. Recompute the live bind. If the phase is not VALID, stop before any native write.
2. Refuse when `recovery` is set (existing “Recovery required” path).
3. Run the existing `preflight_apply` (destination, stale fingerprint, parameter checks).
4. Call the existing `genesis_midi_apply_ex`.
5. Only when that returns `GEN_MIDI_OK`, call `gen_evo_apply_result(&g->evo, 1)`.

`gen_evo_apply_result` with `native_ok == 0` returns OK and leaves the committed Turing register and the published Markov model unchanged. The phase stays VALID so a later successful Apply of the same preview can commit once. Occupied + EMPTY_ONLY still offers Replace and does not commit. A failed restore sets `recovery`, invalidates, and does not commit.

On success the Turing branch copies `proposed` onto `committed` only when `mode == TURING` and `proposed_ok`. The Markov branch copies `pending` onto `model` only when `mode == MARKOV` and `pending_ok`. Mutation has nothing to commit. The phase becomes APPLIED and both flags are cleared. A second Apply finds a cleared preview (`n_ev == 0`, phase APPLIED) and does not advance the register again.

A mode change clears `proposed_ok` and `pending_ok` before any Apply, so Apply cannot commit a different mode’s proposal.

## Native destination rollback, as it already exists

`genesis_midi_apply_ex` snapshots the edited region, writes step by step, and on a write failure calls `genesis_midi_restore`. That is the G3C/G5A behavior. It is not a single atomic store of the whole pattern.

The diagnostic stage `GEN_FAIL_DURING_RESTORE` (fail index 7) commits the writes and returns `GEN_MIDI_ERR_RESTORE` without restoring. The UI sets `recovery` and does not call `gen_evo_apply_result(e, 1)`, so the Evolution session does not advance. The notes from that diagnostic path stay written. That limitation is pre-existing. G5C.1 does not claim a new atomic firmware writer.

If `gen_evo_apply_result` itself returned an error after `GEN_MIDI_OK`, the notes would already be committed and the UI would toast “State not saved”, mark the preview consumed, and not advance a second time. The phase is checked VALID before the native write and the write does not change the phase, so that branch is a guard rather than an expected outcome.

## Reset and cancel

Reset (`P_RESET`) calls `gen_evo_turing_reinit` and then `invalidate`. Reinit replaces the committed register from the current configuration and writes no events. Invalidate drops any preview. The native pattern is not written.

Cancel is `gen_evo_discard`: clear `proposed_ok` and `pending_ok`, set EMPTY unless already APPLIED, leave the committed register and the published model in place. The UI calls it from `invalidate` (knob change, lab change, seed regenerate, stale Apply, generate failure).
