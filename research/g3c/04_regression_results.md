# 04 — Regression Results

| Suite | Result |
|---|---|
| G3C E2E | **43/43** |
| G3B UI | **17/17** |
| G3A integrate | **42/42** |
| G2 `make test` | All passed |
| emu_slice | **43/43** |
| vdev_tap | **6/6** |
| vdev_slicer | **38/38** |
| vdev_screen | **23/23** |
| Monophonic harness | **6/6** |
| PPF Thumb fields | **3/3** |
| Full `test_tools.py` | Env-limited (Play 1.6.0 stock missing; lld object crosscheck) |

## Screen regression investigation

G3B reported 6/23. Cause: Genesis root menu insert shifted Settings from index **7 → 8**; `vdev_screen.py` still entered MIDI. Not an environmental flake. Fixed in `cfw/tests/vdev_screen.py`. Re-run on G3C: **23/23**.
