# Resource budget

| Resource | Alpha 1 | Production `0.2e` |
|---|---:|---:|
| Code cave used | 13,874 | 15,910 |
| Code cave free | 2,510 | 474 |
| Extension used | 61,536 | 56,136 |
| Extension free | 4,000 | 9,400 |
| Extension reserve | 2,048 | 2,048 |
| Expansion budget | 1,952 | 7,352 |
| Static CFW RAM | 256 / 256 | 256 / 256 |
| UI object | 3,124 | 3,124 |
| `gen_evo` | 1,096 | 1,096 |
| `do_generate` frame | `0x43c` | `0x43c` |
| `do_apply` frame | `0x24c` | `0x24c` |
| Diagnostic extension | same as production | 56,896 |
| Bytes reclaimed in the extension |  | 5,400 |

The 5,400 bytes are the outline (1,356), the cave placement of mutation (2,028), and dead-section removal (2,052), minus 36 bytes for the preview name. The minimum target was 4,096. The preferred target was 8,192. This build meets the minimum and does not meet the preferred target.

Physical stack and heap headroom are unknown.
