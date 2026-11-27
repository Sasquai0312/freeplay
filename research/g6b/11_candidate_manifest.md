# Candidate manifest

| Item | Value |
|---|---|
| Decision | `CHORD_LAB_OFFLINE_VALIDATED_RESTRICTED` |
| File | `PlayPlus_FreePlay_Genesis_v0_2_Alpha2.ppf` |
| Card install name | `PlayPlus_FreePlay_01.ppf` |
| Version | `1.3.0-FreePlay0.2f` |
| Size | 4,268,972 bytes |
| SHA-256 | `4598a98cfd83b9e0c2eec080c51980a2d1d0c9b82876a217232dfc708da184fd` |
| Implementation | `314fbefc030f3ef769aa054b9a5be15cb909713d` |
| Branch | `cursor/g6b-chord-progression-lab` |
| Base | `cursor/g6a-native-polyphony-discovery` at `d95a193dbaabe4a86077c53f26eb89c18990e338` |
| Compiler | clang 18.1.3, `-Oz`, `thumbv7em-none-eabihf`, hard float, `-fshort-enums` |
| Linker | `cfw/tools/link.py` with section GC on. `CFW_SKIP_LLD=1`. ld.lld was not used, so no lld comparison |
| Build | `cfw/tools/build.py cfw/targets/playplus-1.3.0/target.json --no-verify`. Verifier run separately |
| Extension | `0x60190b00` .. `0x6019f870`. 60,784 of 65,536. Free 4,752. Reserve 2,048. Budget 2,704 |
| Cave | `0x6018ca40` .. `0x60190866`. 15,910 of 16,384. Free 474. No new cave hook |
| Static CFW RAM | `0x20002700`, 256 of 256 |
| UI object | 3,200 bytes |
| Verifier | PASS. Emulated tests 43/43. Stock checksums, hooks, and branch targets resolved |
| Diagnostic twin | 4,271,312 bytes, SHA-256 `92d367b5133f5048cd7faad5948383956eda6f216bd6afd45bf01d71900f381b`, extension ends `0x6019fbb0` (61,616). Adds `genesis_diag_chord_apply`. Verifier PASS, emulator 43/43 |

`param_visible` is `noinline` and `optnone`. At `-Oz` the same comparisons became the pool constants `0x18380` and then `0x18300`, which the verifier rejects as unknown low addresses. The comparisons are unchanged. That attribute is why growth is 4,648 bytes rather than the first link's 4,064.

The version string is 19 bytes, the same length as `1.3.0-FreePlay0.2e`. The verifier reports `cfw_version` at `0x60190370`.

Private download, not shared by link: [G6B FreePlay0.2f private](https://drive.google.com/drive/folders/180YDbVPe6a4HSgjGhOOZMW2okIbYR0xA). Seven image parts, `PlayPlus_FreePlay_Genesis_v0_2_Alpha2.ppf.part00` through `part06`, plus `README.txt`. Rejoin them in order. `part00` through `part05` are 700,000 bytes. `part06` is 68,972 bytes. The joined file must hash to the SHA-256 above.

ZIP fallback, stored, 4,269,152 bytes: `PlayPlus_FreePlay_Genesis_v0_2_Alpha2.zip.part00` through `part06` in the same folder. `part00` through `part05` are 700,000 bytes. `part06` is 69,152 bytes. The zip contains `PlayPlus_FreePlay_Genesis_v0_2_Alpha2.ppf`.

The image is not in git. The frozen 0.2e image (`1.3.0-FreePlay0.2e`, 4,255,906 bytes, SHA-256 `777461547a86d635c1c1d4bd7dafd01c4e406089cd2d53083a266768269bfdc0`) was not modified. Stock SHA-256 remains `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876`.

This candidate has not been installed on a Play+.
