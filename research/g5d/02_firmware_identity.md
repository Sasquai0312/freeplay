# Firmware identity

The installed image is the G5C.3 candidate. This closeout does not rebuild it and does not change its identity.

| | |
|---|---|
| Version | `1.3.0-FreePlay0.2d` |
| Computer name | `PlayPlus_FreePlay_Genesis_v0_2_Alpha1.ppf` |
| Card name | `PlayPlus_FreePlay_01.ppf` |
| Bytes | 4,271,087 |
| SHA-256 | `39a061780b16832de063d30a248233405be40378e2dffe9833ac15b8bf193228` |
| Implementation commit | `6e28718856c25af9d1c9a30a26a438e4fe7886a6` |
| Implementation branch | `cursor/g5c3-alpha1-hardware-preflight` |
| Tag | `genesis-v0.2-alpha1-owner-validated` on that commit |

The image is not in git.

## Build configuration

Recorded in `research/g5c_3/02_reproducible_build.md` and unchanged here.

- Compiler: Ubuntu clang 18.1.3, `thumbv7em-none-eabihf`, `-mcpu=cortex-m7`, hard float
- Flags: `CFLAGS` in `cfw/tools/build.py` (`-Oz`, freestanding, `-fshort-enums`, `-fno-unwind-tables`, `-ffunction-sections`, `-Werror`)
- Linker: `cfw/tools/link.py` with `CFW_SKIP_LLD=1` (`ld.lld` 18.1.3 is installed and was not used)
- Target: `cfw/targets/playplus-1.3.0/target.json`
- Stock input: `PlayPlus_v1.3.0.ppf`, 4,098,017 bytes, SHA-256 `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876`

```
CFW_SKIP_LLD=1 .venv/bin/python cfw/tools/build.py \
    cfw/targets/playplus-1.3.0/target.json \
    --out /tmp/fp-g5c3 --no-verify
```

Verifier on that image: exit 0, emulator 43/43, `PASS`.

G5C.2 was version `1.3.0-FreePlay0.2c`, 4,271,251 bytes, SHA-256 `bff1c17fa56cdd4fc79e5b69a9ee5df4a85008a80cd6f13cfdc80f94d6c806ac`. Alpha 1 differs by the version byte and the Shift+Back recovery lock.
