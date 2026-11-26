# Virtual-device tests

Harness: `research/g5c_2/diagnostics/g5c2_interact.py`.

```
VPLAY_SNAP=/tmp/fp-g5c2/PlayPlus_FreePlay_01.ppf.boot.snap \
VPLAY_DIR=/tmp/fp-g5c2 \
.venv/bin/python research/g5c_2/diagnostics/g5c2_interact.py \
    /tmp/fp-g5c2/PlayPlus_FreePlay_01.ppf
```

The snapshot was booted from this image for 23,000 ms (`stopped: None`). `LINK_MAP` must be unset or point at `/tmp/fp-g5c2/link.map`. An exported G5A map sends later suites at stale addresses.

Result: 76 checks, 0 failed. Log `/tmp/g5c2-interact.log`. JSON `research/g5c_2/interact_results.json`.

## What the 76 checks cover

- Melody → Euclid → Arp → Evolution → Melody, then back to Turing
- Phrase length clamps to 1 and to the native length
- Turing preview, identical second preview, no native writes, committed register stable on the second preview
- Parameter edit and Engine change drop the preview
- Markov Step, Motif, Bass, Expl, Learn
- Empty Learn toasts `No source notes`, shows Error, does not publish
- Back from that error stays on the page; Step still previews and is not pending
- Signed rotate −1, the negative floor, and the positive ceiling
- Attribute text P, P+R, P+R+V+D, None
- Pads protect steps 1 and 4; a second tap clears step 1; Clr clears the mask
- Reverse plus a protected step toasts `Rev/rot blocked`
- A full grid with only displacement toasts `No note change` and writes nothing
- Phrase length 1 previews
- Length 128 and 0 are not kept (clamped to the live pattern, or replaced with a legal length)
- Native pattern lengths 32 and 64, set with knob 10 before the lab opens, preview, and the encoder reaches page 1
- Step cursor 16 selects page 1 and the pad protects step 16
- Apply once advances the Turing register; a second Bottom is `Generate first`
- Stage-7 Apply toasts `Pattern may differ`, sets recovery, does not mark the session applied, drops the preview
- Bottom while locked toasts `Recovery required`
- Unshifted Back leaves and the lock is still set after reopen
- Shift+Back toasts `Check pattern`, clears the lock, and stays in the lab
- Reset after a 1-step Apply changes the register and does not change notes
- Lock is drawn
- Back from config returns to the stock screen
- The length knob stops at 64 and does not offer 128

## Screenshots

Emulator framebuffer grabs, scale 2, under `research/g5c_2/screenshots/`. These are not mockups.

| File | Screen |
|---|---|
| `01_melody.png` | Melody config |
| `02_turing_config.png` | Turing config |
| `03_turing_preview.png` | Turing preview and register strip |
| `04_markov_step.png` | Markov Step |
| `05_markov_learn.png` | Learn selected |
| `06_markov_empty.png` | Empty source, error |
| `07_mutate_config.png` | Mutation config |
| `08_mutate_protect.png` | Protected steps |
| `09_mutate_blocked.png` | Reverse blocked |
| `10_len32.png` | 32-step pattern |
| `11_len64.png` | 64-step pattern |
| `12_restore_failure.png` | Pattern may differ |
| `13_turing_lock.png` | Lock on |
| `14_stock.png` | After leaving Genesis |

## Harness notes

The bank button is swallowed inside Genesis. The test arms the bank and the length knob on the grid, then opens the menu. Enter’s selection mask can land on track 15; the test presses the select pad on row 0 so the lab follows MIDI track 8, which is the track the length knob changed.

Shift+turn is read from the frames the turn itself draws. An idle frame after the turn often has no new text, so the label has to be taken from that turn.
