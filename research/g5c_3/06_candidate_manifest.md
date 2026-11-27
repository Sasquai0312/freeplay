# Candidate manifest

| | |
|---|---|
| Decision | `READY_FOR_CONTROLLED_ALPHA1_HARDWARE_TEST` |
| Version | `1.3.0-FreePlay0.2d` |
| Computer name | `PlayPlus_FreePlay_Genesis_v0_2_Alpha1.ppf` |
| Card name | `PlayPlus_FreePlay_01.ppf` |
| Bytes | 4,271,087 |
| SHA-256 | `39a061780b16832de063d30a248233405be40378e2dffe9833ac15b8bf193228` |
| Build dir | `/tmp/fp-g5c3/PlayPlus_FreePlay_01.ppf` |
| Download copy | `/opt/cursor/artifacts/PlayPlus_FreePlay_Genesis_v0_2_Alpha1.ppf` |

The image is not in git. It was not flashed.

Label: built and checked on the virtual Play+. Not yet run on the owner’s hardware. The owner installs it only for the controlled checklist.

## Regions

| Region | Span | Used | Capacity | Left |
|---|---|---:|---:|---:|
| Code cave | `0x6018ca40..0x60190072` | 13,874 | 16,384 | 2,510 |
| Extension | `0x60190b00..0x6019fb60` | 61,536 | 65,536 | 4,000 |
| Static `cfw_ram` | `0x20002700..0x20002800` | 256 | 256 | 0 |

Extension reserve is above 2,048. Static RAM did not grow. The cave end address matches G5C.2.

Heap object `genesis_ui` is 3,124 bytes (`movw r0, #0xc34` in `state()`). `gen_evo` is 1,096.

## Compared with G5C.2

G5C.2 hash `bff1c17fa56cdd4fc79e5b69a9ee5df4a85008a80cd6f13cfdc80f94d6c806ac`, 4,271,251 bytes, version `1.3.0-FreePlay0.2c`. Alpha 1 is the rebuild after the session-long recovery lock and the version byte. Extension used fell by 56 bytes.
