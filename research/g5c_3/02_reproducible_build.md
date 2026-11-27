# Reproducible build

Source branch `cursor/g5c3-alpha1-hardware-preflight`, started from `e35e47068720ef0512caa0864c6712ce99c510e4`.

Compiler: Ubuntu clang 18.1.3, target `thumbv7em-none-eabihf`, `-mcpu=cortex-m7`, hard float. Flags are `CFLAGS` in `cfw/tools/build.py` (`-Oz`, freestanding, `-fshort-enums`, `-fno-unwind-tables`, `-ffunction-sections`, `-Werror`).

Linker: `cfw/tools/link.py`. `ld.lld` 18.1.3 is installed. The build set `CFW_SKIP_LLD=1`, the same skip G5C.2 used, because clang-18 and this extension layout disagree with `ld.lld` on blob size. The shipped image is the `link.py` image.

Stock input: `PlayPlus_v1.3.0.ppf`, SHA-256 `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876`. Configuration: `cfw/targets/playplus-1.3.0/target.json`.

```
CFW_SKIP_LLD=1 .venv/bin/python cfw/tools/build.py \
    cfw/targets/playplus-1.3.0/target.json \
    --out /tmp/fp-g5c3 --no-verify
```

Output: `/tmp/fp-g5c3/PlayPlus_FreePlay_01.ppf`, 4,271,087 bytes, SHA-256 `39a061780b16832de063d30a248233405be40378e2dffe9833ac15b8bf193228`.

Version string in the image: `1.3.0-FreePlay0.2d`.

## Why this is not the G5C.2 hash

G5C.2 was `bff1c17fa56cdd4fc79e5b69a9ee5df4a85008a80cd6f13cfdc80f94d6c806ac`, 4,271,251 bytes, version `1.3.0-FreePlay0.2c`.

Two source changes:

1. `CFW_VERSION_STRING` is `1.3.0-FreePlay0.2d`. The string is the same length. It lives in the code cave (`cfw_version` at `0x6018fb7c`, 0x13 bytes). The cave span stays `0x6018ca40..0x60190072`.
2. Shift+Back while recovery is set no longer clears the lock. It toasts `Recovery required` and stays. The G5C.2 path called `refresh_track`, cleared `recovery`, and toasted `Check pattern`. That re-read does not restore pattern bytes, so Alpha 1 keeps the v0.1 power-on lock. `04_recovery_review.md`.

The extension end moved from `0x6019fb98` to `0x6019fb60` (61,592 bytes to 61,536). Inserted records went from 3,850 to 3,846. Cave used size stays 13,874. Static `cfw_ram` stays 256.

`verify.py` after the build: exit 0, emulator 43/43, `PASS`.
