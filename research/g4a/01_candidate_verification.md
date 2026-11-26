# 01 — Candidate Verification

## Artifacts

| File | Size | SHA-256 | Result |
|---|---:|---|---|
| `PlayPlus_v1.3.0.ppf` | 4098017 | `535a903d…4876` | Match |
| `/opt/cursor/artifacts/PlayPlus_FreePlay_Genesis_v0_1.ppf` | 4236752 | `5dc172f7…4f26` | Match |
| Rebuild `/tmp/fp-g4a-rebuild/PlayPlus_FreePlay_01.ppf` | 4236752 | same | **MATCH** |

Compiler: Ubuntu clang 18.1.3.

## Independent package checks (PPF parser + target.json)

| Check | Result |
|---|---|
| Device / base SHA | Play+, stock SHA matches `target.json` |
| Record parse + checksums | 96084 records valid |
| Stock lines unchanged except patches | 889 records changed (data+checksum); 3083 inserted |
| Code cave | `0x6018ca40..0x60190072` — **13874 / 16384** |
| Extension | `0x60190b00..0x6019cbb0` — **49328 / 65536** |
| Bytes outside cave + declared hooks | **0** |
| Static CFW RAM increase | **0** (scratch window unchanged) |
| Version string | `1.3.0-FreePlay0.1b` |
| Active failure-injection UI | None (no menu control) |

Diagnostic symbols `genesis_diag_fixture`, `genesis_diag_generate`, and `cfw_genesis_set_recovery` remain **linked** in this candidate. They are not exposed in the Genesis menu. The image that passed G3C tests **is** this candidate (not a separate stripped build).

## `verify.py` (run independently)

Checksums, hook resolution, and emu_slice **43/43** passed. Final status **FAIL** on three literals that are constants inside `.text`, not addresses:

| Address | Value | Section |
|---|---|---|
| `0x6018e67c` | `0x10003` | `.text.cfw_ext_ok` |
| `0x60193afc` | `0xffa50` | `.text.cfw_pages_render` (adjacent word is an IEEE high half) |
| `0x6019a308` | `0x6e400002` | `.text.state` |

Classified as a known disassembly allow-list limitation, not an unexplained integrity failure. No stray bytes, undecodable cave code, or hook mismatches.
