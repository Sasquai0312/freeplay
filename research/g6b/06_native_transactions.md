# Native transactions

`genesis_midi_apply_ex` takes an optional chord-id array. NULL means locked None on every written step, so a Melody, Euclid, Arp, Turing, or Markov apply does not leave a stale chord id on a step it replaces.

Order:

1. Reject an id above 20 before the region scan and before any fail injection.
2. Reject a non-MIDI track, a bad step, overlapping events, and an occupied step when the policy is `EMPTY_ONLY`.
3. Snapshot occupancy, note, velocity, duration, and the chord lock byte.
4. On `REPLACE_REGION`, turn off occupied steps and write locked None on a hole that was on or that held a locked nonzero chord. An off unlocked step and an off locked-None step are left alone.
5. Upsert each event, then `setParam(10, id)`.
6. Fail point 10, then a real readback of on, note, velocity, duration, lock, and id.
7. Commit. Fail point 7 commits and returns `GEN_MIDI_ERR_RESTORE` without restoring, which is the existing recovery drill.

Readback mismatch restores. If restore cannot put a field back, it returns -11. The UI drops the preview, sets recovery, does not advance Turing, does not publish Markov learning, toasts `Pattern may differ`, and blocks Apply until the next power-on. Shift+Back, a lab change, and closing and reopening Genesis leave the flag set. The flag lives on the Genesis object in the pages slot, which is not freed when the panel closes.

`gen_apply_req` stays 36 bytes. The diagnostic entry is `genesis_diag_chord_apply` (`gen_chord_req`, 40 bytes), kept only in the diagnostic link. The production map does not contain that symbol. The production UI calls `genesis_midi_apply_ex`.

Stack frames measured on the production objects: `do_apply` subtracts `0x24c`, `genesis_midi_apply_ex` subtracts `0x254`, `do_generate` subtracts `0x43c`. The snap the UI passes is the existing 528-byte object. No extra 128-byte halfword array was added.

Native diagnostic image, 25/25:

| Case | Result |
|---|---|
| 1 empty to chord | id 3, note 60, lock set |
| 2 mono to chord | id 4, note 62 |
| 3 chord to chord | id 12 |
| 4 chord to mono | locked None, note 67 |
| 5 chord to empty hole | off, locked None |
| 6 unlocked restore | off, bit 10 and bit 6 clear, note unread, rc -6 |
| 7 locked None, velocity 100 | note 70, chord 0, vel 100, dur 60 |
| 7b stock velocity 7600 | rc -11, note and chord restored, velocity not 7600 |
| 8 existing id | id 12, note 70 |
| 9 id 21 | rejected, rc -12, step unchanged |
| 10 pitch 200 | rejected, step unchanged |
| 11 sample track | rejected |
| 12, 16 length 16 | step 16 rejected, length is 16 |
| 32 and 64 | ids written at 0, 15, 16, 31 and at 32, 47, 48, 63; one past the end rejected |
| 21 fail before write | unchanged |
| 22 mid-write | restored |
| 23 fail at readback | restored |
| 24 fail at step 16 | note 72, id 14 restored |
| 27 sample track | untouched |
