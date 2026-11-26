# Boundary read and write

Adapter entry is `genesis_midi_read_step`. A step is valid only when `0 <= step < TRK_VT_LENGTH`. The suite calls that function. It does not treat a raw step-object pointer as success.

## Length 64

| Step | Result |
|---|---|
| 0, 15, 16, 31, 32, 63 | Read accepted. |
| 64, 127 | `GEN_MIDI_ERR_BAD_STEP` (-2). |

Direct note writes at 0, 31, 32, and 63 (note 61, velocity 80, duration 60), then the same toggle that clears a step, restored occupancy to off. Track 0 stayed length 16 and empty. Those writes used the verified toggle (`0x600de6f1`) and `setParam` indices 6 / 0 / 1.

## Length 32

Steps 0, 15, 16, and 31 read. Step 32 is rejected. A preview event placed at step 32 is rejected and does not turn step 0 on.

## Length 16

After the knob returns the track to 16, step 16 is rejected again. This is the v0.1 destination.

An object existing at index 64 while length is 64 is not evidence for a 65th step. The adapter does not write it.
