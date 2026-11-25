# 05 — Test Results

## Commands

```bash
cd genesis && make test
# ASan requested but clang RT libs absent in this image — noted, not claimed passing
```

## Results

| Suite | Result |
|---|---|
| Genesis unit + property (`test_genesis`) | **PASS** |
| SMF write + `mido` note-on/off balance (12 demos) | **PASS** |
| FreePlay `TestThumbFields` | **PASS** |
| FreePlay Play+ PPF identity/patch/uncovered | **PASS** (artifact present) |
| ASan build | **Blocked** — missing `libclang_rt.asan` |

Property fuzz: 50 seeds × Euclidean + walk; notes in 0..127, velocities 1..127, scale membership enforced.
