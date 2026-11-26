# 08 — G4 Hardware Test Plan (owner)

**Do not begin G4 in this phase.** Prerequisites before first flash:

1. Verified owner rollback / recovery procedure for Play+ 1.3.0 FreePlay updates.
2. Full backup of all projects of value.
3. Candidate hash match (`06_firmware_candidate.md`).
4. Disposable empty project only for first Apply tests.

## Suggested checks (disposable project)

- Boot; existing FreePlay menus (Slicer, LFO, Tap, Settings brightness).
- Genesis open/exit; Melody / Euclid / Arp preview.
- EMPTY_ONLY Apply on empty MIDI track; inspect steps; edit note/vel/length natively.
- External MIDI note-on/off to a known sound source (G4 claim).
- Transport/tempo unchanged unexpectedly.
- Occupied region → Replace confirm / cancel.
- Stop on any recovery-required or unexplained data loss.

If rollback cannot be established, keep status **HARDWARE_BLOCKED**.
