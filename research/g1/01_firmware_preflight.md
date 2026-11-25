# 01 — Firmware Artifact Preflight

## Required artifact

| Field | Expected | Observed |
|---|---|---|
| Exact filename | `PlayPlus_v1.3.0.ppf` | **NOT FOUND** |
| Placement | Repo root `/workspace/PlayPlus_v1.3.0.ppf` | Absent |
| SHA-256 | `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876` | From `target.json` / `patches/PlayPlus.patch.json` — VERIFIED_SOURCE expectation |
| Firmware identity | Play+ stock 1.3.0 | Cannot hash missing file — BLOCKED |
| FreePlay compatibility | Exact match required by build/patcher | VERIFIED_SOURCE |
| File size | Unknown until present | BLOCKED |

## Search performed

| Location | Result |
|---|---|
| `/workspace`, `stock_firmware/` | No `.ppf` |
| `/cursor/stores/{user,self}` | No firmware files |
| `/tmp`, `/home/ubuntu`, `/opt/cursor` | No PlayPlus PPF |
| Google Drive search `PlayPlus_v1.3.0.ppf` | Empty |
| App secrets | None available |

Class: VERIFIED_HOST_TEST (absence).

## Version control exclusion (confirmed)

`.gitignore` already excludes:

```
*.ppf
*.bin
*.hex
stock_firmware/
cfw/build/
cfw/release/
```

Class: VERIFIED_SOURCE.

## Expected extraction and build procedure (once artifact is present)

Do **not** commit the file.

```bash
# 1. Place owner-provided stock firmware (gitignored)
cp /path/to/PlayPlus_v1.3.0.ppf /workspace/PlayPlus_v1.3.0.ppf

# 2. Verify identity
sha256sum PlayPlus_v1.3.0.ppf
# must equal 535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876

# 3. Environment
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
# clang + ld.lld + llvm-objcopy + mkfs.fat + mtools on PATH

# 4. Build Play+ FreePlay only
CC=clang .venv/bin/python -c "import sys; sys.path.insert(0,'cfw/tools'); import build; build.build('cfw/targets/playplus-1.3.0/target.json')"

# 5. Virtual device
mkdir -p /tmp/fp && cp cfw/build/playplus-1.3.0/*.ppf /tmp/fp/img.ppf \
  && cp cfw/build/playplus-1.3.0/link.map /tmp/fp/
VPLAY_DIR=/tmp/fp .venv/bin/python cfw/tests/vdev_slicer.py /tmp/fp/img.ppf
```

Original Play (`PolyendPlay_v1.6.0.ppf`) is **not** required for Play+-only G1, unless a developer chooses to build the Play RT target.

## Owner recovery instructions

1. On a computer with the Play+ SD card or Polyend download, obtain **exactly** `PlayPlus_v1.3.0.ppf` from Polyend (official firmware 1.3.0).  
2. Confirm SHA-256 matches `535a903dffea93b9e624c7d0f613823812f7f7b1df74c91e93e3e93e8de94876`.  
3. Provide the file to this Cloud Agent environment by one of:
   - Uploading into the workspace path `/workspace/PlayPlus_v1.3.0.ppf` (gitignored), or  
   - Placing it in an attached store / Drive folder and notifying the agent of the path.  
4. **Do not** commit the file to GitHub.  
5. Re-issue the G1 prompt (or ask the agent to resume G1).  

Until then, G1 experimental criteria remain blocked.

## Decision impact

Missing artifact → **`BLOCKED_FIRMWARE_ARTIFACT`**.
