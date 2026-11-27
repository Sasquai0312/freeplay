# Recovery review

`genesis_adapt.c` was not modified. `GEN_MIDI_ERR_RESTORE` still has two sources.

| Path | Destination after the return |
|---|---|
| `fail_at == GEN_FAIL_DURING_RESTORE` | `genesis_midi_commit` has run. The new notes are written. The error does not mean the write was undone. |
| `genesis_midi_restore` itself fails | `genesis_midi_cancel` runs. That function returns without undoing notes. The destination is indeterminate from the code alone. |

`genesis_midi_commit` refreshes pads. It does not roll the pattern back. `cfw_edit_begin` pushes one undo snapshot (`fw_undo_push`). Per-step `cfw_edit_lock` / `cfw_edit_unlock` wrap each upsert and remove. A returned error does not by itself prove the audio-IRQ lock depth is zero, the undo snapshot matches the pre-edit pattern, or the step bytes match the snapshot.

Shift+Back in G5C.2 cleared the lock after `refresh_track`. That call checks that the track is external MIDI and reads the pattern length. It does not compare step notes, release a lock, or close a transaction. Those conditions are not established.

## Alpha 1 policy

Apply stays blocked for the rest of the power-on.

- The preview is invalidated. `gen_evo_apply_result` is not called, so Turing does not advance and a pending Markov model is not published.
- The toast is `Pattern may differ`. The banner is `MAY DIFFER`. Neither says the pattern was restored.
- Top, Bottom, and Click toast `Recovery required` and do not write.
- Unshifted Back leaves the lab and leaves the flag set. Reopening does not clear it. `cfw_genesis_enter` does not clear `recovery`.
- Shift+Back toasts `Recovery required` and does not clear the flag.
- Nothing retries the edit.

The flag lives on the one heap object. A power cycle drops that object. It does not undo notes already stored in the project. The owner stops the session, power-cycles only after saving notes of what was on the track, and rolls back the project from the fresh backup if the pattern is wrong.

The harness can still call `cfw_genesis_set_recovery(0)`. That symbol is not a menu row.

`fw_edit_lock_depth` (`0x2001daf4`) was 0 before and after a stage-7 Apply on the virtual device in the runtime probe. That is one emulator observation of the lock counter, not a proof that every native callee released every other resource.
