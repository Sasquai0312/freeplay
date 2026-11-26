# UI architecture

One page remains. `LAB_EVO` is still the lab after Arp. Top still walks Melody → Euclid → Arp → Evo → Melody. G3B’s one-tap Euclid-to-Arp path is unchanged.

Turing, Markov, and Mutation are values of `evo_mode` on that page. They share `adjust`, `value_str`, `cfw_genesis_render`, and `cfw_genesis_input`. There is no second screen object.

## What the screen shows

Config header: mode name (or Melody / Euclid / Arp), track, phrase length, native pattern length.

Turing adds `reg N` and `LOCK` when the lock is on. Markov adds the model line: the preset name, or for Learn one of Pending, Learned, Error, No model. Mutation draws 16 step cells for the current page. A protected step is the accent color. The step cursor is the text color.

The selected parameter is one label and one large value, the same rows G5C.1 used.

Preview header: mode name, or Replace when a replace confirmation is pending. Turing draws the committed register as 16 cells on the current page and prints the register length next to the word `reg`. Markov prints the model line. Mutation draws a cap on a preview bar when that step is protected. Occupancy bars and note names are the existing 16-wide strip. The encoder pages at 16, 32, and 64. Nothing draws a 128-step strip.

Recovery replaces the old `RECOVERY` banner with `MAY DIFFER` and shows `LOCK` beside the parameter. The Apply toast while the lock is held is still `Recovery required`, which is what the G3C lockout test reads.

## State that grew

`pstep[8]` holds protection bits 0..63. `src_n` records how many source events the last mutation read, so a successful mutation that copies the source can say `No note change`. Both sit in the existing heap object, after `mstep` and before `gen_evo`. `sizeof genesis_ui` went from 3,116 to 3,124. `gen_evo` stays 1,096. Harness offsets through `seed` are unchanged (`len` 8, `diag_fail_at` 32, `page` 33, `native_len` 34, `seed` 36).

No static RAM was added. No `malloc`. No new `.data` or `.bss`. The linker still rejects writable sections.

## Bind

`evo_bind` now also hashes the signed rotate, the attribute mask, reverse, quantize, octave, the mutation percents, intensity, and `pstep[8]`. A change to those fields, or any `adjust`, invalidates the preview before Apply reads the bind. Shift+turn during a preview does the same and returns to config.
