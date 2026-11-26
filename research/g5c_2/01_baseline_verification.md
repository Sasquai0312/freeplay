# G5C.2 baseline verification

The phase started from the frozen G5C.1 tip and rebuilt that image before any interface change.

## Provenance

| Item | Value |
|---|---|
| Start branch | `cursor/g5c1-evolution-lab-f3f4` |
| Start HEAD | `5aadd94b997e441f5521f86beaa765de26233f24` |
| Integration commit | `4df7541e327dafe4b5ce8329730dd5e080f19d7f` |
| G5B | `34dc6212bc3f8c6fca38deacca6f1e274e8f9314` |
| G5A | `802c4de2fca77767d63ce7e5dd81a6b7b91030bb` |
| Work branch | `cursor/g5c2-evolution-ui-f3f4` |

`git rev-parse HEAD` on the new branch, before edits, matched `5aadd94`. That commit’s parent is the integration commit.

## Hashes before edits

| Artifact | SHA-256 |
|---|---|
| Stock `PlayPlus_v1.3.0.ppf` | `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` |
| G5C.1 `/tmp/fp-g5c1/PlayPlus_FreePlay_01.ppf` | `1e302709a828e6dad8f4b635f5c5fac5a5583e08b63b6e04930f05be02a6f28c` |

The stock file in the tree matches the frozen hash. The G5C.1 candidate on disk matches the recorded candidate hash. It was not rebuilt for this check; the bytes were hashed again.

## Host baseline

`cd genesis && make test` on the untouched branch, clang 18.1.3, `-O2`:

- All Genesis tests passed
- All evolution tests passed
- All evolution session tests passed
- `sizeof gen_evo=1096`

Portable generators were not modified. The new host test `test_rotate_and_steps` was added after this baseline and is recorded in the regression note.

## Linked baseline that G5C.2 had to keep

From the G5C.1 link map, reconfirmed by the hash above:

| Region | Used | Capacity | Left |
|---|---:|---:|---:|
| Code cave `0x6018ca40..0x60190072` | 13,874 | 16,384 | 2,510 |
| Extension `0x60190b00..0x6019f424` | 59,684 | 65,536 | 5,852 |
| Static `cfw_ram` | 256 | 256 | 0 |

Heap `struct genesis_ui` was 3,116 bytes. `do_generate` own frame 1,120. `do_apply` own frame 624. `gen_xform_mutate_pitch` own frame 552.
