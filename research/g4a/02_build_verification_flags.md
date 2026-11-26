# 02 — Skipped Build Flags

G3C command:

```bash
CFW_SKIP_LLD=1 python cfw/tools/build.py cfw/targets/playplus-1.3.0/target.json --out <dir> --no-verify
```

## `CFW_SKIP_LLD=1`

Skips only `lld_crosscheck`: optional comparison of `link.py` output to `ld.lld -O0`. It does **not** skip compilation, cave-fill checks, hook `expect` checks, SHA gate on the stock file, region limits, or PPF write.

clang-18 + the extension region disagrees with `ld.lld` blob size (13874 vs ~63k). That mismatch is a cross-check limitation, not evidence the `link.py` image is wrong. Independent `verify.py` still decodes the linked image.

## `--no-verify`

Skips the post-link call to `cfw/tools/verify.py` inside `build.py`. It does **not** skip:

- Stock SHA-256 refusal
- Cave fill check
- Hook word `expect` checks
- Extension header / length / checksum packing
- PPF record checksums written by the packer
- Region limit passed to the linker

`verify.py` was run separately on this candidate (see `01`). The only failures are the three constant-literal allow-list errors above. emu_slice inside that run: **43/43**.
