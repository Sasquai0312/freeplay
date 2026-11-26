# Regression

Image: `/tmp/fp-g5a/PlayPlus_FreePlay_01.ppf` (not committed). Boot snapshot reused from that image. Counts are pass/total. Nothing skipped was counted as a pass.

| Suite | Result |
|---|---|
| Portable engine `cd genesis && make test` | pass (includes 16/32/64/128 capacity cases) |
| G3A `g3a_integrate.py` | 42/42 |
| G3B `g3b_ui_validate.py` | 17/17 |
| G3C `g3c_e2e_validate.py` | 43/43 |
| `vdev_screen.py` | 23/23 |
| `emu_slice.py` | 43/43 |
| `vdev_slicer.py` | 38/38 |
| `vdev_tap.py` | 6/6 |
| `midi_mono_validate.py` | 6/6 |
| `TestThumbFields` | 3/3 |
| G5A `g5a_length_validate.py` | 60/60 |
| `verify.py` | checksums, hooks, and emu 43/43 pass. Three previously classified literals still fail the address allow-list (`0x10003`, `0xffa50`, `0x6e400002`). The caps mask `0x7060D` is named in `KNOWN_CONSTANTS`. |

`cfw/tests/test_tools.py` as a whole was not run. It still needs `stock_firmware/PolyendPlay_v1.6.0.ppf`, which is not in this tree, and the ld.lld cross-check. That file’s thumb-field tests are the 3/3 row above.

G5A cases inside the 60: native 32 and 64, knob clamp and restore to 16, boundary reads, caps bits, encoder length, preview pages, full-preview readback for melody/Euclid/walk/arp, replace, stale step 20, failure stages including page edge and near-end, recovery-required stage 7, and rejection of step 32 and step 16 when those positions are outside the destination.

No test was hardware. No 128-step native case exists; the knob clamp and the missing caps bit are the 128 result, recorded as blocked rather than passed.
