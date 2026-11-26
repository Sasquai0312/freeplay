# 03 — End-to-End Validation

Harness: `diagnostics/g3c_e2e_validate.py` → **43/43** (`fixtures/g3c_e2e_results.json`).

| Scenario | Result |
|---|---|
| A Melody Apply + native readback | PASS (14 events, note/vel/dur) |
| B Euclid E(5,16) Apply | PASS (5 occupied) |
| C Arp Maj3 Up + native edit | PASS |
| D Walk melody 16-step Apply | PASS |
| E EMPTY_ONLY reject + REPLACE confirm/cancel | PASS |
| F Stale / no-preview gate | PASS |
| G Unsupported sample track | PASS |
| H Repeated workflow | PASS |
| Recovery lockout | PASS |
| Failure injection 1–7 + EMPTY_ONLY | PASS |

Screenshots: `screenshots/` (+ `diagnostics/g3c_screenshots.tar.gz`).
