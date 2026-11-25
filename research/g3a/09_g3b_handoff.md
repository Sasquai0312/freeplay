# 09 — G3B Handoff

## Ready for G3B

1. Linked engine + adapter in FreePlay ext region.
2. `gen_preview` read-only contract for UI presentation.
3. `genesis_diag_fixture` / `genesis_midi_apply_req` for Apply behind a future menu.
4. Default policy `EMPTY_ONLY`; `REPLACE_REGION` only with snap capacity.

## G3B should

- Add Genesis menu/screen (production UI) using existing FreePlay menu patterns.
- Present preview without writes; cancel drops preview.
- Re-validate track/pattern/occupancy before Apply (reject stale previews).
- Keep generation off the audio IRQ; keep buffers caller-owned.
- Not enable polyphonic Apply until a separate verification phase.
- Not write MIDI channel/CC.

## Do not

- Flash or run on physical hardware in G3B research without an explicit hardware phase.
- Begin G3C until G3B UI contract is accepted.
