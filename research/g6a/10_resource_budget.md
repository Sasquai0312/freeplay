# Resource budget

The production image was not rebuilt. These figures are the G5E image plus measurements taken beside it.

| Resource | Value | How it was obtained |
|---|---|---|
| Extension used / free | 56,136 / 9,400 | unchanged image |
| Reserve held back | 2,048 | unchanged |
| Budget shared by later features | 7,352 | unchanged |
| Cave used / free | 15,910 / 474 | unchanged |
| Static CFW RAM | 256 / 256 | unchanged |
| Genesis UI object | 3,124 | unchanged |
| `gen_evo` | 1,096 | reconfirmed by `make test` |
| `chords.o` Thumb, `-Os`, text+rodata+data | 1,376 | `make thumb-size` this phase |
| Production image growth | 0 | hash still `777461547a86d635c1c1d4bd7dafd01c4e406089cd2d53083a266768269bfdc0` |

`chords.c` is not linked. A Chord Lab that writes native ids does not need it. The 1,376 bytes stay available inside the 7,352 budget only if a later phase links the object on purpose. Linking it still leaves the 2,048 reserve, and it still leaves about 6,000 bytes, but the panel and the adapter have not been compiled, so that remainder is not a promise.

Estimated, and not compiled into an image:

| Piece | Estimate |
|---|---|
| Snapshot field | +2 bytes per step, +128 bytes on the stack-resident `gen_snap` |
| Read and write of parameter 10 | a `getParam` / `setParam` pair beside the existing note, velocity, and duration calls |
| Preview storage | one id per step if the preview shows the quality, or nothing extra if the preview keeps today's one note |
| UI | one value drawn by the existing parameter list. The list already has Root, Scale, Phrase, Seed, Gate%, and Intv |
| Cave hook | none |
| Static RAM | none |

Multi-track generation and a real-time arpeggiator still share the 7,352 bytes. This phase does not split that number, because G6B has not been sized as a linked delta. The constraint is the same one G5E left: measure the real delta, keep 2,048 bytes free, and do not touch the cave or `cfw_ram`.

Physical stack and heap headroom remain unknown. The +128 byte snapshot is the only new stack figure, and it is a struct-layout figure.
