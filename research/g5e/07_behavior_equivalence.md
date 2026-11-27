# Musical behavior

No generator algorithm was edited. `mutate.c`, `turing.c`, `markov.c`, `melody.c`, `rhythm.c`, `arpeggiator.c`, and `evolve.c` are the Alpha 1 sources. `mutate.c` was only moved from the extension source list into the cave source list.

Host suites compile those sources and compare the portable results:

- Melody uniform and random walk
- Euclidean rhythm
- Offline arpeggiator
- Turing locked and evolving, including reset
- Markov Step, Motif, Bass, Exploratory, and Learn
- Mutation of pitch, rhythm, velocity, and duration
- Positive and negative rotation, reversal, attribute protection, protected steps
- Lengths covered by the existing suite, including the 16, 32, and 64-step native checks in G5A

`make test` and `make CC=gcc SAN='-fsanitize=address,undefined -fno-omit-frame-pointer' test` both passed. `gen_evo` is 1,096 bytes. GCC 13.3.0.

The native image checks event count, order, pitch, velocity, duration, and timing through the existing G3A, G3C, and G5A readback. Those three suites were run on the diagnostic image, because production garbage collection drops the harness entry points they call. Results: G3A 42/42, G3C 43/43, G5A 60/60. Unrelated tracks stayed unchanged. The production image links the same generators and omits those entry points. Production interaction (83/83) and the runtime probe exercised generate, apply, cancel, and the recovery lock on that exact image.
