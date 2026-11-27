# Rollback baselines

Two images stay available. Neither was rebuilt for this closeout.

## Working baseline for later Genesis v0.2 development

Alpha 1 is the firmware on the owner’s Play+ and the rollback image for every later v0.2 change.

| | |
|---|---|
| File | `PlayPlus_FreePlay_Genesis_v0_2_Alpha1.ppf` |
| Version | `1.3.0-FreePlay0.2d` |
| Bytes | 4,271,087 |
| SHA-256 | `39a061780b16832de063d30a248233405be40378e2dffe9833ac15b8bf193228` |
| Commit | `6e28718856c25af9d1c9a30a26a438e4fe7886a6` |
| Tag | `genesis-v0.2-alpha1-owner-validated` |
| Card filename | `PlayPlus_FreePlay_01.ppf` |

Install and menu rollback steps remain `research/g5c_3/07_owner_installation.md` and `research/g5c_3/09_rollback_procedure.md`, with this file in place of a newer image. A firmware update does not undo sequencer edits.

## Earlier hardware-validated v0.1 baseline

v0.1 stays preserved. All ten owner-operated checks from that release passed. It is the rollback below Alpha 1.

| | |
|---|---|
| File | `PlayPlus_FreePlay_Genesis_v0_1.ppf` |
| Version | `1.3.0-FreePlay0.1b` |
| Bytes | 4,236,752 |
| SHA-256 | `5dc172f7262d7bda29d9b250ddc09bd70265dd96321e2c35cc8eef34856c4f26` |
| Implementation | `90ed2e6` |
| Hardware preflight | `fb907d4` |

Stock recovery is unchanged: leave only `PlayPlus_v1.3.0.ppf` in `Firmware`, eject, and use the rear reset. Recovery is not guaranteed if the bootloader is damaged or power is lost during the flash. Stock SHA-256 is `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876`, 4,098,017 bytes.
