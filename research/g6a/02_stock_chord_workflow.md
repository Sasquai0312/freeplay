# Stock chord workflow

External MIDI and the internal sample grid do not share this control.

The grid bank starts on tracks 1–8. Writing the bank byte is not enough to change pages. The stock toggle at `0x600cf578` does. After that call, track 8 is an external MIDI track (`type == 1`) and the Note page title is Pitchbend. On a sample track the same knob is Microtune.

Chord editing on that MIDI track:

1. The step pad is held. A fresh toggle turns the step on and locks Chord at 0.
2. Knob 7 is touched. The page title is `Chord` and the value is `None`.
3. Each clockwise detent advances one stock name. The twenty-second detent stays on `Min 13th`.

Names, in order, from the string table at `0x6007ac88` and from the live page:

| Id | Name | Id | Name |
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

The word Chord is at `0x60079df4`. The formatter suffix is ` [-]`. The UI record built at boot lives at `0x2001d6bc`, and the formatter object is at `0x2003d2c8` with count `0x15` (21).

Other strings that look related are not this control. `Polyphony` at `0x6007fba4` sits with the internal patch editor. `Chords and Bass` at `0x60085e00` is a project-template name.

Knob 7 on a sample track does not open Chord. A chord write on track 8 left sample-track parameter 10 unchanged. Parameter 10 is Sample Start on a sample step and Chord on an external MIDI step. The track-type gate stays `type == 1 && inst == 0`.

Screenshots from the passing probe: `research/g6a/screenshots/chord_none.png`, `chord_major.png`, `chord_min7.png`, `chord_min13.png`.
