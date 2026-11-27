# Frozen baseline

G6A started from `cursor/g5e-firmware-optimization` at `0f0a2adc4309fe6b9e8cbb0eb2b75ef0855baee5`.

That commit contains the firmware source `7f40dcf09ebc2351848e21e7bebe6de494d7366b`. The only difference is the implementation line in `research/g5e/10_candidate_manifest.md`.

| Artifact | Identity |
|---|---|
| Production image | 4,255,906 bytes, SHA-256 `777461547a86d635c1c1d4bd7dafd01c4e406089cd2d53083a266768269bfdc0` |
| Version | `1.3.0-FreePlay0.2e` |
| Boot snapshot used for the probe | `/tmp/fp-g5e/PlayPlus_FreePlay_01.ppf.boot.snap` |
| Diagnostic image, not modified and not used for chord writes | 4,258,037 bytes, SHA-256 `b5070abfe76053e261c8a61944dd9d233d0e6ecae5dfef48828e640ef9d449df` |
| Stock `PlayPlus_v1.3.0.ppf` | SHA-256 `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` |

The production image was not rebuilt. No byte was patched. Diagnostic writes went through the stock `setParam` accessor on a disposable emulator session.

Extension budget remains 7,352 bytes. Cave free remains 474. Static CFW RAM remains 256 of 256. The Genesis UI object remains 3,124 bytes. `gen_evo` remains 1,096 bytes.
