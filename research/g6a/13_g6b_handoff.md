# G6B handoff

Decision to implement from: `NATIVE_CHORD_PARAMETER_INTEGRATION_READY`

Do not start from Path A. Do not link `chords.c` or SMF export. Do not add static CFW RAM or a cave hook. Do not flash a device in order to discover the representation again. The representation is the contract in `12_verified_integration_contract.md`.

## First release

Reuse the Genesis panel. These rows already exist: Lab, Track, Phrase, Seed, Root, Scale, Gate%, Intv.

Add one chord value. Its labels are the 21 stock names, or a shorter first set: None, Major, Minor, Sus2, Sus4, Maj 7th, Min 7th, Dom 7th.

Generate, per step in the phrase:

- root from the existing scale and root controls
- one chord id from the preset or the chord value
- the existing velocity and gate, written to parameters 0 and 1
- the id, written to parameter 10

A preset such as I–V–vi–IV is a list of scale degrees plus a quality. The degree picks the root. The quality picks the id. The firmware plays the chord.

Snapshot the halfword with the note. Restore it on cancel and on a failed region. Keep `EMPTY_ONLY` and confirmed `REPLACE_REGION`. Keep the recovery lock.

## Deferred

- Inversions and voice-leading. They do not fit one id.
- Per-voice velocity, duration, chance, or timing.
- Dominant 9th as spelled by `GEN_CHORD_NINTH`. It is not Maj 9th or Min 9th.
- 11ths and 13ths as generated pitch lists. The names exist as ids 17..20 if the UI wants to offer them.
- Path C, several MIDI tracks, for a chord the 21 names cannot spell.
- Linking the 1,376-byte portable object.
- A real-time arpeggiator and a multi-track generator. They still share the 7,352-byte budget.
- An owner listening test. Schedule it after a G6B image exists. G6A did not produce one.

## Before that image is built

Measure the linked delta of the id map, the snapshot field, and the extra UI row. Stop if the extension would pass 65,536 − 2,048, if the cave would move, or if `cfw_ram` would grow. The production baseline to build on is `7f40dcf`, version `1.3.0-FreePlay0.2e`, SHA-256 `777461547a86d635c1c1d4bd7dafd01c4e406089cd2d53083a266768269bfdc0`.
