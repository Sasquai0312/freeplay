# 06 — G4B Initial Hardware Test (not started)

Disposable project, external MIDI track (`type` MIDI, not a sample track), **16 steps**, **EMPTY_ONLY** only. MIDI monitor or a sound module on that track’s output. No REPLACE_REGION.

1. Device boots.
2. LFO, Matrix, Slice, and Tap still open.
3. Menu → Genesis opens.
4. Encoder changes a value; Shift+encoder changes the parameter.
5. Melody Lab, seed 42, Click Generate. Preview appears.
6. Before Apply, the step pads for that track are still empty.
7. Bottom key Apply on the empty track. Toast shows Applied and the track.
8. Leave Genesis. The sequencer shows those notes.
9. Press Play. The sequence runs.
10. MIDI note-on and note-off are visible or audible.
11. Exit Genesis. Menu and transport still work.
12. If 1–11 pass, repeat a short Euclidean and Arp preview+Apply on the same disposable project.

## Stop — do not retry blindly

Installation error, reboot loop, blank screen, dead controls, notes appearing before Apply, another project changed, a stuck MIDI note, transport dead, or **RECOVERY** on screen.

## If RECOVERY appears

Genesis Apply stays blocked for the rest of that power-on, including after leaving and reopening Genesis or changing lab/track. It is not a successful Apply and it does not mean the pattern was restored. Power-cycle the device. Reload the disposable project if the pattern looks wrong. Do not use Undo as a fix. Do not continue the test on a project you care about.

The latch is RAM for that boot only. A restart clears the latch. A restart does **not** undo notes already written.
