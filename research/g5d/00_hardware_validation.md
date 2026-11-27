# G5D hardware validation

Decision: `ALPHA1_HARDWARE_VALIDATED`

The owner installed Genesis v0.2 Alpha 1 on a physical Polyend Play+ and reported these results:

| Check | Result |
|---|---|
| Turing Machine | PASS |
| Markov presets | PASS |
| Markov Learn | PASS |
| Mutation Lab | PASS |
| 32-step generation | PASS |
| 64-step generation | PASS |
| External MIDI playback | PASS |

The owner confirmed that all six generators are accessible.

Usability finding: Markov and Mutation were nested inside the Evo lab. The owner could not find them at first. They later found both through the Evo mode selector and tested them successfully. That nesting is the reason G5E puts Turing, Markov, and Mutation on the Lab+ selector.

No other individual physical checks were reported. Power-loss atomicity was not tested. Physical stack and heap headroom were not measured.

The validated image is `PlayPlus_FreePlay_Genesis_v0_2_Alpha1.ppf`, version `1.3.0-FreePlay0.2d`, 4,271,087 bytes, SHA-256 `39a061780b16832de063d30a248233405be40378e2dffe9833ac15b8bf193228`. Implementation commit `6e28718856c25af9d1c9a30a26a438e4fe7886a6`.

Annotated tag `genesis-v0.2-alpha1-owner-validated` points at that commit. It was not moved.
