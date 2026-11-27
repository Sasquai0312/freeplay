# Compiler and linker experiments

All of these used the Alpha 1 flags as the base: clang 18.1.3, `-Oz`, function and data sections, hard float, Thumb, `link.py`. Cave and extension sizes are linked bytes.

| Configuration | Cave | Extension | Versus 61,536 | Decision |
|---|---:|---:|---:|---|
| `-Oz` baseline | 13,874 | 61,536 | 0 | kept as the starting point |
| `-Os` | overflow |  |  | rejected |
| `-O2` | overflow |  |  | rejected |
| `-fno-inline` | 14,202 | 62,364 | +828 | rejected |
| `-mno-outline` | 13,962 | 61,876 | +340 | rejected |
| `-mllvm -inline-threshold=0` | 13,870 | 61,420 | −116 | rejected, too small |
| pages/UI `-fno-inline` | 13,882 | 60,264 to 60,452 | about −1,100 | rejected, worse than outlining |
| `-fno-unroll-loops` | 13,882 | 60,180 | −1,356 | no change beyond outlining |
| `-fmerge-all-constants` | 13,882 | 60,180 | −1,356 | no change beyond outlining |
| `-fno-jump-tables` | 13,894 | 60,456 | −1,080 | rejected |
| extension LTO, then `-Oz` codegen | 13,882 | 59,856 | −1,680 | rejected |

LTO saved 308 bytes past outlining and was not adopted. It would put the extension through a second code generator. The custom linker still applied the relocations. The extra 308 bytes did not justify that split, and LTO did not approach 4,096 on its own.

`ld.lld` was not used. `CFW_SKIP_LLD=1` stays. The custom linker’s relocation types, Thumb branches, and the hard-float objects are unchanged. Branch range is still checked at ±16 MiB.

Dead extension sections were measured before they were dropped:

| Roots kept | Unreachable extension bytes |
|---|---:|
| Cave code only | 2,114 |
| Cave plus recovery symbols | 2,058 |
| Cave plus every harness `call_fn` symbol | 1,302 |

The adopted linker pass drops only extension sections that are not reachable from cave code, the extension header, or an explicit keep list. Cave placement and relocation of the sections that remain are the same.
