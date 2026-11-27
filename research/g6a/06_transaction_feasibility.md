# Transaction and restoration feasibility

The current snapshot cannot hold a chord. `gen_native_step` stores `on`, `note`, `vel`, and `dur` (`cfw/src/genesis_adapt.h`). Parameter 10 is absent. An Apply that restored from that struct would drop the chord id. That is a silent drop of a real field, and G6B must not do it.

The field that has to be added is one signed halfword per step. `gen_native_step` is 8 bytes today. One more `s16` makes it 10. Across 64 steps that is 128 bytes. The snapshot is a stack local in `genesis_midi_apply` and in the UI, not static CFW RAM. Physical stack headroom is still unknown, so 128 bytes is a layout measurement, not a device measurement.

Restoration that was demonstrated, on the emulator, through `setParam`:

| Action | Chord id after | Note, velocity, duration |
|---|---|---|
| Fresh step | 0 | 60, 100, 60 |
| Set Major | 3 | unchanged |
| Set another id, including the clamp of 21 to 20 | the clamped id | unchanged |
| Set 0 | 0 | unchanged |
| Step 5 | still unread (off) | untouched |
| Sample track parameter 10 | unchanged | |

`EMPTY_ONLY` and `REPLACE_REGION` stay the G3C policies. A chord write is a write to an occupied step's parameter, so it belongs on the confirmed replace path, not on empty-only. The length check stays the G5A contract: reject a step outside 16, 32, or 64 before any `setParam`. This phase did not add 128-step support and did not change a pattern's length.

The G5C.3 power-on recovery lock is unchanged. This phase did not add an Apply path, so it did not exercise the lock. Native Undo is not atomic, and a power loss in the middle of a multi-step write is not atomic. G6B keeps the existing lock and the existing per-step restore.

Partial write: `setParam` updates one index and its lock bit. Restoring the chord restores that index only. The diagnostic restore did not need to rewrite note, velocity, or duration because those bytes did not change.
