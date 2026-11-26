# Linker and candidate

Build, from `/workspace`, Ubuntu clang 18.1.3:

```
CFW_SKIP_LLD=1 .venv/bin/python cfw/tools/build.py \
    cfw/targets/playplus-1.3.0/target.json \
    --out /tmp/fp-g5c2 --no-verify
```

Version string in the image: `1.3.0-FreePlay0.2c`.

The `.ppf` is not committed. The build directory is `/tmp/fp-g5c2/`. A downloadable copy is `/opt/cursor/artifacts/PlayPlus_FreePlay_G5C2_offline.ppf`, with the same SHA-256 as the build output. The label on that copy is offline-tested, not hardware-validated.

## Linked regions

| Region | Span | Used | Capacity | Left | Versus G5C.1 |
|---|---|---:|---:|---:|---:|
| Code cave | `0x6018ca40..0x60190072` | 13,874 | 16,384 | 2,510 | end address unchanged |
| Extension | `0x60190b00..0x6019fb98` | 61,592 | 65,536 | 3,944 | +1,908 |
| Static `cfw_ram` | `0x20002700` | 256 | 256 | 0 | unchanged |

The extra 1,908 bytes are under the 3,804 preference. The 3,944 bytes left are over the 2,048 reserve. Both figures are the span from the first section address to the end of the last section, which is how G5C.1 counted (the G5C.1 end was `0x6019f424`).

`verify.py` on this image: every stock record outside the cave, the hooks, and the inserted extension records is byte-identical and in order. 889 stock records changed (data and checksum only). 3,850 data records inserted. File size 4,271,251 bytes (G5C.1 was 4,265,888).

## Other link checks

- Branch targets and addresses:  the verifier decoded the image and reported every branch resolved, then emulated 43/43.
- `.ARM.exidx` is still dropped. The link map has no exidx section.
- No `.data` / `.bss` in the custom link. `link.py` still raises on writable and `SHT_NOBITS` sections.
- Heap object 3,124 bytes. `gen_evo` 1,096. No new static RAM. No new allocation.
- Symbols used by the UI (`gen_evo_*`, `gen_mutate_protect_step`, `genesis_midi_apply_ex`) resolve inside the extension. Cave hooks are the same entry points as G5C.1.

## Hashes

| Artifact | SHA-256 |
|---|---|
| Stock `PlayPlus_v1.3.0.ppf` | `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` |
| G5C.1 candidate | `1e302709a828e6dad8f4b635f5c5fac5a5583e08b63b6e04930f05be02a6f28c` |
| G5C.2 `/tmp/fp-g5c2/PlayPlus_FreePlay_01.ppf` | `bff1c17fa56cdd4fc79e5b69a9ee5df4a85008a80cd6f13cfdc80f94d6c806ac` |

## Label

Offline-tested on the virtual Play+. Not hardware-validated. It was not flashed, installed, or executed on a physical device.
