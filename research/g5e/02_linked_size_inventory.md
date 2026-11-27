# Linked size inventory

Object-file size is the `.text` plus `.data` of the relocatable file. Linked size is what `link.py` placed. Production linking drops extension sections that nothing reachable calls, so several object files are larger than their linked footprint.

Largest object files in the production build:

| Object | Bytes |
|---|---:|
| `genesis_ui.c` | 11,657 |
| `pages.c` | 10,027 |
| `sliceview.c` (cave) | 7,218 |
| `lfoui.c` | 6,067 |
| `slicer.c` | 5,594 |
| `mod.c` | 4,292 |
| `genesis_adapt.c` | 3,387 |
| `colors.c` | 2,851 |
| `mutate.c` (cave) | 2,108 |
| `markov.c` | 1,855 |
| `evolve.c` | 1,550 |
| `turing.c` | 1,384 |
| `melody.c` | 854 |
| `arpeggiator.c` | 848 |
| `transform.c` | 794 |
| `rhythm.c` | 678 |
| `dlw.c` (cave) | 292 |

Largest linked functions:

| Bytes | Where | Function |
|---:|---|---|
| 4,928 | extension | `cfw_pages_render` |
| 2,712 | extension | `cfw_genesis_render` |
| 2,604 | cave | `cfw_wave_render` |
| 2,444 | extension | `cfw_lfo_render` |
| 2,132 | extension | `cfw_pages_tick` |
| 2,000 | extension | `cfw_slicer_render` |
| 1,806 | cave | `gen_mutate` |
| 1,672 | extension | `do_generate` |
| 1,572 | extension | `cfw_genesis_input` |
| 864 | extension | `do_apply` |
| 816 | extension | `gen_arp` |
| 636 | extension | `gen_turing_generate` |
| 564 | extension | `gen_markov_generate` |

Name-based linked buckets (a section is counted once, by the first matching name; overlap is approximate):

| Bytes | Region | Bucket |
|---:|---|---|
| 8,772 | extension | knob pages |
| 8,149 | extension | Genesis UI, generate, apply |
| 5,380 | extension | slicer |
| 4,617 | extension | LFO |
| 2,214 | extension | MIDI adapter that survived GC |
| 1,886 | cave | mutation |
| 1,481 | extension | Markov |
| 1,100 | extension | Turing |
| 816 | extension | arpeggiator |
| 462 | extension | evolution session |
| 438 | extension | melody and rhythm |
| 224 | cave | shared `dl` / `text` |

Extension string sections `.rodata.str1.1` are 1,132 and 791 bytes. Alignment padding is 78 bytes in the extension and 21 bytes in the cave. Static CFW RAM is still 256 bytes. No new BSS was accepted; the linker rejects writable sections.

`genesis_adapt.o` is 3,387 bytes and the production image keeps 2,214 bytes of MIDI adapter code. The difference is diagnostic entry points and portable helpers nothing in the firmware calls. Those helpers remain in the host sources. The diagnostic image keeps the harness entry points.
