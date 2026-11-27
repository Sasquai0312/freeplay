# Authentic chord fixtures

The fixtures were made on the frozen 0.2e image, in the emulator, through the stock pad, knob 7, and `setParam`. They were not invented in memory.

Live pattern length was 16. Track 8, step 4, after a fresh toggle and before any chord turn:

| Field | Value |
|---|---|
| On | yes |
| Note (index 6) | 60 |
| Velocity (index 0) | 100 |
| Duration (index 1) | 60 |
| Chord (index 10) | 0, UI `None` |
| Step 5 on the same track | off, chord read returns nothing |

After the knob walk and after `setParam`:

| Requested chord | Stored id | Bytes at step+18 | UI |
|---|---|---|---|
| Major | 3 | `03 00` | Major |
| Min 7th | 12 | not separately dumped; knob readback was 12 | Min 7th |
| Min 13th | 20 | low byte 20, high byte 0 | Min 13th |
| back to None | 0 | restored with `setParam(10, 0)` | None |

What the suggested pitch lists are, on this firmware:

| Suggested pitches | Native fixture that matches the name | What is not stored |
|---|---|---|
| C major 60, 64, 67 | note 60, chord id 3 | the 64 and the 67 |
| C minor seventh 60, 63, 67, 70 | note 60, chord id 12 | the upper three pitches |
| C major first inversion 64, 67, 72 | no stock field | an inversion is not an id |

The first inversion cannot be represented by picking a different id on root E. E G C is not one of the 21 qualities rooted on E. Inversions stay out of the native contract.

Neighbor step 5 stayed off. Note, velocity, and duration stayed 60, 100, and 60 while the id moved through the whole list.
