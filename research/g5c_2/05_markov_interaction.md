# Markov interaction

Four presets and Learn are the Style values: Step, Motif, Bass, Expl, Learn. The header line is the preset name when Style is not Learn.

## Learned states

| What you see | Session fields |
|---|---|
| Pending | `pending_ok` after a successful Learn preview |
| Learned | `model_ready` and the published model’s style is Learn, and the last status is OK |
| Error | last status is not OK (the generate-failure path discards the phase, so the phase is not `ERROR`) |
| No model | Learn is selected, nothing is pending, nothing is published, last status is OK |

A pending model is copied onto the published model only inside `gen_evo_apply_result` after `GEN_MIDI_OK`. The UI does not call that on a failed Apply. Switching to a preset uses the scratch model and clears `pending_ok`. It does not replace a learned model with a preset.

## Empty source

Learn with no notes toasts `No source notes`. The header shows Error. `pending_ok` and `model_ready` stay clear. Back returns to config and stays on the page. A following Step preview still generates from the preset. The virtual-device run covered that order.

`gap_ticks` is still 0. Out-of-scale notes still break the chain. There is no matrix editor and no learned-degree numeral. The model’s `n_deg` is available in the session object and was left off the screen to keep the extension inside the reserve.

Phrase length, density, scale, root, pitch range, V.min, gate, seed, preview, and Apply are the same shared controls as Turing.
