# Musical demonstrations

Twelve Standard MIDI files are in `genesis/examples/evolution/`. The host exporter wrote them at 120 bpm, 480 ticks per quarter, one step per quarter, gate 360 ticks (native duration 45). Channel is 0. Each file is format 1 with balanced note-on and note-off pairs, checked by `genesis/tools/check_smf.py`, which does not use the Genesis writer.

The checksum is FNV-1a of the MIDI bytes. Parameter manifests are the sibling `.txt` files. The zip `genesis/examples/evolution/genesis_g5b_demos.zip` contains the MIDI files and the manifests.

These checks are structural. They do not say how the music sounds.

| File | Generator | Seed | Root | Scale | Steps | Events | Native | FNV-1a |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `01_turing_locked_8.mid` | Turing, length 8, evolve 0, rest 25, lock | 11 | 60 | natural minor | 8 | 7 | yes | `9f5731f8` |
| `02_turing_slow_16.mid` | Turing, length 16, evolve 12, rest 8 | 22 | 62 | dorian | 16 | 14 | yes | `720dd392` |
| `03_turing_moderate_32.mid` | Turing, length 16, evolve 35, rest 10 | 33 | 57 | natural minor | 32 | 29 | yes | `58196d75` |
| `04_turing_evolve_64.mid` | Turing, length 32, evolve 55, rest 12 | 44 | 60 | dorian | 64 | 60 | yes | `172bd5b3` |
| `05_markov_stepwise_minor.mid` | Markov stepwise, rest 8, octave motion | 55 | 57 | natural minor | 32 | 29 | yes | `e7f8970b` |
| `06_markov_bassline.mid` | Markov bassline, range 36–60, rest 5 | 66 | 36 | natural minor | 16 | 13 | yes | `ee0ebfcd` |
| `07_markov_exploratory.mid` | Markov exploratory, rest 6 | 77 | 62 | dorian | 32 | 32 | yes | `b937ad89` |
| `08_markov_learned.mid` | Learned from C D E G E D C | 88 | 60 | major | 32 | 32 | yes | `5e263ed9` |
| `09_mutation_source.mid` | Identity of the C natural-minor phrase | 100 | 60 | natural minor | 16 | 16 | yes | `d0915fc8` |
| `10_mutation_pitch.mid` | Pitch probability 30, intensity 2 | 101 | 60 | natural minor | 16 | 16 | yes | `cdfadc28` |
| `11_mutation_rhythm_protected_pitch.mid` | Displace 80, intensity 2, pitch protected, dest 32 | 102 | 60 | natural minor | 32 | 16 | yes | `9e918a51` |
| `12_mutation_combined.mid` | Pitch 40, displace 35, velocity 50 into 70–110, dest 32 | 103 | 60 | natural minor | 32 | 16 | yes | `107bc54e` |

The learned source fixture is the seven notes `60 62 64 67 64 62 60`. Demo 11 keeps that phrase’s pitch, velocity, and duration multiset and changes tick positions. The destination is 32 steps so displacement has empty steps; a full 16-step grid cannot move a note without overwriting another, and the collision rule refuses that overwrite. Demo 9 is the phrase with every probability at 0.

Pitch range for the Turing demos is 48–72, 50–74, 48–76, and 48–81 in file order. Markov ranges are 57–81, 36–60, 50–86, and 60–84. Mutation pitch bounds are 48–84. Velocity on the generators is 100 except Markov demos at 96.
