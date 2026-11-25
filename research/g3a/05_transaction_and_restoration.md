# 05 — Transaction and Restoration

## Edit policies

| Policy | Behavior |
|---|---|
| `EMPTY_ONLY` (default) | Reject if any step in destination region is on |
| `REPLACE_REGION` | Clear region after snap, then write; restore on failure |
| `MERGE` | Deferred (`UNSUPPORTED`) |

Pattern length is never auto-changed. Other tracks untouched. No channel/CC writes.

## Native undo

`cfw_edit_begin` → `fw_undo_push` still used as the stock edit recipe. Soft Undo UI for MIDI batches remains **unverified** (G1B). Do not advertise native batch undo as working.

## Genesis snapshot

Caller-owned `gen_snap` (max 64 steps): occupancy + note + vel + dur for the affected region. Captured before first write. Restore via verified read/write path. Stale magic / wrong track / out-of-range region → `RESTORE` error.

Because the adapter only modifies those verified fields, snapshot restoration matches what Apply changes.

## Failure injection

| Stage | Outcome |
|---|---|
| Before/after snap, before write | Error; no native change |
| After first write / mid / before commit | Error; Genesis restore → prior state |
| During restore | `RESTORE`; writes left → recovery-required |

Apply is **not** claimed atomic against power loss; with injection + restore it is experimentally clean for the tested stages.

## EMPTY_ONLY and inactive steps

Empty occupancy alone does not make inactive step attributes disposable. Snapshot captures occupancy plus note/vel/dur when locked. Off steps often have unlocked params (`getParam` → none); restore turns steps off and does not invent locked values. Verified: after EMPTY_ONLY Apply + restore, region occupancy matches the pre-Apply empty state; only generated steps were activated during Apply.
