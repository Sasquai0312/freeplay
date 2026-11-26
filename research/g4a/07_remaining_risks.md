# 07 — Remaining Risks

- First physical boot of this image has not happened. Virtual tests are not device proof.
- `verify.py` still flags three in-code constants. Package checksums, region bounds, hooks, and emu_slice passed.
- Diagnostic apply/generate symbols are linked but have no menu. A corrupted call could still reach them; normal UI does not.
- Reset recovery with more than one `.ppf` in `Firmware` may pick a file at random (Play+ emergency manual text). Use a single official file.
- Polyend Tool USB recovery is documented for the original Play manual, not confirmed here for Play+.
- Firmware reinstall does not roll back pattern data.
- Version string does not change versus upstream FreePlay 0.1b; Genesis in the menu is the check.
- 16-step EMPTY_ONLY only for the first hardware pass. No channel/CC/polyphony. Physical MIDI is the point of G4B, not a G4A result.
