# 08 — Memory and Build Constraints

Play+ 1.3.0. All sizes from `target.json` + published `patches/PlayPlus.patch.json` static analysis.  
**Do not treat region capacity as free capacity.**

## Region capacities vs utilization

| Region | Capacity | Observed utilization (upstream FreePlay 0.1b patch) | Headroom estimate | Class |
|---|---|---|---|---|
| Cave `0x6018ca40`–`0x60190a40` | 16384 B | Cave start blob **13158 B** in patch JSON | ~**3226 B** | VERIFIED_STATIC_ANALYSIS |
| Ext `0x60190b00`–`0x601a0b00` | 65536 B | Append blob **34712 B** (magic `CFWX`) | ~**30824 B** | VERIFIED_STATIC_ANALYSIS |
| `cfw_ram` `0x20002700`–`0x20002800` | 256 B | `sizeof(struct cfw_state) ≤ 256` asserted; documented **full** | **~0 B** for new fields | VERIFIED_SOURCE |
| In-place hook patches | — | 24 entries, **13250 B** total patch payload (includes cave) | — | VERIFIED_STATIC_ANALYSIS |

Notes:

- Cave blob length is the contiguous write at cave start; remaining cave must stay fill-compatible with verifier.  
- Ext size includes header + checksum region; usable code < 34712 until rebuilt.  
- Genesis should prefer **ext** for new code; keep cave trampolines minimal.  
- Larger runtime state: `fw_operator_new` heap once per boot (slicer/mod/pages pattern). RT heap is finite — INFERRED tight.

## Allocation mechanisms

| Mechanism | Lifetime | Class |
|---|---|---|
| `cfw_ram` scratch | Survives warm reset; layout-checked each boot | VERIFIED_SOURCE |
| `fw_operator_new` | Until reboot; pointers in heap/pages state | VERIFIED_SOURCE |
| Stack | Freestanding; avoid large frames in IRQ | VERIFIED_SOURCE / practice |
| Ext guard | `cfw_ext_ok()` magic+checksum | VERIFIED_SOURCE |

## Alignment

Thumb functions odd addresses in `symbols.ld`; cave/ext link via `link.py`. Class: VERIFIED_SOURCE.

## Stack / IRQ

Audio IRQ path: no alloc, no FS, short work (`DEVELOPING.md`). Class: VERIFIED_SOURCE.

## Genesis implication

Fixed `GenBuffer` (~few KB) fits comfortably in remaining **ext** code budget and/or heap for UI state. Do not enlarge `cfw_ram` struct without reclaiming fields.
