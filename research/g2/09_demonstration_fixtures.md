# 09 — Demonstration Fixtures

Directory: `genesis/examples/`  
Archive: `genesis/examples/genesis_demos.zip` (also `/opt/cursor/artifacts/genesis_demos.zip`)

| # | File | Generator | Seed | Root | Scale | Notes |
|---|---|---|---|---|---|---|
| 1 | `01_c_minor_random` | melody_uniform | 1001 | 60 | nat minor | E(12,16) |
| 2 | `02_d_dorian_walk` | melody_walk | 2002 | 62 | dorian | |
| 3 | `03_euclidean_5_16` | rhythm | — | 62 | — | E(5,16) |
| 4 | `04_euclidean_bassline` | euclid+melody | 4004 | 36 | nat minor | |
| 5 | `05_arp_up` | arp_up | 1 | 60 | Cmaj7 | |
| 6 | `06_arp_updown` | arp_updown | 1 | 60 | Cmaj7 | |
| 7 | `07_arp_random` | arp_random | 777 | 60 | Cmaj7 | |
| 8 | `08_chords_I_V_vi_IV` | harmony | — | 60 | major | |
| 9 | `09_chords_voice_led` | harmony VL | — | 60 | major | |
| 10 | `10_mutated_melody` | mutate | 55 | 60 | major | |
| 11 | `11_weighted_melody` | weighted | 321 | 60 | major | |
| 12 | `12_prob_rhythm_walk` | prob+walk | 12 | 57 | min pent | dens 60% |

Each has a `.txt` sidecar (settings + event dump). Tempo 120 BPM, TPQ 480 unless noted.
