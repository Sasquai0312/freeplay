# 11 — Limitations and Roadmap

## Deferred (explicit)

Turing / Markov / CA / genetic generators; multi-track harmonic coordination; real-time arp; held-note MIDI; generative CC; native UI/undo/persistence; firmware deploy.

## Current limits

- Host ASan not runnable in this image (missing clang RT).  
- Thumb sizes are per-module object sums, not a linked FreePlay image.  
- Euclidean Bresenham form may differ from other Bjorklund visualizations; pulse count is exact.  
- Weighted melody weights degrees by pitch index modulo degree count (practical, not voice-leading aware).

## Genesis v0.1 subset

Ship: scales + Euclidean/prob rhythm + uniform/walk melody + offline arp + seed/root/range/velocity/gate.  
Optional later: chords, transforms, weighted melody.
