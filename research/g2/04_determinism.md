# 04 — Determinism

## PRNG

xorshift32. `gen_rng_seed(r, 0)` remaps to `0xA5A5A5A5`. Bound uses rejection sampling (unbiased).

## Guarantees

Identical config + seed → identical `gen_event` sequences (verified in unit tests for melody). No `rand()`. No hidden globals.

## KAT

Seed `1` first output: `270369` (unit test).
