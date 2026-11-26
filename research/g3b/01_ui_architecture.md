# 01 — UI Architecture

## Pattern

Reuses LFO / Slicer panel lifecycle:

1. Empty submenu builder `cfw_menu_build_genesis` registered on root Menu.
2. `sliceview` detects builder → `WMODE_GENESIS` (7).
3. Heap state via `cfw_pages_genesis` (`pages.c`).
4. Render / input / LED hooks in `sliceview` / `lfoui` / `mod` tick.

## Module

`cfw/src/genesis_ui.c` (+ `.h`) — single shared panel for all labs.

## States

| Code | Meaning |
|---|---|
| `ST_CFG` | Configuring (no native writes) |
| `ST_PREV` | Preview ready (`gen_preview.valid`) |
| `ST_ERR` | Generation / target error; preview invalid |

Transitions:

- Open → `ST_CFG`, clear preview.
- Generate OK → `ST_PREV`.
- Param / track / lab change → invalidate → `ST_CFG`.
- Back from preview → `ST_CFG`, drop preview.
- Exit panel → `lit=0`, release screen ptr; heap slot retained with magic (no leak across reopen).

No native sequencer writes in CFG / GENERATING / PREV / cancel / exit.

## Shared parameters

Track, length, seed, root, scale, velocity range, gate% — plus lab-specific fields gated by `param_visible`.

## Apply / recovery

Apply tile present but inert; toast `Apply in G3C`. Architecture leaves room for a future recovery-required gate (G3A `-11`) before any write path in G3C.
