# Owner hardware test plan

Do this on the Play+ with a MIDI monitor on the external MIDI track. Do not treat the emulator results as playback proof.

Candidate: `PlayPlus_FreePlay_Genesis_v0_2_Alpha2.ppf`, version `1.3.0-FreePlay0.2f`. Confirm the SHA-256 in `11_candidate_manifest.md` before installing. Keep the 0.2e card image available.

## Setup

1. External MIDI track, the track Genesis shows as `MIDI ok` (track 9 on a stock layout).
2. Monitor that prints note-on, note-off, channel, and time.
3. Pattern length 16 for the first pass. Repeat the chord apply on lengths 32 and 64 only after the 16-step notes look right.

## Checks

1. Open Genesis. Lab+ reaches Chords without an extra mode. The opening click must not skip a lab and must not clear a recovery lock if one is already set.
2. Preview I–V–vi–IV on C natural minor. The grid shows roots and short chord names. The track is still empty.
3. Cancel. The track is still empty.
4. Apply. The monitor, while the transport runs, should show simultaneous note-ons for each chord, one channel, then note-offs. Record the pitches for at least: Major (id 3), Minor (4), Sus2 (7), Sus4 (8), Dim (5), Maj 7th (11), Min 7th (12), Dom 7th (14).
5. Stop the transport. Notes must release. Start again on the next chord and confirm the previous chord does not stick.
6. Hold versus Step. Hold is one trigger per change. Step retriggers each step. Compare the note-off times with the gate.
7. A phrase longer than one cycle repeats the same roots and ids.
8. Melody Apply on a fresh step writes a single note, not a chord. Melody Apply while a chord is present asks before replacing and does not change the chord if you cancel.
9. Mutate on that phrase toasts `Has chords` and does not edit the steps.
10. If Apply ever toasts `Pattern may differ`, stop. Do not Apply again. Power-cycle to clear the lock, or roll back.

## Rollback

Install `PlayPlus_FreePlay_Genesis_v0_2_Alpha1_Optimized.ppf`.

| Field | Value |
|---|---|
| Version | `1.3.0-FreePlay0.2e` |
| Size | 4,255,906 bytes |
| SHA-256 | `777461547a86d635c1c1d4bd7dafd01c4e406089cd2d53083a266768269bfdc0` |

That image is the hardware-validated Alpha 1. G6B does not replace it until these playback checks are accepted.
