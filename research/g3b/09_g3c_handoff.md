# 09 — G3C Handoff

## Ready for G3C

1. Production UI + non-destructive preview (`NATIVE_GENESIS_UI_READY_RESTRICTED`).
2. G3A adapter still linked; diag Apply path unchanged for regression.
3. Apply tile / toast already reserved (`Apply in G3C`).
4. Edit policies preserved: `EMPTY_ONLY`, `REPLACE_REGION`; MERGE deferred.
5. Ext remaining ≈ **17.6 KiB**; cave ≈ **2.5 KiB** for transaction UI + recovery UX.

## G3C should

- Wire verified Apply behind confirmation; never from unverified paths.
- Re-validate track / pattern / occupancy / seed before write; reject stale previews.
- Surface recovery-required (`-11`) clearly; block further Apply until verified recovery.
- Keep generation off audio IRQ; keep buffers caller-owned.
- Stay monophonic; no channel / CC writes.
- Optionally unlock 32-step destinations only after a verified length-write experiment.

## Do not

- Flash physical hardware in G3C research without an explicit hardware phase.
- Advertise native Undo until experimentally proven for Genesis Apply.
- Begin unrelated feature work that risks FreePlay regressions.
