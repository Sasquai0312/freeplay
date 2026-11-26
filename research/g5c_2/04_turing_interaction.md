# Turing interaction

The generator in `turing.c` was not edited. The page only exposes the existing request.

| Control | What it changes |
|---|---|
| Length | Phrase length, 1..native pattern length. This is not the register. |
| RegLen | Register length, 1..64. The config row prints `reg N` beside it. |
| Evolve | Probability 0..100. |
| Lock | On shows `LOCK` in the error color on the config row and on the preview. |
| Dens, scale, root, P.min, P.max, V.min, Gate%, Seed | Same fields G5C.1 already sent. Velocity is V.min. Gate for Evolution is still clamped to 4..480 ticks inside `evo_gate`. |
| Reset | Momentary. Calls `gen_evo_turing_reinit` immediately. No confirmation. Toast `Register reset`. Does not write native notes. |
| Click | Preview. Does not copy `proposed` onto `committed`. |
| Bottom | Apply. On `GEN_MIDI_OK`, `gen_evo_apply_result(..., 1)` runs once. |

## Register versus phrase

The header `len` is the phrase. The `reg` figure is the shift-register length. The preview draws the committed cells, paged with the same 16-step encoder page as the notes, and only while `committed.ready` is set.

## Preview and repeat

The first preview may initialize the committed register when it is empty or the configuration signature changed. That is the existing `gen_evo_turing_preview` rule. A second Click with the same inputs reproduces the event bytes and leaves the committed register bytes unchanged. The virtual-device run recorded both.

Apply copies the proposed register onto the committed one exactly once. A second Bottom without a new preview toasts `Generate first` and does not write.

## Reset

Reset writes the committed register now. It does not ask for confirmation and it does not call the native writer. If evolution is 0 and the phrase length is a multiple of the register length, one generate lands on the same bytes as a fresh init, because the position wraps. The interaction test uses phrase length 1 so the advanced position is visible, then Reset, and the register bytes change while the native notes stay put.

Reset leaves the session phase at `APPLIED` if it was already applied. The next preview’s `publish` sets `VALID` again. Reset is not a second commit.
