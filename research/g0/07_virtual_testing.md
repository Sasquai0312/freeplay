# 07 — Virtual Hardware and Testing

Pinned: FreePlay `5835b33`.

## What the virtual Play is

`cfw/vdev/vplay.py` boots a real `.ppf` (stock or FreePlay-patched) under Unicorn (ARMv8 AArch32 Thumb), with HLE for SD, USB stubs, LED DMA, audio block IRQ, and injected pad/button/knob/encoder events (`hle.py`, `session.py`, `flows.py`).

Not cycle-accurate; no claim of real CPU load or audible quality (`DEVELOPING.md` L92). Class: VERIFIED_SOURCE.

## Capability matrix

| Behavior | Supported in vdev? | Evidence | Class |
|---|---|---|---|
| Custom menu rendering | Yes — EVE DL capture / screenshots | `eve_render.py`, `vdev_*` shots | VERIFIED_SOURCE |
| Pad interaction | Yes — listener injection | `hle.py` input_slots; tests | VERIFIED_SOURCE |
| Encoder / knobs / buttons | Yes | `flows.py`, `vdev_pages.py` | VERIFIED_SOURCE |
| MIDI track state | Partially — firmware runs; CFW tests focus sample/synth/CC | `vdev_mod` CC path | VERIFIED_SOURCE (CC); UNKNOWN (MIDI step params exercised) |
| Sequencer data | Yes for sample edits (slicer tests) | `vdev_slicer.py` | VERIFIED_SOURCE |
| MIDI event capture | USB MIDI TX stubbed (skip if no device) | `hle.py` `usb_midi_tx` | VERIFIED_SOURCE (stub); not a capture harness |
| Clock simulation | Audio IRQ at ~344 Hz when enabled | `hle.enable_audio` | VERIFIED_SOURCE |
| Transport simulation | Play/Stop via buttons; seq flags readable | tests / symbols | VERIFIED_SOURCE |
| Automated regression | Yes — `vdev_*.py` checklist | `docs/DEVELOPING.md` table | VERIFIED_SOURCE |

## Upstream tests (commands)

Documented workflow (`docs/DEVELOPING.md`):

```bash
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
# place PlayPlus_v1.3.0.ppf at repo root
CC=clang .venv/bin/python cfw/build_all.py
mkdir -p /tmp/fp && cp cfw/build/playplus-1.3.0/*.ppf /tmp/fp/img.ppf \
  && cp cfw/build/playplus-1.3.0/link.map /tmp/fp/
VPLAY_DIR=/tmp/fp .venv/bin/python cfw/tests/vdev_slicer.py /tmp/fp/img.ppf
```

First boot ~10 minutes; then snapshots. Class: VERIFIED_SOURCE.

## G0 reproduction results

| Test | Command | Result | Class |
|---|---|---|---|
| Deps install | `pip install -r requirements.txt` | OK | VERIFIED_HOST_TEST |
| Thumb encode unit tests | `python cfw/tests/test_tools.py TestThumbFields` | 3 OK | VERIFIED_HOST_TEST |
| PPF unit tests | `test_tools.py TestPPF` | ERROR missing `PlayPlus_v1.3.0.ppf` / Play PPF | BLOCKED |
| Full build | `cfw/build_all.py` | Not attempted — no stock firmware | BLOCKED |
| Any `vdev_*.py` | — | Not run — needs built image | BLOCKED |
| clang Thumb smoke | compile empty add() | OK | VERIFIED_HOST_TEST |

## Behaviors requiring physical hardware

Upstream explicitly lists (`DEVELOPING.md`, README):

- Real CPU load / dropouts under heavy LFO.  
- Audible zipper noise.  
- True pad/screen colour appearance.  
- Physical knob feel.  
- Play RT / K66 real-device confirmation.  
- MIDI jack / USB to external synth validation.  
- Stuck-note behavior under transport stress.  

Class: VERIFIED_SOURCE (documented limits). Genesis Apply to external gear is **hardware-mandatory** after vdev structural tests.

## Genesis test plan (future)

1. Host: `genesis/tests` — Euclidean vectors, seed stability, clamp.  
2. vdev: open Genesis menu; set params; Preview LEDs; Apply; Undo; compare step memory.  
3. vdev: MIDI track discrimination (refuse synth).  
4. Hardware: Play+ → synth note/velocity/length audit.  

## Missing artifacts

| Artifact | Status |
|---|---|
| `PlayPlus_v1.3.0.ppf` | Absent — BLOCKED |
| `stock_firmware/PolyendPlay_v1.6.0.ppf` | Absent — BLOCKED |
| Built `cfw/build/playplus-1.3.0/` | Absent |
| vdev snapshots | Absent |
