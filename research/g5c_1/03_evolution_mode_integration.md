# Evolution mode integration

The page has one mode knob, `P_EMODE`: Turing, Markov, Mutate. `adjust` writes the knob and calls `gen_evo_set_mode`. That clears `proposed_ok` and `pending_ok` and sets the phase to EMPTY unless the phase is already APPLIED. It does not copy another mode’s proposed register into the committed Turing state, and it does not publish a Markov model.

## Turing

`fill_turing` builds a `gen_turing_cfg` from knobs the validated API already accepts.

| Control | Source | Notes |
|---|---|---|
| Register length | `P_ELEN`, 1–64 | Independent of the phrase. The register loops. |
| Phrase length | existing Length, clamped to `native_len` 1–64 | Passed as `cfg.steps`. |
| Evolution probability | `P_EVOL`, 0–100 | |
| Lock | `P_LOCK` | |
| Density | existing Dens | `rest_prob = dens >= 100 ? 0 : 100 - dens`. Higher Dens is fewer rests. Default 70 → rest 30. |
| Scale and root | existing knobs | |
| Pitch range | existing P.min / P.max | |
| Seed | existing u16 | 0 becomes 1 before `gen_turing_init`. |
| Velocity | V.min | V.max stays on screen and is not the Turing note velocity. |
| Gate | existing Gate | Clamped to 4..480 ticks so `gen_native_duration_ok` accepts it. Melody, Euclid, and Arp still use the unclamped `gate_ticks`. |
| Reset | `P_RESET` | A turn calls `gen_evo_turing_reinit` and `invalidate`. The knob does not stick and does not write notes. |

Preview calls `gen_evo_turing_preview`. That reads the committed register, writes events into the caller buffer, and stores the next register in `proposed`. The committed bytes are not replaced until `gen_evo_apply_result(e, 1)` after a successful native Apply. A register-defining change (seed, root, pitch ends, scale, register length, rest probability) reinitializes the committed register first. `gen_turing_init` rejects a bad configuration before it writes, so a failed init does not need a rollback copy.

Repeated previews with the same configuration and the same committed register produce the same events, because generate does not commit the proposed register. Lock and zero evolution keep the cell array. Both are covered by `tests/test_evo_session.c`.

## Markov presets

Styles are the four validated presets plus learned mode: Stepwise, Motif, Bassline, Exploratory, Learn (`GEN_MARKOV_STEPWISE` through `GEN_MARKOV_LEARNED`).

| Control | Source |
|---|---|
| Preset or learned | `P_STYLE` |
| Phrase length | existing Length, 1–64 |
| Scale and root | existing knobs |
| Pitch range | P.min / P.max |
| Density | same rest mapping as Turing |
| Seed | existing u16, 0 → 1 |
| Octave motion | `P_MOCT` |
| Velocity | V.min |
| Initial degree | fixed at 0 |
| Gate | `evo_gate`, 4..480 |

A preset preview builds the chain in `gen_evo.scratch` and does not replace `model`. `pending_ok` is cleared, so a later Apply cannot publish a stale learn. The integer-weighted first-order chain inside `gen_markov_preset` / `gen_markov_generate` is unchanged.

## Learned Markov

Generate with Style = Learn reads the selected native track through `genesis_midi_read_step` into `g->evo_src` (not the preview buffer, and not the source pattern). Off steps are skipped. An on step with a note or velocity outside 1..127 (note 0..127) returns `GEN_ERR_PARAM`. A duration that fails `gen_native_duration_ok` returns `GEN_ERR_TIMING`. The read does not call a write.

`gen_evo_markov_learn` passes that buffer to `gen_markov_learn` with `gap_ticks = 0` and scratch = `e->scratch`. Time gaps do not break the chain. An out-of-scale note still breaks it, which is the G5B rule inside `gen_markov_learn`. The destination model is only replaced after the scratch chain is complete; a timing, pitch, or empty failure returns before that copy. The published `model` is not written by learn.

Preview of learned mode uses the pending model when `pending_ok` is set, otherwise the published model when it is a ready learned model. If neither exists, the status is `GEN_ERR_EMPTY`. No preset is substituted. Generate hashes the model before and after and fails if generation mutated it. Apply with `native_ok != 0` copies pending onto `model` once. A failed Apply leaves `model` as it was.

An empty source is `GEN_ERR_EMPTY`. The toast is “Empty source”.

## Mutation

Preview reads the same native adapter into `evo_src`, then calls `gen_evo_mutate_preview`. The output buffer is `do_generate`’s stack `storage[64]`. Those two buffers do not alias. The session rejects an aliased output before `gen_mutate` runs.

| Control | Knob | API field |
|---|---|---|
| Pitch mutation | `P_MPITCH` 0–100 | `pitch_prob` |
| Rhythm displacement | `P_MDISP` 0–100 | `displace_prob` |
| Velocity variation | `P_MVEL` 0–100 | `velocity_prob`, range V.min..V.max |
| Gate variation | `P_MGATE` 0–100 | `gate_prob`, range `[max(4, gate/2), gate]` |
| Note insertion | `P_MINS` 0–100 | `insert_prob` |
| Note removal | `P_MREM` 0–100 | `remove_prob` |
| Reverse | `P_MREV` | `reverse` |
| Rotate | `P_MROT` 0..len-1 | `rotate_steps` as a non-negative count |
| Octave | `P_MOCTV` 0..4 | `octave` = value − 2, so −2..+2 |
| Scale quantization | `P_MQUANT` | `quantize` |
| Amount | `P_MINT` 0–24 | `intensity` |
| Attribute protection | `P_MPROT` 0–15 | bits pitch 1, rhythm 2, velocity 4, duration 8 |
| Step protection | `P_MSTEP` | 0 = none, otherwise step `value-1` if it is inside the phrase |
| Seed | existing u16 | 0 → 1 |

Operation order, collision policy, and protection semantics are the G5B `gen_mutate` implementation. This phase does not change them. A fully occupied phrase cannot accept insertion or displacement: the occupied destination is left in place, and the session test “occupied grid does not move” checks an 8-step full grid at displace probability 100. Reverse or rotate combined with rhythm protection, or with any protected step, returns `GEN_ERR_UNSUPPORTED` before the output is published. The toast is “Protected”. A capacity failure clears the output count and does not publish a preview.

## Controls the knobs do not represent directly

- Negative rotate. The API accepts a signed step count. The knob is 0..len−1.
- More than one protected step. The API has a 16-byte bitset. The knob arms one step inside the current phrase, or none.
- Attribute protection is one 4-bit value, not four separate switches.
- Phrase length, seed, scale, root, and pitch range are the existing Genesis knobs. They are not duplicated.
- Density is a rest probability with the mapping above. Mutate hides Dens because mutation has no rest probability.
- Turing and Markov velocity is V.min. V.max still bounds mutation velocity.
- Evolution gate is clamped to 4..480. A gate the melody lab would emit below 4 ticks is raised so the native duration check can accept the event.
- Portable 128-step calls still work if a host caller invokes `gen_turing_generate`, `gen_markov_generate`, or `gen_mutate` directly. The native session and the Length knob reject 65 and 128.
