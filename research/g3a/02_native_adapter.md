# 02 — Native MIDI Adapter

## Files

| Path | Role |
|---|---|
| `cfw/src/genesis_adapt.h` | Public contract |
| `cfw/src/genesis_adapt.c` | Adapter + diagnostic fixtures |
| `cfw/src/genesis_aeabi.c` | Freestanding `__aeabi_memcpy*` / memset |

## Operations

- `genesis_midi_validate_track` — `type==1 && inst==0`
- `genesis_midi_pattern_length` — `TRK_VT_LENGTH`
- `genesis_midi_read_step` — occupancy + params 6/0/1
- `genesis_midi_ticks_to_step` / `ticks_to_dur` — portable ↔ native
- `genesis_midi_validate_events` — mono G2 validate + grid + channel policy
- `genesis_midi_begin` / `upsert` / `remove` / `commit` / `cancel`
- `genesis_midi_snapshot` / `restore` — Genesis-owned region snap
- `genesis_midi_apply_req` / `apply_ex` — full transaction
- `genesis_midi_preview_req` — read-only preview
- `genesis_diag_fixture` — generate fixture A–D then preview or apply

Native writes use only verified mechanisms: `cfw_edit_begin`, `fw_step_toggle`, `setParam` 6/0/1, `cfw_refresh_pads`, edit lock.

## Channel policy

Engine channels 0–15 are metadata. Adapter rejects mixed-channel buffers. No native channel parameter is written. Track routing is preserved.

## Result codes

See `gen_midi_err` in the header (`OK`, `BAD_TRACK`, `BAD_STEP`, `UNSUPPORTED`, `CAPACITY`, `TIMING`, `OCCUPIED`, `MONO`, `CHANNEL`, `RESTORE`, …).

## Request wrappers (vdev `call_fn`)

Unicorn `call_fn` only loads **r0–r3**. Multi-argument Apply/preview/fixture entry points are therefore exposed as **pointer-to-request** APIs. This is not a native ARM arity limit — convenience multi-arg wrappers remain for in-firmware callers.

| Struct | Size (Thumb) | Purpose |
|---|---|---|
| `gen_fixture_req` | 32 | Generate fixture then preview or apply |
| `gen_gen_req` | 20 | Generate-only into caller buffer |
| `gen_apply_req` | — | Apply portable events |
| `gen_preview_req` | — | Build read-only preview |

`_Static_assert` checks offsets/sizes in `genesis_adapt.c`. Host G2 direct generation matches firmware `genesis_diag_generate` via `call_fn` for fixtures A/B/D (same seed → identical events). Invalid/NULL/oversize requests rejected.
