# 04 — Generated Pattern Validation

Engine generation runs **inside** the linked firmware (`gen_fixture` → G2 APIs). Host harness only drives `genesis_diag_fixture`.

| Fixture | Spec | Result |
|---|---|---|
| A | C nat minor, 16 steps, seed 42 | **14** notes applied; note/vel/dur/occupancy readback OK; restore OK |
| B | Euclidean 5/16, D Dorian, seed 123 | **5** notes; readback OK; restore OK |
| C | A minor pent walk, 32 steps, seed 2026 | **Skipped** — native pattern length 16 |
| D | Arp 60/64/67 up, 16 steps | **16** notes; readback OK; restore OK |

Sample track occupancy unchanged across applies. Invalid targets rejected before write.

Raw results: `research/g3a/fixtures/g3a_integrate_results.json`.
