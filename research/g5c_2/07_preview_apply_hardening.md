# Preview and Apply hardening

## Lifecycle

The session phases are unchanged: EMPTY, VALID, STALE, ERROR, APPLIED.

| Action | Result |
|---|---|
| Click, success | VALID, preview events stored, native pattern untouched |
| Click again, same inputs | Same events. Turing committed bytes stay put after the first init. |
| Turn a parameter, or Shift+turn during a preview | `invalidate`: proposal dropped, phase EMPTY unless already APPLIED, screen back to config |
| Change Engine | `gen_evo_set_mode` clears the proposal and a pending learn. Committed Turing and the published model stay. |
| Bottom while VALID and the bind matches | One native Apply. `gen_evo_apply_result(..., 1)` only on `GEN_MIDI_OK`. |
| Bottom again | `Generate first`. The previous preview is not written twice. |
| Occupied region, empty-only policy | Replace confirmation. Session is not committed. |
| Back from a preview or an error | Stays in Genesis, in config. |
| Back from config | Leaves to the stock UI. |

`evo_bind` covers mode, seed, lengths, track, harmony, density, gate, Turing knobs, Markov style, mutation percents, signed rotate, attribute bits, `pstep`, the pattern id, and a live re-read of each step. Apply calls `gen_evo_set_live` with that bind and refuses unless the phase is VALID, before any native write.

## Restore failure

`genesis_adapt.c` was not modified. Two paths return `GEN_MIDI_ERR_RESTORE`:

1. Diagnostic stage 7, `GEN_FAIL_DURING_RESTORE`. The writer has already committed the new notes and then returns the error without restoring. The destination is fully modified.
2. `fail_restore` when `genesis_midi_restore` itself fails. That path calls `genesis_midi_cancel`. The destination is indeterminate from the return code alone.

The UI cannot tell those apart. Any `GEN_MIDI_ERR_RESTORE` is handled as “the pattern may have changed”:

- `recovery` is set
- `invalidate` drops the preview and does not call `gen_evo_apply_result`, so Turing does not advance and a pending model is not published
- the screen says `MAY DIFFER` and the toast is `Pattern may differ`
- Top, Bottom, and Click stay blocked. Bottom toasts `Recovery required`
- nothing retries Apply
- Shift+Back calls `refresh_track`. If the track is still a MIDI track, recovery clears, the preview stays invalid, and the toast is `Check pattern`. The user has to generate again. If the track is not MIDI, recovery stays and the toast is `Need MIDI track`
- Unshifted Back leaves the lab and leaves recovery set. Reopening still blocks Apply

The virtual-device run set `diag_fail_at` to 7 on an empty region, observed the toast and the lock, left with unshifted Back, reopened, confirmed the lock, then cleared it with Shift+Back.

There is still no atomic native write. Stage 7 commits. A restore that fails inside `genesis_midi_restore` is not rolled forward or back by this phase. Readback is the existing `refresh_track` on Shift+Back, not a new writer.
