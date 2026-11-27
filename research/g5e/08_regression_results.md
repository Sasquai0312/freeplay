# Regression results

Production image: `/tmp/fp-g5e/PlayPlus_FreePlay_01.ppf`, SHA-256 `777461547a86d635c1c1d4bd7dafd01c4e406089cd2d53083a266768269bfdc0`.

Diagnostic image: `/tmp/fp-g5e-diag/PlayPlus_FreePlay_01.ppf`, SHA-256 `b5070abfe76053e261c8a61944dd9d233d0e6ecae5dfef48828e640ef9d449df`. Same cave, extension 56,896 bytes. Used only where a suite calls a diagnostic symbol.

| Suite | Result | Image |
|---|---|---|
| Portable Genesis, Evolution, and session (`make test`) | passed, `gen_evo` 1,096 | host |
| GCC 13.3 ASan and UBSan | passed | host |
| Clang ASan | unavailable: `libclang_rt.asan_static-x86_64.a` is not installed | not counted |
| Thumb fields | 3/3 | none |
| MIDI mono | 6/6 | production |
| G3A | 42/42 | diagnostic |
| G3B | 17/17 | production |
| G3C | 43/43 | diagnostic |
| G5A | 60/60 | diagnostic |
| Screen | 23/23 | production |
| Tap | 6/6 | production |
| Slicer | 38/38 | production |
| Evolution interaction | 83/83 | production |
| Firmware verifier, including the emulator | 43/43, result PASS | production and diagnostic |
| Runtime heap and stack probe | 12/12 | production |
| Full `cfw/tests/test_tools.py` | not run: `PolyendPlay_v1.6.0.ppf` is not in the tree | not counted |

G5A’s two length checks now look for the label `Phrase`. The encoder still raises 16 to 20 and to 32. The assertions were not removed.

The interaction suite covers the six labs, forward wrap, Melody/Euclid/Arp, phrase length, Turing preview, Markov styles and empty Learn, mutation rotate and protect, 32-step and 64-step generation, one-shot Apply, stale preview rejection, unrelated-track preservation, and the recovery lock. After a restore error the preview is dropped, Apply stays blocked, Shift+Back does not clear the lock, and changing labs does not clear it. Top while the lock is set toasts “Recovery required” and leaves the lab where it is. Menu re-entry uses button 0, which is also Lab+, so reopening the panel can land on a later lab. The lock is still set when that happens. The object finder now accepts labs 0 through 5. It previously stopped at lab 3, which made Markov and Mutate unreadable to the harness.

The runtime probe ran 100 open, switch, generate, cancel, and close cycles: 34 Turing, 33 Markov, 33 Mutate. One UI object, pointer `0x2004d670`, size 3,124, stable across the cycles. Extension header length 56,136. Stack sample for a Turing preview and a Mutation preview: drop 2,128 bytes, 2,400 bytes below the reset stack pointer. That sample is not physical headroom.

No assertion was deleted to obtain a pass.
