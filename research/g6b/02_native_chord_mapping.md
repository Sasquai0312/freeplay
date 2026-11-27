# Native chord identifier mapping

Authoritative table: stock display names for parameter 10, ids 0 through 20 inclusive. That is 21 names in total, counting None. There are not 21 names after None. The extra knob detent and `setParam(21)` stay on 20 (`Min 13th`). `setParam(255)` and `setParam(-1)` clamp to 0. The setter does not return an error, so Genesis rejects an id above 20 before the call.

Evidence is `research/g6b/fixtures/chord_preflight.json` (`names`, and `after_extra_detent: 20`). Names are the stock formatter strings, not a claim about sounding intervals.

| Id | Stock name | Id | Stock name |
|---|---|---|---|
| 0 | None | 11 | Maj 7th |
| 1 | Power 4th | 12 | Min 7th |
| 2 | Power 5th | 13 | MinMaj7 |
| 3 | Major | 14 | Dom 7th |
| 4 | Minor | 15 | Maj 9th |
| 5 | Dim | 16 | Min 9th |
| 6 | Aug | 17 | Maj 11th |
| 7 | Sus2 | 18 | Min 11th |
| 8 | Sus4 | 19 | Maj 13th |
| 9 | Maj 6th | 20 | Min 13th |
| 10 | Min 6th | | |

Storage is a signed 16-bit value at step offset 18, parameter type 3. Read with `getParam` at `0x6010acfc`. Write with `setParam` at `0x6010ad88`. External MIDI eligibility remains `type == 1 && inst == 0`.

The portable interval engine in `chords.c` is not linked into the firmware. Chord quality for a progression comes from `chordprog.c` and this id list.
