# 06 — Memory and Performance

## Host struct sizes

| Type | Bytes |
|---|---|
| `gen_event` | 12 |
| `gen_buf` | 16 |
| `gen_rng` | 4 |

Working memory: caller buffers only (typical demo ≤ 512 events ≈ 6 KiB). Stack: small fixed arrays (≤ ~128–256 bytes in generators).

## ARM Thumb `-Os` (clang thumbv7em, freestanding compile)

| Module | text+rodata |
|---|---|
| event | 570 |
| random | 174 |
| scales | 597 |
| rhythm | 668 |
| melody | 898 |
| chords | 1376 |
| arpeggiator | 1016 |
| transform | 908 |
| **Total** | **6207** |

Design target ≤ 12 KiB: **met** at object-sum level. Full firmware link with FreePlay cave/ext not performed (G2 prohibition) — decision notes target-size as measured for modules, not final linked CFW image.

## Genesis v0.1 subset (recommended)

`event + random + scales + rhythm + melody + arpeggiator` ≈ **3923 bytes** text+rodata. Defer `chords`/`transform` if flash pressure appears during G3.
