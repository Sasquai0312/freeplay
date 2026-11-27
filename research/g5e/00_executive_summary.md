# G5E executive summary

Decision: `OPTIMIZATION_VALIDATED_TARGET_PARTIAL`

The production candidate reclaims 5,400 bytes of extension space. The minimum target was 4,096. The preferred target was 8,192. The stretch target was 12,288. This build meets the minimum and stops there. Reaching the preferred target would mean cutting existing FreePlay rendering or moving more code into a cave that has 474 bytes left. Neither was done.

What changed:

- Display-list helpers are one copy in the code cave.
- Mutation generation sits in the remaining cave.
- Unreachable extension sections are dropped from the production image. A diagnostic image keeps the harness entry points.
- Lab+ lists Melody, Euclid, Arp, Turing, Markov, and Mutate. Turing, Markov, and Mutation still share one Evolution implementation.

What did not change:

- Musical algorithms.
- FreePlay features.
- Validation, readback, snapshot restore, and the G5C.3 recovery lock.
- Authorized memory regions and the 256-byte static CFW RAM block.
- The hardware-validated Alpha 1 and v0.1 images.

The candidate is version `1.3.0-FreePlay0.2e`, file `PlayPlus_FreePlay_Genesis_v0_2_Alpha1_Optimized.ppf`, 4,255,906 bytes, SHA-256 `777461547a86d635c1c1d4bd7dafd01c4e406089cd2d53083a266768269bfdc0`. It was not installed on a Play+. G6A was not started.
