# Native adapter experiments

No firmware was added. The diagnostic is `research/g6a/diagnostics/chord_native_probe.py`. It drives the stock UI and the stock accessors on the production snapshot. Results: `research/g6a/fixtures/chord_native_probe.json`, 20 of 20 checks passed.

| Check | Result |
|---|---|
| Bank toggle reaches the MIDI tracks | pass |
| Note page is Pitchbend | pass |
| Knob 7 opens Chord at None | pass |
| Knob walks all 21 names in order | pass |
| Knob stops on Min 13th | pass |
| Note, velocity, duration stay 60, 100, 60 | pass |
| Id 20 is the halfword at step+18 | pass |
| `setParam` 3 reads back 3 | pass |
| `setParam` 21 clamps to 20 | pass |
| `setParam` 255 clamps to 0 | pass |
| `setParam` -1 clamps to 0 | pass |
| Step 5 stays off | pass |
| Sample-track parameter 10 is unchanged | pass |
| Restore with `setParam(10, 0)` reads 0 | pass |
| Live pattern length stays 16 | pass |
| The same halfword accepts id 3 on steps 0, 15, 16, 31, 32, and 63 | pass |
| Sample-track knob 7 is not Chord | pass |
| While transport is running, the parameter flush reads index 10 | pass |
| USB MIDI transmit is not entered during that window | pass |

The step-index check writes id 3 and then writes 0 back. On a step that was previously unlocked, `setParam(10, 0)` leaves the value locked at None. That is the occupied representation of None, not the unlocked off-step state. Clearing an off step completely is still the stock toggle.

Out-of-range ids are clamped, not rejected. A G6B writer should send 0..20 and not rely on an error return.

Pattern length was not switched to 32 or 64 in this probe. The field exists on those step objects. The existing length gate still decides which steps Genesis may edit. See `06_transaction_feasibility.md`.
