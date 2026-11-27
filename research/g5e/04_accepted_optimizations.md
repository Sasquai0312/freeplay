# Accepted optimizations

Linked extension bytes, measured on the production image. The pieces do not overlap.

| Change | Extension bytes reclaimed |
|---|---:|
| One shared `dl`, `text`, `text_f`, and `vline` in the cave | 1,356 |
| `mutate.c` placed in the existing cave | 2,028 |
| Drop extension sections nothing reachable calls | 2,052 |
| Preview screen also prints the generator name | −36 |
| Total | 5,400 |

Baseline extension 61,536. Production extension 56,136. Free 9,400. After the 2,048-byte reserve, the expansion budget is 7,352.

Cave used 15,910 of 16,384. Free cave space is 474 bytes. Static CFW RAM stayed 256.

The shared display helpers copy the old burst rule: 640 words, then wait until the coprocessor is idle. `text_f` still flushes at 600 words. Call sites in the panels are branches. `gen_mutate` moved from the extension into the cave; the UI still calls it, and the branch is inside the ±16 MiB range.

Production keep list: `cfw_genesis_set_recovery`, `cfw_genesis_recovery_active`. Those are not menu rows. The interaction harness uses them to release the recovery lock after the lock has been checked. The extension header is always kept.

Diagnostic keep list adds the symbols G3A, G3C, and G5A call: `genesis_diag_fixture`, `genesis_diag_generate`, `genesis_midi_caps`, `genesis_midi_preview_req`, `genesis_midi_apply_req`, `genesis_midi_validate_track`, `genesis_midi_pattern_length`, `genesis_midi_restore`, `genesis_midi_ticks_to_dur`, `genesis_midi_ticks_to_step`, `genesis_midi_read_step`. That image’s extension is 56,896 bytes, 760 bytes larger than production.

Symbols removed from the production image and still present in the portable sources include unused entry points such as `gen_rhythm_to_events`, `gen_melody_weighted`, `gen_turing_reset`, `gen_markov_add`, and `genesis_midi_apply` (the UI calls `genesis_midi_apply_ex`). Host tests compile those `.c` files themselves.

Not accepted, because they lost space or broke the cave limit: `-Os`, `-O2`, `-fno-inline`, disabling the outliner, jump-table removal, and extension LTO. Musical algorithms were not replaced. Validation, readback, snapshot restore, and the G5C.3 recovery lock were not weakened. No region grew. No static RAM was added.
