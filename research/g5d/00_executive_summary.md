# G5D executive summary

Decision: `ALPHA1_OWNER_FUNCTIONAL_VALIDATION_PASSED`.

The owner installed Genesis v0.2 Alpha 1 on a Polyend Play+ and reported that it works. This closeout records that statement. It does not add firmware, and it does not start G5E.

| | |
|---|---|
| Implementation branch | `cursor/g5c3-alpha1-hardware-preflight` |
| Implementation commit | `6e28718856c25af9d1c9a30a26a438e4fe7886a6` |
| Annotated tag | `genesis-v0.2-alpha1-owner-validated` |
| Tag target | the implementation commit above |
| Version | `1.3.0-FreePlay0.2d` |
| File | `PlayPlus_FreePlay_Genesis_v0_2_Alpha1.ppf` |
| Bytes | 4,271,087 |
| SHA-256 | `39a061780b16832de063d30a248233405be40378e2dffe9833ac15b8bf193228` |

The owner’s words, recorded in full in `01_owner_report.md`:

> I was able to download and test the new firmware and everything works as expected.

That sentence is a general functional confirmation. It is not a per-item result for the staged checklist in `research/g5c_3/08_owner_hardware_checklist.md`.

Alpha 1 stays the working image. Later Genesis v0.2 work rolls back to this image. The v0.1 image remains the earlier hardware-validated baseline. See `04_rollback_baselines.md`.

G5E, firmware memory optimization, is described in `05_g5e_handoff.md` and is not started.
