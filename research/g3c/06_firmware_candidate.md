# 06 — Firmware Candidate (Genesis v0.1)

## Identity

| Field | Value |
|---|---|
| Filename | `PlayPlus_FreePlay_01.ppf` (owner alias `PlayPlus_FreePlay_Genesis_v0_1.ppf`) |
| Size | 4236752 bytes |
| SHA-256 | `5dc172f7262d7bda29d9b250ddc09bd70265dd96321e2c35cc8eef34856c4f26` |
| Base firmware | `PlayPlus_v1.3.0.ppf` SHA `535a903d…4876` (4098017 B) |
| Source branch | `cursor/g3c-apply-workflow-f3f4` |
| Compiler | clang-18 (`Ubuntu clang version 18.1.3`) |

Staged offline copy (not in git): `/opt/cursor/artifacts/PlayPlus_FreePlay_Genesis_v0_1.ppf`

## Build

```bash
export CFW_SKIP_LLD=1   # clang-18 ext layout vs ld.lld size mismatch (documented)
python cfw/tools/build.py cfw/targets/playplus-1.3.0/target.json \
  --out /tmp/fp-g3c --no-verify
```

`--no-verify`: disassembly literal false-positives; **emu_slice 43/43** still run as functional check.

Do **not** commit generated `.ppf`. Owner supplies stock `PlayPlus_v1.3.0.ppf`.
