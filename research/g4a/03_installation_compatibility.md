# 03 — Installation Compatibility

## Existing FreePlay

Upstream FreePlay documents one update path for a recognized `.ppf`: *Settings → Firmware → Update Firmware*, then pick the file and confirm. Returning to stock first is **not** required by that procedure. The same menu is how a later `PlayPlus_FreePlay_01.ppf` replaces the copy already installed.

The public installer at https://sloppyjoo.github.io/freeplay/ checks the official file and then requires the result to match **upstream** FreePlay byte for byte. It will **not** produce this Genesis candidate. Copy the verified candidate yourself.

## Filename

Upstream README: do not rename; the device only lists firmware files whose names it recognises. Install this candidate as **`PlayPlus_FreePlay_01.ppf`**. The alias `PlayPlus_FreePlay_Genesis_v0_1.ppf` is for the owner’s computer only.

Version string stays `1.3.0-FreePlay0.1b`, so Settings → Firmware may look like the previous FreePlay build. Confirm success by **Menu → Genesis**, not by the version string alone.

## What an update does not claim

- Projects, samples, and settings are not documented as wiped. Upstream says FreePlay is not meant to touch `Projects`, and to back them up anyway.
- Polyend’s Play+ download page says to save the current project and, after an official update, create a new project to initialize config. Use a **disposable** project for Genesis.
- Flashing firmware does **not** restore pattern data that Genesis Apply already wrote.

## Owner must confirm

- The unit is a Play+, not an original Play.
- Current firmware already shows FreePlay (LFO / Matrix / Slice present).
- A copy of the **currently working** `PlayPlus_FreePlay_01.ppf` is saved off the card before it is replaced (same filename).
- Official `PlayPlus_v1.3.0.ppf` (SHA `535a903d…4876`) is on hand.
