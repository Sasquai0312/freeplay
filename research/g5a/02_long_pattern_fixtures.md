# Longer-pattern fixtures

No owner project was required. The virtual device created the destinations with the stock Track Length knob. Nothing was patched into an unknown length field.

| Fixture | How it was obtained | Native length |
|---|---|---|
| Track 8 at 32 | Knob 10, bank 1, select pad row 0 | 32. Other tracks stayed 16. |
| Track 8 at 64 | Same knob, 48 steps from 16 | 64. Further steps do not increase it. |
| Track 8 returned to 16 | Knob 10 the other way, fresh boot | 16. Step 16 then fails the adapter read. |
| 128 | Knob attempt | Clamped at 64. Not a native destination. |

Screen: `research/g5a/screenshots/track_length_64.png` shows Track Length 64 on Pattern 1.

Early probe notes are in `research/g5a/fixtures/g5a_length_probe.json`. One climb in that probe stopped at 32 because the turn budget ran out before the control had moved; a later uninterrupted climb reached 64, and the G5A suite repeats that climb.

The G3B file `research/g3b/fixtures/g3b_32step_experiment.json` is a symbol scan, not a pattern.
