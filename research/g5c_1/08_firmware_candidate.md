# Firmware candidate

Offline only. This image was not flashed, installed, or executed on hardware. A successful link is not hardware compatibility, and this file is not a release.

## Command

```
CFW_SKIP_LLD=1 .venv/bin/python cfw/tools/build.py \
  cfw/targets/playplus-1.3.0/target.json --out /tmp/fp-g5c1 --no-verify
.venv/bin/python cfw/tools/verify.py \
  cfw/targets/playplus-1.3.0/target.json \
  /tmp/fp-g5c1/PlayPlus_FreePlay_01.ppf
```

`--no-verify` on the build keeps the three historical literal findings from aborting the write. `verify.py` was then run on the image that was written. Argument order is target JSON, then the `.ppf`.

## Hashes

| Artifact | SHA-256 |
|---|---|
| Input `PlayPlus_v1.3.0.ppf` | `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` |
| Candidate `/tmp/fp-g5c1/PlayPlus_FreePlay_01.ppf` | `1e302709a828e6dad8f4b635f5c5fac5a5583e08b63b6e04930f05be02a6f28c` |

Report: `/tmp/fp-g5c1/PlayPlus_FreePlay_01.md`. Link map: `/tmp/fp-g5c1/link.map`. Version string in the image sources: `1.3.0-FreePlay0.2b`.

The candidate is not in git. G5A’s image and the stock image were not overwritten.

## What verify checked

- Every record checksum parses.
- 889 stock records changed in data and checksum only. 3,731 records inserted at `0x60190b00..0x6019f424`. Every other stock line is byte-identical and in order.
- Bytes outside the cave and the hook slots: 0.
- Hook slots point at the cave functions named in the report.
- Emulated end-to-end tests (Unicorn, stock image plus this CFW): **43/43 passed**.
- Result line: **FAIL**, three literals that are not in the allow-list:

| Address | Literal | Status |
|---|---|---|
| `0x6018e67c` | `0x10003` | pre-existing, same value G5A reported |
| `0x60193afc` | `0xffa50` | pre-existing |
| `0x6019bfc0` | `0x6e400002` | pre-existing value; the address moved because the extension grew |

`0x000F4240` and `0x000186A0` from the Evolution objects are named in `KNOWN_CONSTANTS` and do not appear in the error list. The three historical findings were not “fixed” by editing unrelated code or by deleting a real constant.

## Integrity of unrelated regions

The report’s “bytes outside cave+hooks” count is 0. The bootloader note in `target.json` still applies: the patch does not include bytes below `0x60030000`. No firmware region was relocated. Static RAM bounds are unchanged.

## Runtime dependencies

External references listed by the report are the existing Play firmware symbols (`fw_operator_new`, `fw_toast_show`, step and track helpers, and the same hook targets as prior phases). The new objects add no libc allocator and no libm. See the `llvm-nm -u` table in `02_native_architecture.md`.

## Hardware

No flash, no install, no serial session, no device execution. Do not treat this candidate as a hardware-validated build.
