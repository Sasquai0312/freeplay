# Native compatibility

The adapter file `cfw/src/genesis_adapt.c` is unchanged from G5A. These constraints are still the ones the Evolution page feeds.

## Destination

`genesis_midi_validate_track` accepts a track only when the step vtable reports `(type & 0xff) == 1` and `inst_of` returns 0. Anything else is `GEN_MIDI_ERR_BAD_TRACK`, before a write. `do_generate` calls `refresh_track` first and toasts “Need MIDI track” when that fails. There is no native channel write. Events are built with `channel = 0`.

## Length

`NUM_STEPS` is 64. The Length knob maximum is `native_len`, which the G5A track read clamps to 1–64. `gen_evo_native_steps` accepts 1, 16, 32, and 64 and rejects 0, 65, and 128. A direct portable call can still request 128 steps. The native session and the knob do not offer that.

Caps bits from `genesis_midi_caps` are unchanged: monophonic, the existing timing bits, and `GEN_MIDI_CAP_LEN16 | LEN32 | LEN64`. There is no 128-step cap bit.

## Events

`gen_event` remains 12 bytes: `tick` u32, `note` u8, `velocity` u8, `channel` u8, `_pad` u8, `duration` u32. The host print in `tests/test_evolution` reports `sizeof event=12`.

`gen_evo_audit` requires:

- note 0..127 and velocity 1..127
- `tick` divisible by `ticks_per_step` (480 in the UI)
- the step index inside the phrase
- `gen_native_duration_ok`
- `tick + duration` inside the phrase
- `gen_buf_validate(..., 1)` monophonic, and `count` not above the phrase or 64

`publish` drops the output count when audit fails, so a bad event does not become a VALID preview. Note index 6, velocity index 0, and duration index 1 are still the adapter’s parameter indices.

## Timing and gate

Native ticks per step stay 480. The smallest duration `gen_native_duration_ok` accepts at that rate is 4 ticks. Evolution gate is clamped to 4..480. A source duration that converts to a rejected tick count is `GEN_ERR_TIMING` during the read, not a clamped note. A duration that spans more than the remaining region fails audit.

## RNG

Only `gen_rng_seed` / `gen_rng_bound` / `gen_rng_range` (xorshift32). The UI passes `seed ? seed : 1`. The engine still maps a literal 0 seed to `0xA5A5A5A5`.

## Source immutability

Learn and mutation copy steps out with `genesis_midi_read_step` into `evo_src`. Preview does not call `genesis_midi_upsert` or `genesis_midi_remove`. Apply is the existing writer, and only after the preview is VALID and the destination still matches. The host session tests compare the source phrase before and after learn and after mutation, including rejected combinations.

## What is intentionally unchanged

- Monophonic output.
- Step-aligned note starts.
- Region is the span of the generated events, which is the phrase the UI just generated.
- Occupied destination plus EMPTY_ONLY asks for Replace instead of writing.
- Unsupported destinations are rejected in `preflight` / `validate_track` before modification.
