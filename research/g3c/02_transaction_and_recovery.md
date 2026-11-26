# 02 — Transaction and Recovery

## Transaction

G3A path: validate → snapshot → `cfw_edit_begin` → write → readback path → commit. Failure stages 1–6 restore via `genesis_midi_restore`. Not claimed power-loss atomic.

## Recovery-required

Stage 7 / `GEN_MIDI_ERR_RESTORE` sets session latch `recovery` on heap UI state (`cfw_pages_genesis`).

- Blocks further Apply (and surfaces LOCK UI).
- Invalidates preview.
- Survives open/close within the boot (heap slot retained).
- Not written to flash / project storage.
- Cleared only via diagnostic `cfw_genesis_set_recovery(0)` or power cycle (musical data not claimed restored).

Owner: treat recovery-required as stop; reload disposable project / reboot; do not trust Undo.

## Native Undo

Soft Undo UI for MIDI batches remains **unverified**. Not advertised. Internal snapshot restore ≠ user Undo.
