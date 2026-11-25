# 07 — MIDI Output Observability

## Virtual stubs

`cfw/vdev/hle.py` installs `usb_midi_tx` (`0x600c6688`): if `usb_midi_dev == 0`, the send is skipped (return to LR).

## Experiment

Diagnostic re-hook logged entry to `usb_midi_tx` during Play with MIDI trigs present.

| Observation | Value |
|---|---|
| `usb_midi_dev` | 0 |
| TX entries captured | **0** |
| Physical MIDI | Impossible in vdev |

## Distinctions

| Layer | Status in G1B |
|---|---|
| **A. Sequencer storage** | **Verified** (getParam / is_on) |
| **B. Virtual event dispatch** | **Not observed** — TX not entered with stubbed USB device |
| **C. Physical MIDI output** | **Cannot** establish |

## Diagnostic capture (non-production)

A host-side hook on `usb_midi_tx` can log register args without CFW source changes. Enabling a fake `usb_midi_dev` to run the real TX body was **not** done (risk of unmodeled USB paths). Optional future work: HLE that records intended packet buffers when the firmware prepares a send, still without claiming physical I/O.
