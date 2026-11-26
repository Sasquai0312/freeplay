# 05 — Virtual Device Validation

Harness: `research/g3b/diagnostics/g3b_ui_validate.py`  
Results: `fixtures/g3b_ui_results.json` — **17/17**

| Scenario | Result |
|---|---|
| A Melody C-min seed 42 → preview → exit | PASS; no sequencer mutation |
| B Euclid E(5,16) → 5 hits → regen | PASS |
| C Arp Maj3 Up → 16 hits; mode change regen | PASS |
| D Track change / unsupported path | PASS (invalidate + toast path) |
| E Cancel / exit no mutation | PASS |
| F Repeated open/generate/close | PASS |
| Apply disabled toast | PASS (`Apply in G3C`) |

Screenshots: `research/g3b/screenshots/` (+ `diagnostics/g3b_screenshots.tar.gz`).

Disposable projects only; firmware image not committed.
