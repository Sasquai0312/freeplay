# Engine and adapter

## Portable engine

`GEN_RHYTHM_MAX_STEPS` is 128, as in v0.1. Callers own the event buffer. `gen_buf_push` returns `GEN_ERR_CAPACITY` and rolls the buffer back; it does not drop events. New host checks cover melody and Euclidean lengths 16, 32, 64, and 128, a 127-event buffer against 128 hits, and an arpeggio of 128 notes. 129 steps and an arpeggio longer than the buffer are errors.

Portable 128 does not make 128 a native destination. The firmware UI never asks for more than the native length, and that length is at most 64.

## Adapter

`genesis_midi_*` is still the only writer.

- `genesis_midi_caps` now also sets bits 16, 17, and 18 (`GEN_MIDI_CAP_LEN16/32/64`). Bit 0 (monophonic) is unchanged. Bit 19 is clear.
- `GEN_MIDI_DEST_MAX` and `GEN_SNAP_MAX` are 64.
- Step, snapshot, and restore ranges are still `step0 + count <= plen`.
- Duration conversion is unchanged: `round(ticks * 60 / ticks_per_step)`, with the existing overflow reject and the existing 3840 ceiling.
- Overlap on one step is still `GEN_MIDI_ERR_MONO`. Channel is still not written.
- Diagnostic fail stages 8 (first write at step >= 16) and 9 (the write before the last event) restore. They are not on a menu. Stages 1–7 are unchanged.

Any native length from 1 to 64 is editable. 16, 32, and 64 are the lengths with musical fixtures. Lengths above 64 are not advertised and `pattern_length` will not return them.

## UI gate that moved

`PROD_PLEN` (hard reject unless length is 16) is gone. Apply checks `1 <= plen <= 64` and `plen == preview.length`. Requested length still starts at 16 and cannot be turned past `native_len`.
