# 09 — Known Limitations

- Destinations restricted to verified **16-step** patterns; 32-step deferred.
- MERGE policy deferred; polyphonic Apply off; no channel/CC writes.
- Native soft Undo unverified / not advertised.
- Recovery latch is **session/boot heap**, not flash-persistent.
- Virtual MIDI dispatch / physical audible output unverified (G4).
- `CFW_SKIP_LLD` required for clang-18 builds; full `test_tools` Play 1.6.0 stock absent in this environment.
- Manual seed editing remains encoder low-byte + regen mix (G3B policy).
