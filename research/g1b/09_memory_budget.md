# 09 — Memory Budget (G1A build, G1B session)

From `cfw/build/playplus-1.3.0/link.map` (clang-18 local build):

| Region | Used | Capacity | Remaining |
|---|---|---|---|
| Cave (sections `< 0x60190B00`) | **13719** | 16384 | **2665** |
| Extension (`≥ CFW_EXT_BASE`) | **35914** | 65536 | **29622** |

Previous G1 estimates (~13158 cave / ~34712 ext) are slightly below this clang-18 link; treat **13719 / 35914** as current actuals.

## Notes

| Topic | Finding |
|---|---|
| Alignment / section overhead | Included in map section sizes; total linked CFW sections ≈ 49633 bytes |
| Static CFW RAM | Still fully allocated for existing features (`cfw_ram`); no new static MIDI adapter RAM in tree |
| Heap | Stock `operator new` available; G3 should prefer stack/fixed buffers for Apply |
| Diagnostic instrumentation | Python vdev harnesses only — **0** firmware bytes |
| Estimated production mono adapter | Small: validate + get/setParam loop + edit_begin/refresh — likely **≪ 2 KiB** text if kept in ext; measure when implemented |
| Temporary diagnostics in footprint | None in firmware image |

Do not include G1B Python diagnostics in production footprint.
