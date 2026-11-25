# 08 — Safety, Reliability, and Recovery

Pinned: FreePlay `5835b33`.

**Disclaimer:** Static analysis and host simulation do **not** prove hardware safety. Class: VERIFIED_SOURCE principle (`DEVELOPING.md` / README beta warnings).

## Safeguards for future implementation

| Safeguard | FreePlay precedent | Genesis action |
|---|---|---|
| Firmware compatibility checks | Patcher SHA256 of exact stock input | Keep Genesis builds pinned to Play+ 1.3.0 hashes; refuse other |
| Patch boundary verification | Build verifier rejects unexpected bytes | Run `verify.py` on every build |
| Memory bounds | Fixed arrays; `cfw_ram` assert; ext checksum | Fixed `GenBuffer`; validate track/step ranges |
| Preserve project data | Undo + dirty flags; side-files for CFW state | Always `cfw_edit_begin`; never write CFW state into stock blobs incorrectly |
| MIDI note range | — | Clamp 0–127 before setParam |
| Note-off guarantees | Sample: no note-off (documented) | Offline Apply relies on stock MIDI lengths; RT paths need explicit off — UNKNOWN until TX known |
| Stuck-note prevention | — | Prefer offline; on any RT work: Stop cleanup hook |
| Transport stop cleanup | `SEQ_VT_SET_PLAYING` used in usbdisk | Research hook for Genesis RT only |
| Clock sync | Tempo via project float | Do not desync stock clock |
| CPU budget | Audio IRQ short | Generate offline on Apply / main loop |
| Recovery to stock | Documented dual-file + reset | Document in Genesis README; keep stock PPF on SD |
| Preserve FreePlay | Additive hooks | New hooks only; no remove of LFO/Slicer |

## Recovery procedures (upstream)

From README “Going back to the official firmware” (VERIFIED_SOURCE):

1. Normal: Settings → Firmware → flash `PlayPlus_v1.3.0.ppf`.  
2. Stuck: SD with **only** official PPF in `Firmware/`, power on, press rear reset.  

Bootloader below `0x60030000` not modified by patch (`target.json` `recovery_note`). Class: VERIFIED_SOURCE.

## Operational risks specific to Genesis

| Risk | Severity | Notes |
|---|---|---|
| Wrong MIDI param indices | High | Can scramble step locks; mitigate with vdev golden tests before hardware |
| Whole-track trig replace | Medium | Offer non-destructive modes; Undo |
| Synth track mistaken for MIDI | Medium | Need type discrimination |
| Ext region overflow | Medium | Measure linked size vs 64K |
| Heap exhaustion | Medium | One-shot alloc; fail soft with toast |
| User flashes wrong firmware version | High | Installer already hash-gates; keep that |
| Publishing `.ppf` | Legal/policy | Forbidden (CONTRIBUTING) |

## Beta hygiene

- Back up `Projects/` before any Genesis flash.  
- Do not deploy before a gig.  
- Report issues to project tracker, not Polyend.  

Class: VERIFIED_SOURCE (README norms).
