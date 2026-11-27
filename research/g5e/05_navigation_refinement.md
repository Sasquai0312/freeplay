# Lab+ navigation

The Alpha 1 selector was Melody, Euclid, Arp, Evo. Turing, Markov, and Mutation were an Engine parameter inside Evo. The owner could not find Markov and Mutation until they opened that parameter.

The selector is now:

1. Melody
2. Euclid
3. Arp
4. Turing
5. Markov
6. Mutate

Top still cycles the lab and wraps. One `bind_lab` sets `evo_mode` and calls `gen_evo_set_mode` only when the mode actually changes. All three labs use the same Evolution state, renderer, and parameter path. There is no second UI.

`gen_evo_set_mode` clears an unpublished preview and a pending Learn. It does not clear a committed Turing register or a published Markov model. Switching to Melody, Euclid, or Arp does not rewrite `evo_mode`. The next Turing, Markov, or Mutate visit binds that lab’s mode again.

The Engine parameter index is still in the enum, so later parameter offsets did not move. `param_visible` does not show it. Shift+encoder cannot land on it.

Phrase length is the parameter labeled Phrase. Turing register length stays RegLen, and the config row labels it `reg`. The config title is the lab name. A preview of Melody, Euclid, or Arp still says Preview and now also prints the lab name. Turing, Markov, and Mutate previews use the lab name as the title. Seed, `pg i/n`, Markov Learn text, the LOCK tag, signed RotBy, and the protected-step marks are unchanged.

Shift+Back during recovery still toasts “Recovery required” and does not clear the lock. Top during recovery does the same and does not change labs.

Controls that stay on the same buttons: preview and regenerate on Click, Shift+Click regenerates the seed, Shift+encoder selects a parameter, the encoder edits the parameter or pages a preview, Bottom applies, Back cancels. Track select is unchanged. Apply is still one-shot, exact-preview, EMPTY_ONLY unless Replace is confirmed, and it still rejects a stale preview.
