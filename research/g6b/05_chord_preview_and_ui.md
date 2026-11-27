# Chord preview and UI

Lab order on Lab+ (button 0): Melody, Euclid, Arp, Turing, Markov, Mutate, Chords. Chords is lab 6. There is no hidden mode selector.

Parameters added in front of Reset: Prog, Qual, Type, ChInt, Hold. Type is shown only when Qual is Fixed. V.max is hidden on this lab. V.min is labeled Vel. Track, phrase, seed, root, scale, velocity, and gate stay on the existing rows.

Defaults: preset I–V–vi–IV, triads, fixed id Major, interval index 1 (8 steps), sustain, root 60, natural minor, seed 42, track 8, gate 100%. On natural minor the preset spells i–v–VI–iv. The scale is not rewritten.

Preview text shows the lab, root, scale, preset, quality mode, phrase/interval, Hold or Step, seed, page, and the root plus the short chord name for each visible step. Short names are the grid labels (`Maj`, `Min`, `Su4`, …). The Type row uses the full stock names.

Chord ids live on the UI object at `cid[]` (offset 3134), not inside `gen_preview`. The preview struct stays 344 bytes. Apply reads `cid[]` for the chord lab and passes NULL for every other lab, which writes locked None.

Preview build does not call `setParam`. Cancel returns to config and leaves the track bytes unchanged. Apply uses the stored events and ids. Changing a parameter, the track, or the pattern fingerprint drops the preview.

Menu Enter and Lab+ share button 0. `cfw_genesis_enter` sets `arm_top` and `arm_ms`. The first Top release inside 200 ms is consumed and does not change the lab or the recovery flag. A later Lab+ still cycles. If no external MIDI track validates during enter, the track stays at 8 instead of the last index the scan tried.

Screenshots in `research/g6b/screenshots/`:

| File | What it shows |
|---|---|
| `01_chords_config.png` | Chords, track 9, pattern 32, MIDI ok |
| `02_chords_prog.png` | Prog focused |
| `03_chords_preview.png` | Preview, seed 42, chord names |
| `04_chords_page.png` | Page 2/2 of a 32-step phrase, 4 hits |
| `05_chords_applied.png` | `Applied 2` / the 32-step apply toast |
| `06_chords_replace.png` | `Replace? Apply again` |
| `07_mutate_has_chords.png` | `Has chords` |
| `08_recovery.png` | Recovery still set after Shift+Back |

The 32-step apply wrote steps 0, 8, 16, and 24 with notes 60, 67, 68, 65 and ids 4, 4, 3, 4, matching the stored preview.
