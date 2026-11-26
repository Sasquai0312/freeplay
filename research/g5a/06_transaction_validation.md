# Transaction validation

Policies are the G3A pair. MERGE is still deferred. Apply is still not power-loss atomic.

## Musical Apply

Each case generated with the firmware UI (the portable engine inside `do_generate`), paged the preview, applied to an empty track, and compared every step with a host build of the same engine call.

| Fixture | Length | Policy | Result |
|---|---|---|---|
| C natural minor, uniform, seed 42, density 70 | 32 | EMPTY_ONLY | Readback matched, including steps on page 2. |
| Same melody, seed 43 | 32 | REPLACE_REGION after the occupied prompt | Readback matched. |
| Euclidean E(11, 32), seed 42 | 32 | EMPTY_ONLY | Readback matched. |
| A minor pentatonic walk, seed 2026, root 57 | 64 | EMPTY_ONLY | 4 preview pages, readback matched. |
| Ascending major triad arpeggio, root 60 | 64 | EMPTY_ONLY | 4 preview pages, readback matched. |

Sample track 0 was unchanged after each Apply. Track 9 stayed length 16 and unchanged during the 64-step adapter edit.

## Failure injection

Events are one note per step so a 32-event edit and a 64-event edit cross the page boundary on step 16. Injection is `genesis_midi_apply_req`, not a menu.

| Stage | 32-step | 64-step |
|---|---|---|
| Before snapshot | Restored, no notes left | Covered by the same code path; 64-step run focused on the write stages below |
| Before first write | Restored | — |
| After first write | Restored | — |
| Mid-edit | Restored | — |
| Page edge (stage 8) | Restored | Restored, track 9 unchanged |
| Near last event (stage 9) | Restored | — |
| Before commit | Restored | — |
| During restore (stage 7) | Returns -11, notes remain | Returns -11, notes remain |

EMPTY_ONLY on a full 64-step region returns -8 and writes nothing further. REPLACE_REGION then rewrites all 64 steps. A second EMPTY_ONLY on that region is rejected.

Stage 7 does not clear the notes. The UI, when it sees -11, sets the session recovery latch and blocks further Apply until `cfw_genesis_set_recovery(0)`. That latch is still heap/session state, not flash. The G3C UI lockout checks were re-run with the regression suite.

No allocation is taken from the heap for the snapshot. The snapshot is the existing stack `gen_snap`. If the region does not fit in 64 steps the adapter returns capacity before any write. A 64-step region fits.
