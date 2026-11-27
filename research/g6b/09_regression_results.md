# Regression results

Primary decision stays `CHORD_LAB_OFFLINE_VALIDATED_RESTRICTED`. No assertion was removed or weakened. Counts below ignore skipped cases; these suites do not report skips.

## Final images

Production `1.3.0-FreePlay0.2f` is 4,268,972 bytes, SHA-256 `4598a98cfd83b9e0c2eec080c51980a2d1d0c9b82876a217232dfc708da184fd`. Diagnostic twin is 4,271,312 bytes, SHA-256 `92d367b5133f5048cd7faad5948383956eda6f216bd6afd45bf01d71900f381b`. The diagnostic image adds `genesis_diag_chord_apply` and the other diagnostic keeps. Production does not export that symbol.

`param_visible` is `noinline` and `optnone` on both images so `-Oz` does not emit a parameter-index bitmask in `0x10000`..`0xFFFFF`. The firmware verifier rejects that window. The comparisons are the same ones the optimized form used.

## Firmware verifier

| Image | Result | Emulated tests |
|---|---|---|
| Production | PASS | 43/43 |
| Diagnostic | PASS | 43/43 |

Both reports: stock record checksums valid, hook bytes and branch targets resolved, cave still ends at `0x60190866`, extension still starts at `0x60190b00`, static `cfw_ram` still `0x20002700`. `CFW_SKIP_LLD=1`. The custom linker `cfw/tools/link.py` was used. ld.lld was not run, so there is no lld comparison.

## Production suites on the final image

| Suite | Result |
|---|---|
| MIDI mono | 6/6 |
| G3B | 17/17 |
| Screen | 23/23 |
| Tap | 6/6 |
| Slicer | 38/38 |
| G5C.2 interaction | 84/84, including the Chords tap and the wrap back to Melody |
| G5C.3 runtime | 12/12. Extension length read back as 60,784. One 3,200-byte UI object. 100 open / generate / cancel / close cycles |

## Diagnostic suites on the final image

| Suite | Result |
|---|---|
| Native chord cases | 25/25, including lock-bit checks, live lengths 16/32/64, page boundaries, and readback failure |
| G3A | 42/42 on the image before the `param_visible` attribute |
| G3C | 43/43 on that same image |
| G5A | 60/60 on that same image |

G3A, G3C, and G5A exercise the Apply and length paths. Those paths were not edited when `param_visible` was marked `optnone`. They were not re-run after that attribute change. The native chord suite was re-run on the final diagnostic image and is the diagnostic failure-injection run.

## Host

| Suite | Result |
|---|---|
| Portable Genesis, Evolution, and the Evolution session | PASS (`make -C genesis test`, exit 0). `gen_evo` is 1,096 bytes |
| GCC 13.3 ASan + UBSan | PASS (`make -C genesis host-asan CC=gcc`) |
| Clang ASan | Not run. `libclang_rt.asan_static-x86_64.a` is not installed |
| Thumb `-Os` size | PASS. `chordprog.o` text+rodata+data is 1,204 bytes. Linked total 14,608 |

`cfw/tests/test_tools.py` was not run. It expects `PolyendPlay_v1.6.0.ppf`, which is not in this tree.

## Chord UI session before the attribute change

On the production image from before `param_visible` was marked `optnone`, the chord UI session passed preview, pagination, exact Apply, replace confirmation, `Has chords` rejection, melody leaving an occupied chord, melody writing locked None, and the recovery lock across lab change, Shift+Back, and close/reopen. The 100-cycle walk visited labs 0..6. One check failed because an 8-byte window included `arm_ms`. The first five bytes, the tail of `gen_evo`, were `0000000000` before and after. The assertion now reads those five bytes. That UI session was not repeated on the final image. The final image's Lab+ order is covered by the 84/84 interaction run.
