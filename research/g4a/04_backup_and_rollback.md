# 04 — Backup and Rollback

## Backup (mandatory)

From the upstream FreePlay README: copy the whole SD card. Projects live in `Projects`. Also keep, off the card:

- The official `PlayPlus_v1.3.0.ppf`.
- The **current working** FreePlay file (same recognized name), saved under a different folder name on the computer before the card copy is replaced.

Samples are on the card with the project folders. There is no separate verified export for every device setting beyond the SD copy and `cfwSettings` if present.

## Normal rollback (device boots)

Same menu as install (upstream README “Going back to the official firmware”; Polyend Play+ downloads page uses Main Menu → System Settings → Firmware → Update Firmware):

- Official 1.3.0: pick `PlayPlus_v1.3.0.ppf`.
- Previous FreePlay: put that saved file back on the card **as** `PlayPlus_FreePlay_01.ppf` and select it.

This restores the application image. It does **not** undo sequencer edits already stored in a project.

## Device will not boot / update UI unavailable

Upstream FreePlay README (this repository):

1. Card on a computer.
2. In `Firmware`, leave **only** `PlayPlus_v1.3.0.ppf`. Move every other `.ppf` off the card.
3. Eject, reinsert.
4. Press the small reset button on the back. It installs that file. FreePlay does not patch the bootloader (`target.json` recovery note).

Polyend Play+ emergency text (manual reprints, not the short downloads page) says a reset may install a **random** `.ppf` if several remain in `Firmware`. That is why the single-file step is mandatory. Do not treat “several good files on the card” as a controlled recovery.

The original Play manual also describes a Polyend Tool USB flash. That step was **not** found in the Play+ downloads installation guide used here, so it is **not** a claimed Play+ procedure.

## Not established

- Recovery if the bootloader itself is damaged (this candidate does not modify it; damage from a power loss mid-flash is still a residual risk).
- Restoration of musical data by reinstalling firmware.
