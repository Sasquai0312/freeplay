# Candidate integrity

Hashes were recomputed on this machine. The G5C.2 and v0.1 files were still in `/opt/cursor/artifacts/`. The stock file is `PlayPlus_v1.3.0.ppf` in the repository.

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| Stock `PlayPlus_v1.3.0.ppf` | 4,098,017 | `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` |
| Hardware-validated v0.1 `PlayPlus_FreePlay_Genesis_v0_1.ppf` | 4,236,752 | `5dc172f7262d7bda29d9b250ddc09bd70265dd96321e2c35cc8eef34856c4f26` |
| G5C.2 offline `/opt/cursor/artifacts/PlayPlus_FreePlay_G5C2_offline.ppf` | 4,271,251 | `bff1c17fa56cdd4fc79e5b69a9ee5df4a85008a80cd6f13cfdc80f94d6c806ac` |
| Alpha 1 rebuild `/tmp/fp-g5c3/PlayPlus_FreePlay_01.ppf` | 4,271,087 | `39a061780b16832de063d30a248233405be40378e2dffe9833ac15b8bf193228` |

The G5C.2 file in `/tmp/fp-g5c2/` matches the artifact hash above. v0.1 matches the frozen size and hash. Stock matches the frozen hash.

Alpha 1 is not byte-identical to G5C.2. The rebuild is the candidate. The difference is the recovery-policy change and the version byte `0.2c` to `0.2d`, described in `02_reproducible_build.md`.

`verify.py` on the Alpha 1 image exited 0. Record checksums passed. 889 stock records changed (data and checksum only). 3,846 extension records were inserted at `0x60190b00..0x6019fb60`. Bytes outside the cave and the hooks: 0. Emulated slice tests: 43/43. Result line: `PASS`. Log `/tmp/g5c3-verify.log`.

## Literals, checked on this image

| Literal | Class | Instruction context on Alpha 1 |
|---|---|---|
| `0x10003` | Expected | `cfw_ext_ok` at `0x6018e63e`: `ldr r4, [pc, #0x3c]` loads `0x6018e67c`, then `cmp r3, r4` against the header byte count. The following `lsrs r2, r3, #2` is the `/4`. |
| `0xffa50` | Expected | `cfw_pages_render`: `0x601937c2` loads `0x40d005b0` into `sb`. `0x60193830` loads `0xffa50`. A later `add r0, sb` / `add r1, r0` builds `0x40E00000`, which is `DL_VERTEX2F(28*16, 0)`. |
| `0x6e400002` | Expected | `state()` at `0x6019bf62` loads the word and `strd`s it into the new object at offset 8. The same function does `movw r0, #0xc34` (3,124) before `fw_operator_new`. |

`research/g5c_2/diagnostics/literal_check.py` still passes. No new allow-list entries were added. The known-constant list is the G5C.2 list.
