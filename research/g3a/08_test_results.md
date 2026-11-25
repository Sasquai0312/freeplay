# 08 — Test Results

## Baselines reproduced

| Suite | Result |
|---|---|
| G2 `make test` | All Genesis tests passed |
| PPF / Thumb field unit | OK |
| Monophonic harness | **6/6** |
| `emu_slice` | **43/43** |
| `vdev_tap` | **6/6** |
| `vdev_slicer` | **38/38** (fresh wav boot snap) |
| `vdev_screen` | **23/23** (clean boot snap) |

Known: menu-tree timeouts / ASan runtime unavailable — unchanged. Host UBSan not required for Thumb objects; G2 host tests run under normal `cc`.

## G3A integration (`g3a_integrate.py`)

**42/42 passed** — `research/g3a/fixtures/g3a_integrate_results.json`

Covers: track discrimination, timing, preview non-mutation, fixtures A/B/D apply+readback+restore, C skip on plen, EMPTY_ONLY (+ inactive occupancy restore), REPLACE_REGION, failure injection, repeated apply, **request-wrapper ABI** (NULL/bad id/capacity, host↔`call_fn` event equivalence A/B/D, preview≡generate), caps.
