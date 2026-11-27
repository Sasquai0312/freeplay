# G5F release manifest

| Item | Value |
|---|---|
| Decision | `OPTIMIZED_ALPHA1_HARDWARE_VALIDATED` |
| Image | `PlayPlus_FreePlay_Genesis_v0_2_Alpha1_Optimized.ppf` |
| Card name | `PlayPlus_FreePlay_01.ppf` |
| Version | `1.3.0-FreePlay0.2e` |
| Size | 4,255,906 bytes |
| SHA-256 | `777461547a86d635c1c1d4bd7dafd01c4e406089cd2d53083a266768269bfdc0` |
| Firmware source | `7f40dcf09ebc2351848e21e7bebe6de494d7366b` |
| Documentation tip that contains it | `0f0a2adc4309fe6b9e8cbb0eb2b75ef0855baee5` |
| Tag | `genesis-v0.2-alpha1-optimized-owner-validated` |
| Stock image | `PlayPlus_v1.3.0.ppf` |
| Stock SHA-256 | `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` |

The documentation tip differs from the firmware source by one line in `research/g5e/10_candidate_manifest.md`. No firmware source changed after `7f40dcf`.

Rollback baselines, unmodified:

| Image | Version | Size | SHA-256 |
|---|---|---|---|
| Alpha 1 | `1.3.0-FreePlay0.2d` | 4,271,087 | `39a061780b16832de063d30a248233405be40378e2dffe9833ac15b8bf193228` |
| v0.1 | `1.3.0-FreePlay0.1b` | 4,236,752 | `5dc172f7262d7bda29d9b250ddc09bd70265dd96321e2c35cc8eef34856c4f26` |

The production image is not in git. G6A did not rebuild it. The copy at `/tmp/fp-g5e/PlayPlus_FreePlay_01.ppf` still hashes to `777461547a86d635c1c1d4bd7dafd01c4e406089cd2d53083a266768269bfdc0`.

Regions, unchanged from G5E:

| Region | Used | Capacity | Free |
|---|---|---|---|
| Extension | 56,136 | 65,536 | 9,400 (2,048 held back; budget 7,352) |
| Cave | 15,910 | 16,384 | 474 |
| Static CFW RAM | 256 | 256 | 0 |
