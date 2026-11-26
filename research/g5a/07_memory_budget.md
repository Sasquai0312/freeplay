# Memory

Measured from `/tmp/fp-g5a/link.map` after the G5A link. `CFW_SKIP_LLD=1` skips only the ld.lld size compare.

| Region | v0.1 | G5A | Capacity | Left |
|---|---|---|---|---|
| Code cave `0x6018ca40` | 13874 | 13874 | 16384 | 2510 |
| Extension `0x60190b00` | 49328 | 49836 | 65536 | 15700 |
| Static `cfw_ram` `0x20002700` | 256 | 256 | 256 | 0 |

Extension delta is **+508 bytes**, inside the provisional +1024 target. Cave delta is 0. No new static CFW RAM. The image is 4238184 bytes, SHA-256 `efd5a1a6fc2d67b05f41ae22e2d6f6c58d6bac822e14c3bfe61acf9a8e6fcb17`.

## Runtime

One heap object (`fw_operator_new` of `struct genesis_ui`) holds the preview. The 16-bit occupancy mask became a 64-byte fingerprint. Event storage is the existing `events[64]` (768 bytes) plus the preview arrays. There is not a second copy of the generated events.

Worst stack during Apply is the UI `gen_snap` (16 + 64×8 = 528 bytes) plus the adapter’s fallback `gen_snap` of the same size. Both are fixed at 64 steps, including when the destination is 16 or 32. Generate’s caller-owned `gen_event[64]` is about 768 bytes and was already there for the 64-event ceiling. Nothing in G5A allocates a 128-step snapshot.

The fingerprint and the snapshot are different records: one is the pre-preview occupancy used to detect a stale destination, the other is the pre-write undo of the edited region.

Available extension bytes are not treated as free heap. The only Genesis heap block is that one UI object. Snapshot memory is stack.
