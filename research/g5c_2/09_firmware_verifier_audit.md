# Firmware verifier audit

`cfw/tools/verify.py` flags a 4-byte literal when the value looks like a flash pointer (`0x60000000..0x70000000`), RAM (`0x1FFF0000..0x20080000`), or low memory (`>= 0x10000` and `< 0x100000`), unless it is already a known symbol or constant.

All three historical literals are data. Each one is in the frozen G5C.1 image and, for the first two, in the G5A image before Evolution existed. They were classified expected, named in `KNOWN_CONSTANTS`, and locked with `research/g5c_2/diagnostics/literal_check.py`.

## `0x10003` — expected

| | |
|---|---|
| Where | `.text.cfw_ext_ok` in `cfw/src/sliceview.c`. On the G5C.1 image the pool word is at `0x6018e67c`, loaded from `0x6018e63e`. |
| In the frozen baseline | Yes. G5A’s verify log listed it. |
| Role | `cfw_ext_ok` compares the extension header byte count before dividing by 4. `n = w[1] >> 2` is at most `CFW_EXT_MAX/4` (`0x4000`) when `w[1] <= 0x10003`. |
| Why it is flagged | `0x10003` is `>= 0x10000` and `< 0x100000`, so the low-memory rule matches. It is not a pointer. |
| Evidence | `(0x10000 // 4) * 4 + 3 == 0x10003`. The compare is the inclusive bound. |

## `0xffa50` — expected

| | |
|---|---|
| Where | `cfw_pages_render` in `cfw/src/pages.c`. G5C.1 pool address `0x60193afc`. The neighboring pool word is `0x40d005b0`. |
| In the frozen baseline | Yes. The same literal is in the G5A verify log. `pages.c` is not Evolution code. |
| Role | EVE display-list addend. `0x40d005b0 + 0xffa50 = 0x40E00000`, which is `DL_VERTEX2F(28 * 16, 0)`, the text column `TILE_TEXT_X`. |
| Why it is flagged | It sits in the low-memory window. It is an addend, not an address. |
| Evidence | `literal_check.py` recomputes the vertex word. `pages.c` already uses float for envelope curves; those pool words are a separate, older pattern and were not retouched. |

## `0x6e400002` — expected

| | |
|---|---|
| Where | `state()` in `cfw/src/genesis_ui.c`. G5C.1 image address `0x6019bfc0`, loaded and stored with `strd` into the new UI object. |
| In the frozen baseline | Yes, since G5C.1. It moved when the extension grew. It is not in the stock image. |
| Role | Four packed default bytes: scale 2 (natural minor), algorithm 0 (uniform), velocity min 64, velocity max 110. Seed 42 is a separate halfword. |
| Why it is flagged | `0x6e400002` falls in `0x60000000..0x70000000`, so it looks like flash. The bytes are `02 00 40 6e`. |
| Evidence | `struct.unpack('<I', bytes([2, 0, 64, 110])) == 0x6e400002`. The G5C.2 defaults are the same, so the literal is still emitted. |

## What was not done

No warning was silenced. The known-constant list still contains the previous entries, including `0x000F4240` and `0x000186A0` from G5C.1. The regression test imports `KNOWN_CONSTANTS` and checks the three formulas. It does not skip the verifier.

## Result on the G5C.2 image

```
.venv/bin/python cfw/tools/verify.py cfw/targets/playplus-1.3.0/target.json \
    /tmp/fp-g5c2/PlayPlus_FreePlay_01.ppf
```

Emulated slice tests inside that process: 43/43. Process exit code: 0. Result line: `PASS`.

`literal_check.py` also exited 0.
