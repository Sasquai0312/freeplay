# Regression results

No firmware source changed, and the production image hash is still `777461547a86d635c1c1d4bd7dafd01c4e406089cd2d53083a266768269bfdc0`. The emulator suites were not repeated. Their G5E results stand because the image they ran is the same file.

Re-run this phase:

| Suite | Result |
|---|---|
| `make test` (Genesis, Evolution, evolution session) | passed. `gen_evo` is 1,096 |
| `make CC=gcc SAN='-fsanitize=address,undefined -fno-omit-frame-pointer' test` | passed. GCC 13.3 ASan and UBSan |
| `research/g6a/diagnostics/chord_native_probe.py` | 20/20 on the production snapshot |

Not re-run, image unchanged, G5E record:

| Suite | G5E result | Image |
|---|---|---|
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
| Firmware verifier, emulator | 43/43 PASS | production and diagnostic |
| Runtime probe | 12/12 | production |

Unavailable, same as G5E:

| Suite | Reason |
|---|---|
| Clang ASan | `libclang_rt.asan_static-x86_64.a` is not installed |
| Full `cfw/tests/test_tools.py` | `PolyendPlay_v1.6.0.ppf` is not in the tree |

New diagnostic cases, and the ones that were not exercised:

| Case | Result |
|---|---|
| Authentic chord readback | pass, id 0 reads as None and id 3 as Major |
| Supported id range 0..20 | pass |
| Id 21, 255, and -1 | pass as clamps to 20 or 0, not as error rejections |
| A voice-count field | not present, so there is no voice-count test |
| Pitch-range rejection | not retested. Note stayed 60 |
| Create, replace, clear | pass |
| Monophonic fields preserved | pass |
| Halfword restore | pass |
| Production `gen_snap` restore of the chord | not implemented, and the struct still omits the field |
| Unrelated step and sample track | pass |
| Steps 0, 15, 16, 31, 32, 63 hold the field | pass, while length stayed 16 |
| Switching the pattern to 32 or 64 | not exercised |
| Sample knob is not Chord | pass |
| Duplicate pitches | not representable |
| Shared note, velocity, duration | pass |
| Playback classification | recorded in `07_playback_evidence.md`. Multi-note dispatch was not observed and is not a pass |
| Resource bound | production image unchanged |

No existing assertion was deleted or weakened.
