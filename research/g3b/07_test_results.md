# 07 — Test Results

## Baselines (this phase)

| Suite | Result |
|---|---|
| G3A `g3a_integrate.py` | **42/42** |
| G2 `make test` | All Genesis tests passed |
| `emu_slice` | **43/43** |
| `vdev_tap` | **6/6** |
| `vdev_slicer` | **38/38** |
| Monophonic harness | **6/6** |
| Firmware artifact SHA | `535a903d…4876` (4098017 B) |
| `vdev_screen` | **6/23** — Settings brightness/dim rows not reached with available snap (historical menu/settings env limitation; not expanded) |
| `test_tools.py` PPF | Env-limited (missing Play 1.6.0 stock; lld crosscheck `cfw_ext_ok`) |

## G3B UI

| Suite | Result |
|---|---|
| `g3b_ui_validate.py` | **17/17** |

## 32-step follow-up

Fixture C still skipped (`plen` 16). Bounded probe: no verified length-setter symbol / UI mapping; see `fixtures/g3b_32step_experiment.json`.
