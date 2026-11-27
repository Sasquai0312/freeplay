# Playback evidence

| Layer | G6B classification |
|---|---|
| Native storage | Verified. Parameter 10 halfword, ids 0..20, lock bit 10. |
| Native editing | Verified. `setParam` / `getParam` / clear-lock, including while the step is off. |
| Native readback | Verified. Apply compares on, note, velocity, duration, lock, and id before commit. |
| Transaction restoration | Verified for fields `setParam` can store. Stock toggle velocity 7600 is the exception in `03_parameter_lock_contract.md`. |
| Static playback | Not re-tested in G6B. G6A showed the flush at `0x6010afd4` loads the type-3 halfword only when the lock bit is set. No interval table `00 04 07` was found in the stock image. |
| Virtual MIDI dispatch | Not observed. G6A did not enter `usb_midi_tx` at `0x600c6688` while the transport ran. G6B did not repeat that probe. |
| Physical MIDI playback | Unverified. No hardware was flashed. |

Chord note-off, transport stop, and the pitches inside a stock quality were not measured. The owner plan in `12_owner_hardware_test_plan.md` is the first physical check. Do not treat a stored id as a verified voicing.
