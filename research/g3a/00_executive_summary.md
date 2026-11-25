# G3A — Executive Summary

**Decision: `NATIVE_ENGINE_INTEGRATION_READY`**

Native monophonic adapter links the G2 Genesis engine into Play+ FreePlay and applies generated MIDI into the verified G1B sequencer contract under Unicorn vdev. No physical hardware.

## Highlights

| Item | Result |
|---|---|
| Branch | `cursor/g3a-native-adapter-f3f4` from G1B `9a6c99b` + merge G2 `6de09c9` |
| Firmware build | Play+ FreePlay with Genesis subset + adapter in ext region |
| Fixtures A/B/D | Apply + native readback + Genesis snapshot restore |
| Fixture C (32) | Skipped: default pattern length 16 (per native limit) |
| Policies | `EMPTY_ONLY` (default), `REPLACE_REGION` (Genesis snap); `MERGE` deferred |
| Timing | `native_dur = round(ticks * 60 / ticks_per_step)`; off-grid rejected |
| Channel | Metadata only — never written |
| Preview | Read-only; no native mutation |
| Ext utilization | **42648 / 65536** (G1B 35914; Δ = 6734) |
| Cave | **13738 / 16384** |
| Integration harness | **42/42** (incl. ABI host↔`call_fn` equivalence) |
| FreePlay regressions | emu 43/43, tap 6/6, slicer 38/38, screen 23/23, mono 6/6 |

## Primary API

`genesis_midi_*` + request wrappers (`gen_apply_req`, `gen_fixture_req`, `gen_gen_req`) for vdev `call_fn` (4-register limit). See `02_native_adapter.md`.
