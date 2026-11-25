# 10 — Evidence Register (G1)

| ID | Claim | Class | Pointer |
|---|---|---|---|
| G1-E001 | G1 branch created from G0 `8bfc72b` | VERIFIED_HOST_TEST | `git checkout -b … 8bfc72b` |
| G1-E002 | Upstream pin `5835b33` unchanged | VERIFIED_SOURCE | parent history |
| G1-E003 | `PlayPlus_v1.3.0.ppf` absent | VERIFIED_HOST_TEST / BLOCKED | filesystem + Drive search |
| G1-E004 | Expected input SHA-256 known | VERIFIED_SOURCE | `target.json`, `PlayPlus.patch.json` |
| G1-E005 | Thumb unit tests pass | VERIFIED_HOST_TEST | `TestThumbFields` |
| G1-E006 | clang Thumb smoke OK | VERIFIED_HOST_TEST | local compile |
| G1-E007 | PPF/vdev/build blocked | BLOCKED | missing artifact |
| G1-E008 | `cfw_step_at` / toggle / edit_begin / locks / refresh mapped | VERIFIED_SOURCE | `slice.c`, `slicer.c` |
| G1-E009 | Step size 0x2A; sample tracks 0–7; dual step vtables; 22 params | VERIFIED_SOURCE | `emu_slice.py` PROFILES |
| G1-E010 | Track type 0/1 semantics | VERIFIED_SOURCE | `FIRMWARE_NOTES` |
| G1-E011 | MIDI note/vel/dur indices unknown | UNKNOWN | — |
| G1-E012 | MIDI write experiments not run | BLOCKED | — |
| G1-E013 | Polyphony unresolved | UNKNOWN | — |
| G1-E014 | Undo recipe exists for sample batches | VERIFIED_SOURCE | slicer place |
| G1-E015 | MIDI undo untested | BLOCKED | — |
| G1-E016 | Cave used ~13158/16384; ext ~34712/65536 | VERIFIED_STATIC_ANALYSIS | patch JSON blobs |
| G1-E017 | `cfw_ram` full (256 B) | VERIFIED_SOURCE | `slice.h` assert; docs |
| G1-E018 | Project CC map verified | VERIFIED_SOURCE | `ccpreset.c` |
| G1-E019 | No hardware flash/deploy | VERIFIED_HOST_TEST | process policy |
| G1-E020 | No FreePlay production code changes | VERIFIED_HOST_TEST | diff = `research/g1` only |
| G1-E021 | Velocity/Length MIDI UI remap | INFERRED | public Polyend manual summaries |
| G1-E022 | Integration contract drafted | design | `09_integration_contract.md` |

## Missing artifacts

1. Owner-provided `PlayPlus_v1.3.0.ppf`  
2. Built `cfw/build/playplus-1.3.0/` image  
3. vdev MIDI step dumps / write logs  

## Owner recovery (exact)

See `01_firmware_preflight.md` — place file at `/workspace/PlayPlus_v1.3.0.ppf`, verify SHA-256 `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876`, re-run G1.
