# 10 — Verified G3 Integration Contract (conceptual)

Independent of G2 engine. **No production adapter code in G1B.**

## Capability flags (only verified → enabled)

```c
typedef uint32_t GenesisMidiCaps;
enum {
    GEN_MIDI_CAP_MONOPHONIC           = 1u << 0, /* ON — §04 */
    GEN_MIDI_CAP_POLYPHONIC           = 1u << 1, /* OFF — §05 */
    GEN_MIDI_CAP_VELOCITY             = 1u << 2, /* ON — param 0 */
    GEN_MIDI_CAP_DURATION             = 1u << 3, /* ON — param 1 */
    GEN_MIDI_CAP_CHANNEL              = 1u << 4, /* OFF — not isolated */
    GEN_MIDI_CAP_MICROTIMING          = 1u << 5, /* OFF — Pitchbend≠microtime product; not mapped for Apply */
    GEN_MIDI_CAP_PROBABILITY          = 1u << 6, /* OFF — Chance idx 18 scale TBD */
    GEN_MIDI_CAP_RATCHET              = 1u << 7, /* OFF — Repeat idx 16 enum TBD */
    GEN_MIDI_CAP_MIDI_CC              = 1u << 8, /* OFF — step CC not UI-verified */
    GEN_MIDI_CAP_NATIVE_UNDO          = 1u << 9, /* ON — cfw_edit_begin mechanism; soft Undo MIDI UI open */
    GEN_MIDI_CAP_NONDESTRUCTIVE_PREVIEW = 1u << 10, /* ON as feasible design — not shipped */
};
```

Evidence references: `03_midi_parameter_evidence.md`, `04_monophonic_validation.md`, `06_transaction_and_undo.md`.

## Parameter binding (verified only)

| Field | Firmware |
|---|---|
| note | `setParam/getParam` index **6** |
| velocity | index **0** (clamp 0..127 on write) |
| duration_steps | index **1** with `units = round(steps * 60)`, clamp 0..3840 |
| occupancy | `is_on` / `fw_step_toggle` |

## Operations

```c
int genesis_midi_validate_track(ctx, track);
  /* type==1 && instrument_ptr==0 → OK; else BAD_TRACK */

int genesis_midi_pattern_length(ctx, track);
  /* TRK_VT_LENGTH */

int genesis_midi_read_step(ctx, track, step, out);
  /* is_on, getParam 6/0/1 */

int genesis_midi_begin(ctx, undo_label);
  /* cfw_edit_begin */

int genesis_midi_upsert(ctx, track, step, ev);
  /* reject bad ranges; ensure on; set note/vel/dur */

int genesis_midi_remove(ctx, track, step);
  /* toggle off if on */

int genesis_midi_commit(ctx);
  /* cfw_refresh_pads (+ toast optional) */

int genesis_midi_cancel(ctx);
  /* no further writes; if begin already pushed, document Undo dependency */
```

## Host guards (mandatory)

- Reject `track` not external MIDI.  
- Reject `step < 0 || step >= pattern_length`.  
- Reject note/velocity outside 0..127; duration outside 0..64 steps before encoding.  
- Never enable inferred indices in production builds.
