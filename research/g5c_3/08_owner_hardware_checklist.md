# Owner hardware checklist

Disposable project. One external MIDI track, with a sound module or a MIDI monitor on that output. Sample tracks stay as they are. Do not use a project you want to keep.

Stages are in order. A failed line is a stop. Do not continue into the next stage to see what else happens.

If Apply shows `Pattern may differ` or `Recovery required`, stop. Write down the track number and whether notes changed. Power-cycle only after that note. The lock lasts until power-on memory is gone, and a restart does not undo notes. Restore the project from the backup made just before this install. Do not press Shift+Back expecting the pattern to come back. Shift+Back does not clear the lock.

## Stop

Stop and roll back (`09_rollback_procedure.md`) on any of:

- The update does not finish, or the unit boots in a loop.
- The display is frozen, blank, or scrambled.
- Pads, keys, or the encoder do not answer.
- A pattern you did not edit changes.
- A note sticks on after Stop.
- Genesis reports an allocation or memory error more than once.
- `Recovery required` or `Pattern may differ` appears.
- The sequencer shows notes you did not Apply, or a track you did not select changes.

## Stage A — boot and the labs that already shipped

1. Update completes and the unit restarts.
2. Version line reads `1.3.0-FreePlay0.2d`.
3. Display and the pads answer.
4. LFO, Matrix, and Slice still open.
5. Menu → Genesis opens. Back from the config screen returns to the menu.
6. On a 16-step external MIDI track, Melody, Click, preview, Bottom Apply. Leave Genesis. Play. Notes sound, and they release when you stop.

## Stage B — 32 and 64

Set the length with the stock length knob **before** opening Genesis. Inside Genesis the knobs are ignored.

1. Length 32. Generate. Turn the encoder and confirm page 2 of the preview. Apply. Play. The later steps sound.
2. Length 64. Confirm four preview pages. Apply. Play.
3. A track you did not select is unchanged.

## Stage C — Turing

1. Engine Turing. RegLen and Length are different numbers on the screen. Lock off.
2. Generate twice without changing anything. The preview matches.
3. Apply once. Bottom again says `Generate first` and does not write a second copy.
4. Raise Evolve, Generate, Apply. The phrase can differ from the first one.
5. Generate, then Back, and confirm the track did not gain those notes.
6. Reset toasts `Register reset` and does not write notes by itself.

## Stage D — Markov

1. Styles Step, Motif, Bass, and Expl each preview.
2. On a disposable melody, Style Learn, Generate. The line can say Pending or Learned only after a successful Apply.
3. Learn on an empty track says `No source notes`.
4. Apply one learned or preset phrase and play it. Notes release on Stop.

## Stage E — Mutation

1. Write a short melody you can remember, or Apply a Turing phrase and leave it.
2. Pit% only, other percents at 0. Generate. Rhythm of the source stays put if Attr includes R, or if you left rhythm percent at 0.
3. RotBy positive, then negative. The screen shows a minus for the negative value.
4. On a 32- or 64-step pattern, protect two steps on different pages with the step pads. Generate. Those steps match the source.
5. Rev with a protected step says `Rev/rot blocked` and does not write.
6. Back from a preview leaves the source. Apply only when the preview is the one you want. If the region is occupied, Apply asks `Replace? Apply again` before it overwrites.

## Stage F — stability

1. Open and leave Genesis several times. Switch Melody, Euclid, Arp, and Evo with the top key.
2. Change the destination track with the select pad. Generate and cancel.
3. Start and stop the transport. MIDI notes release.
4. Tracks you did not edit match what you started with.

Do not try to force a restore failure. If one appears on its own, stop and record the screen text and the track.
