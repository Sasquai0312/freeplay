# 06 — Preview Contract

`gen_preview` (see `genesis_adapt.h`):

- Target track, step0, pattern length
- Generator tag, seed, event count, policy
- Per-step occupancy / note / velocity / native duration
- `valid` flag after validation

Built via `genesis_midi_preview_req` / `genesis_diag_fixture` with `preview_out` set.

**Must not** modify native sequencer state. Cancellation = drop the preview buffer; no commit path.

Stale preview: caller must re-validate track/pattern identity and destination occupancy before Apply (G3B responsibility). G3A rejects Apply on occupied regions under `EMPTY_ONLY`.
