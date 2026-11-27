# G5C.3 executive summary

Decision: `READY_FOR_CONTROLLED_ALPHA1_HARDWARE_TEST`.

Alpha 1 is a private firmware candidate for the owner’s Play+. It was built from this branch, checked by the firmware verifier, and exercised on the virtual device. It has not been installed on the Play+.

| | |
|---|---|
| Version | `1.3.0-FreePlay0.2d` |
| Download name | `PlayPlus_FreePlay_Genesis_v0_2_Alpha1.ppf` |
| Bytes | 4,271,087 |
| SHA-256 | `39a061780b16832de063d30a248233405be40378e2dffe9833ac15b8bf193228` |
| Card filename | `PlayPlus_FreePlay_01.ppf` |

The image is not the G5C.2 file. Two source changes produced the new hash: the version byte, and Shift+Back while recovery is set. That key now toasts `Recovery required` and leaves Apply blocked until the next power-on. A track re-read does not restore the pattern.

## What passed

- Verifier exit 0, emulator slice 43/43, no bytes outside the cave and the hooks.
- Extension left 4,000 bytes. Static `cfw_ram` stayed 256. Cave end stayed `0x60190072`.
- One 3,124-byte UI object, reused across 100 open / generate / cancel / close cycles.
- Applicable regressions in `05_regression_results.md`, including the Evolution interaction suite 76/76.
- Production menu rows do not set `diag_fail_at`, call `genesis_diag_fixture`, or edit a sample track.

## What stays unknown

- The Play+ stack ceiling. The emulator saw a 2,128-byte drop inside CFW code and did not fault. That sample excludes native callees and is not remaining headroom.
- Free bytes in the `fw_operator_new` arena. One successful allocation is not a free-heap measurement.
- Whether a `GEN_MIDI_ERR_RESTORE` return left the destination fully written or partly written. The UI does not retry and does not claim the pattern was restored.

Installation, the staged checklist, and rollback are `07_owner_installation.md`, `08_owner_hardware_checklist.md`, and `09_rollback_procedure.md`. Stop on any unexplained failure. v0.1 remains the hardware-validated rollback image.

This phase stops here. It does not start G5E or G6A.
