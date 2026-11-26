# Control inventory

Taken from the Play+ target headers and the Genesis input path in `cfw/src/genesis_ui.c`. No control was added that the firmware does not already receive.

## Display

| Item | Value |
|---|---|
| Panel | 320×192 at (0, 24) |
| Text column | `TILE_TEXT_X` 28 |
| Subpixel | 16 units per pixel (`DL_VERTEX2F`) |
| Small font | 8, advance about 9 px |
| Large font | 12, used for the current value |
| Preview page | 16 steps (`PAGE_STEPS`) |
| Native phrase ceiling | 64 (`MAX_DEST`, knob 10) |

Existing primitives only: filled rectangles, `text`, `text_f`, key tiles, toasts. No new font, no animation.

## Controls the device already delivers

| Kind | Id | Genesis use |
|---|---|---|
| Button | 0 Top | Next lab: Melody, Euclid, Arp, Evo, then Melody. Blocked while recovery is set. |
| Button | 3 Bottom | Apply. In config, toast “Generate first”. While recovery is set, toast “Recovery required”. |
| Button | 6 Click | Generate. Shift+Click regenerates the seed, then generates. Blocked while recovery is set. |
| Button | 7 Back | From a preview, drop it and stay. From Replace, cancel and stay. From an error, return to config and stay. From config, leave Genesis. Unshifted Back does not clear recovery. Shift+Back while recovery is set re-reads the track and, if it is still MIDI, clears the lock and stays. |
| Button | 10 Shift | Held state. Not a Genesis command by itself. Combined with the encoder or with Back or Click. |
| Encoder kind 4 | — | Config: turn changes the value, Shift+turn changes the parameter. Preview: turn pages 16 steps, Shift+turn changes the parameter and drops the preview. |
| Knob kind 2/3 | — | Swallowed while Genesis is open. Track length is the stock knob and is only useful before the lab allocates. |
| Pad | column 19 | Select. Chooses the track (row, plus 8 when the bank bit is set). |
| Pad | columns 0–15 | Swallowed. On Mutation config, a pad on the selected track toggles protection for `page * 16 + column`. |
| Pad | other columns | Swallowed, no new function. |

There is no long-press event delivered to Genesis. Button 2 (bank) is swallowed once the lab is open, so the bank has to be set on the grid before entering the menu.

## Parameter ranges on the Evolution page

Shared, always shown: Lab, Track, Length (1..native pattern length), Seed (0..65535), Root, Scale, V.min, V.max, Gate%.

| Mode | Extra parameters | Range |
|---|---|---|
| Turing | Dens, P.min, P.max, Engine, RegLen, Evolve, Lock, Reset | RegLen 1..64. Evolve 0..100. Lock on/off. Reset is momentary. |
| Markov | Dens, P.min, P.max, Engine, Style, OctMv | Style Step, Motif, Bass, Expl, Learn. |
| Mutate | P.min, P.max, Engine, Pit%, Rhy%, Vel%, GtVar, Ins%, Rem%, Rev, RotBy, OctSh, Quant, Amt, Attr, Step, Clr | Percents 0..100. RotBy is signed, `-(len-1)..+(len-1)`. Attr is the 0..15 mask shown as P+R+V+D. Step is 0..len-1. Clr is momentary. Density is hidden, same as G5C.1. |

Engine is the mode parameter. The header already shows Turing, Markov, or Mutate, so the knob label stays short.

Length is refreshed from the live pattern before it is clamped, so the ceiling is the native length rather than 64 when the pattern is shorter.
