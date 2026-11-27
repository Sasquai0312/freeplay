# Candidate manifest

| Item | Value |
|---|---|
| Decision | `OPTIMIZATION_VALIDATED_TARGET_PARTIAL` |
| File | `PlayPlus_FreePlay_Genesis_v0_2_Alpha1_Optimized.ppf` |
| Card install name | `PlayPlus_FreePlay_01.ppf` |
| Version | `1.3.0-FreePlay0.2e` |
| Size | 4,255,906 bytes |
| SHA-256 | `777461547a86d635c1c1d4bd7dafd01c4e406089cd2d53083a266768269bfdc0` |
| Implementation | `7f40dcf09ebc2351848e21e7bebe6de494d7366b` |
| Branch | `cursor/g5e-firmware-optimization` |
| Base | `cursor/g5c3-alpha1-hardware-preflight` at `6e28718856c25af9d1c9a30a26a438e4fe7886a6` |
| Compiler | clang 18.1.3, `-Oz`, `thumbv7em-none-eabihf`, hard float |
| Linker | `cfw/tools/link.py` with section GC on. `CFW_SKIP_LLD=1`. ld.lld was not used |
| Production keep | `cfw_genesis_set_recovery`, `cfw_genesis_recovery_active` |
| Extension | 56,136 of 65,536. Free 9,400. Reserve 2,048. Budget 7,352 |
| Cave | 15,910 of 16,384. Free 474 |
| Static CFW RAM | 256 of 256 |
| Verifier | PASS, emulator 43/43. Literal `0x00080007` classified as the Lab/Track/Phrase/Preset visibility mask |
| Diagnostic twin | 4,258,037 bytes, SHA-256 `b5070abfe76053e261c8a61944dd9d233d0e6ecae5dfef48828e640ef9d449df`, extension 56,896 |

The version string is 19 bytes, the same length as `1.3.0-FreePlay0.2d`. It lives at `0x60190370` in the cave. The Intel HEX record splits it across two lines; the decoded bytes are `1.3.0-FreePlay0.2e`.

Private download, not shared by link: [G5E FreePlay0.2e optimized private](https://drive.google.com/drive/folders/1yEfUyoVlsho9VRBelttSyso1lKrfXQZs). Seven parts, `part00` through `part06`, plus `README.txt`. Rejoin them in order. `part00` through `part05` are 700,000 bytes. `part06` is 55,906 bytes. A stored ZIP of the same bytes is `PlayPlus_FreePlay_Genesis_v0_2_Alpha1_Optimized.zip` (4,256,158 bytes) beside the image in the artifact store. The image is not in git. Alpha 1 (`1.3.0-FreePlay0.2d`, SHA-256 `39a061780b16832de063d30a248233405be40378e2dffe9833ac15b8bf193228`) and v0.1 (`1.3.0-FreePlay0.1b`, SHA-256 `5dc172f7262d7bda29d9b250ddc09bd70265dd96321e2c35cc8eef34856c4f26`) were not modified. Stock SHA-256 remains `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876`.

This candidate has not been installed on a Play+.
