# Handoff after Alpha 1

G5C.3 stops at the owner package. It does not start G5E or G6A, and it does not flash the Play+.

## If the owner checklist passes

The next firmware change is a later phase. Do not treat a passed checklist as permission to expand the cave, static RAM, or the extension past the 2,048 byte reserve. Do not replace `genesis_midi_apply_ex`. Stage 7 still commits and returns `GEN_MIDI_ERR_RESTORE`.

Record the owner’s notes against `08_owner_hardware_checklist.md`: version string seen, each stage, and any stop.

## If the owner stops

Use `09_rollback_procedure.md`. v0.1 remains the hardware-validated rollback image:

- Commit of that implementation: `90ed2e6`
- Preflight of that image: `fb907d4`
- File `PlayPlus_FreePlay_Genesis_v0_1.ppf`
- SHA-256 `5dc172f7262d7bda29d9b250ddc09bd70265dd96321e2c35cc8eef34856c4f26`
- 4,236,752 bytes

## Limits that are still open

- Physical stack capacity is not in this tree. The emulator sample is in `03_stack_heap_preflight.md`.
- Free bytes in the `fw_operator_new` arena were not read from a firmware symbol. One 3,124 byte UI object allocated on the virtual device and was reused.
- A restore error can leave the destination fully written or partly written. The UI locks Apply until the next power-on and does not undo the notes.
- Full `cfw/tests/test_tools.py` still needs `stock_firmware/PolyendPlay_v1.6.0.ppf`. Clang ASan still cannot link `libclang_rt.asan`.

Boot snapshots for any later virtual run must be taken from the Alpha 1 image. `LINK_MAP` must be unset or must point at `/tmp/fp-g5c3/link.map`.
