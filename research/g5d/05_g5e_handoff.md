# Handoff to G5E

G5E is the next phase. Its subject is firmware memory optimization. This closeout does not start it.

## Why it is next

Alpha 1 is the owner-confirmed working image, and its memory margins are the ones in `03_resources_and_limits.md`.

- Extension has 4,000 bytes left. The standing reserve is 2,048.
- The code cave has 2,510 bytes left. Its end address is `0x60190072`.
- Static `cfw_ram` is 256 of 256 bytes.
- The largest nested frame estimates are still the G5C.2 estimates. The physical stack ceiling is unknown.
- Free heap on the Play+ is unknown.

## Bounds for that phase

G5E may study those measurements and make a minimal, behavior-preserving reduction where a measurement shows a real cost. It keeps the Alpha 1 musical behavior. It keeps the power-on Apply lock.

G5E does not expand the cave, the extension, or static `cfw_ram`. It does not replace `genesis_midi_apply_ex`. It does not add generators, native polyphony, multi-track Apply, or real-time MIDI hooks. It does not treat the owner’s general confirmation as permission to raise the 2,048 byte extension reserve.

Any later image uses a new version string and a new hash. Rollback for that work is Alpha 1, tag `genesis-v0.2-alpha1-owner-validated`, commit `6e28718856c25af9d1c9a30a26a438e4fe7886a6`.

Virtual-device boots for a later image come from that image. `LINK_MAP` is unset, or it points at that image’s link map.

G6A is not next.
