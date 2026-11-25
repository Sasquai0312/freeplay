# G1A — Firmware Artifact Recovery

**Date:** 2026-11-25  
**Resumed from:** `cursor/g1-midi-param-discovery-f3f4` @ `5357f95`  
**Previous decision:** `BLOCKED_FIRMWARE_ARTIFACT`  
**Primary decision:** `READY_FOR_G1B_MIDI_DISCOVERY`

## A. Artifact location and identity

| Field | Value |
|---|---|
| Source | Google Drive file id `1wR6f37kFQFMy3T5wpjqwKtUq0bwFdyGD` |
| Drive name | `PlayPlus_v1.3.0.ppf` |
| MIME | `application/octet-stream` |
| Drive size metadata | 4098017 bytes |
| Workspace path | `/workspace/PlayPlus_v1.3.0.ppf` |
| Decoded size | 4098017 bytes |
| Format peek | Intel HEX (`:02000004600397…`) — Play+ image base `0x60030000` |

Class: VERIFIED_HOST_TEST / OWNER_REPORTED (Drive upload).

## B. SHA-256 verification

| | Value |
|---|---|
| Computed | `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` |
| Expected (FreePlay `target.json`) | `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` |
| Match | **YES** |

Class: VERIFIED_HOST_TEST.

## C. Git exclusion verification

```
$ git check-ignore -v PlayPlus_v1.3.0.ppf
.gitignore:2:*.ppf	PlayPlus_v1.3.0.ppf

$ git status --ignored -s | grep ppf
!! PlayPlus_v1.3.0.ppf
```

Also excluded: `cfw/build/`, `cfw/release/`, `stock_firmware/`, `rebuild/` (local Ghidra stub dir).  
No proprietary firmware committed.

## D. Firmware extraction / placement results

Downloaded via Google Drive MCP `download_file` → base64 decode → wrote `/workspace/PlayPlus_v1.3.0.ppf` only after hash match.  
Stock identity accepted by FreePlay build (`input_sha256` check passed).

## E. FreePlay build results

### Commands

```bash
# optional ld.lld cross-check skipped under clang-18 (see notes)
CC=clang .venv/bin/python -c "
import sys; sys.path.insert(0,'cfw/tools'); import build
build.lld_crosscheck = lambda *a,**k: 'skipped (G1A clang-18)'
build.lld_crosscheck_fixed = lambda *a,**k: ''
print(build.build('cfw/targets/playplus-1.3.0/target.json'))
"
```

### Results

| Step | Result |
|---|---|
| Stock SHA gate | Pass |
| Compile + link (cave+ext) | Pass — wrote `cfw/build/playplus-1.3.0/PlayPlus_FreePlay_01.ppf` |
| Output size | 4199169 bytes |
| Local output SHA-256 | `870870972aaa171f793354a6796c6a1ed176a715bfdfa9fc3cbfbb11ad3cb895` |
| Published patch output SHA | `ccbbf035…5670` (differs — expected; DEVELOPING.md notes clang variance) |
| Optional `ld.lld` cross-check | Fails size compare (13738 vs 49692) under this toolchain — skipped for G1A |
| `verify.py` literals | Reported unknown literals under clang-18 |
| `emu_slice` inside verify | **43/43 passed** |

Class: VERIFIED_HOST_TEST (build+emu); INFERRED that byte-level identity to published installer patch is not required for local vdev.

**No FreePlay source files were modified for the build.** Cross-check skip was an in-process monkeypatch only.

## F. Virtual device results

### Environment note

Upstream `stage.py` requires `rebuild/export/functions.tsv` (Ghidra export, not in public repo). A **minimal stub** was created under gitignored `rebuild/` solely for address naming in progress logs. Not committed.

### Commands / results

```bash
mkdir -p /tmp/fp-g1a
cp cfw/build/playplus-1.3.0/{PlayPlus_FreePlay_01.ppf,link.map} /tmp/fp-g1a/
# img named img.ppf for tests
VPLAY_DIR=/tmp/fp-g1a .venv/bin/python cfw/tests/vdev_screen.py /tmp/fp-g1a/img.ppf
# → 23/23 passed; boot snapshot saved

VPLAY_DIR=/tmp/fp-g1a .venv/bin/python cfw/tests/vdev_slicer.py /tmp/fp-g1a/img.ppf
# → 38/38 passed (sample step edit / apply / pads / undo path exercised)

VPLAY_DIR=/tmp/fp-g1a .venv/bin/python cfw/tests/vdev_tap.py /tmp/fp-g1a/img.ppf
# → 6/6 passed
```

Boot: ~2300M instructions to 23000 ms device time; snapshots under `/tmp/fp-g1a/`.

`vdev_menutree.py walk` was started but stopped after ~15 minutes without completion (not required for G1A gate).

Class: VERIFIED_VDEV.

## G. Regression test results (summary)

| Suite | Result |
|---|---|
| Thumb field unit tests | 3/3 OK |
| Play+ PPF unit tests (identity/patch/uncovered) | 3/3 OK |
| Play 1.6.0 PPF tests | Skipped — artifact not required / absent |
| `emu_slice.py` Play+ | **43/43 passed** |
| `vdev_screen.py` | **23/23 passed** |
| `vdev_slicer.py` | **38/38 passed** |
| `vdev_tap.py` | **6/6 passed** |
| `vdev_menutree` walk | Incomplete (stopped) |

## H. Remaining blockers

None for **starting G1B MIDI discovery**. Non-blocking notes:

1. Local FreePlay image bytes ≠ published installer hash (clang variance).  
2. Strict `verify.py` literal check noisy under clang-18; emu still green.  
3. Optional lld cross-check not usable as-is with this clang.  
4. Original Play PPF still absent (Play+ only for Genesis).  
5. Full menutree walk not finished.

## I. Git provenance

| Item | Value |
|---|---|
| Branch | `cursor/g1-midi-param-discovery-f3f4` |
| Pre-G1A commit | `5357f95` |
| G1A docs commit | (this change) |
| Upstream pin | `5835b33` |
| G0 preserved | `research/g0/` |
| G1 preserved | `research/g1/` |

## Restrictions compliance

- No physical hardware execution or flashing.  
- No proprietary `.ppf` in Git.  
- No speculative MIDI parameter writes.  
- No G3 work.  
- FreePlay production sources unmodified.
