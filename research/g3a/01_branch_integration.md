# 01 — Branch Integration

## Provenance

| Ref | Value |
|---|---|
| G1B branch | `cursor/g1b-midi-discovery-f3f4` @ `9a6c99b` |
| G2 branch | `cursor/g2-portable-engine-f3f4` @ `6de09c9` |
| G3A branch | `cursor/g3a-native-adapter-f3f4` |
| Integration | `git checkout -b` from G1B, then `git merge 6de09c9` → `72d95d7` |
| Base for PR | `cursor/g0-architecture-audit-f3f4` (agent default); do **not** merge to `main` |

Historical G1B and G2 branches are preserved on the remote.

## Firmware preflight

| Field | Expected | Observed |
|---|---|---|
| File | `PlayPlus_v1.3.0.ppf` | `/workspace/PlayPlus_v1.3.0.ppf` |
| Size | 4098017 | 4098017 |
| SHA-256 | `535a903d…4876` | match |

Proprietary firmware is gitignored; not committed.
