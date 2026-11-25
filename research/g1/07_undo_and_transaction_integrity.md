# 07 — Undo and Transaction Integrity

## Existing mechanism (source)

Stock recipe used by FreePlay (`slice.c`):

```
cfw_edit_begin(ctrl, "Label");  // fw_undo_push + dirty flags
// ... mutations under cfw_edit_lock ...
cfw_refresh_pads(ctrl);
```

| Property | Finding | Class |
|---|---|---|
| Grouping | One `fw_undo_push` before a batch → one undo for that batch (sample slicer) | VERIFIED_SOURCE |
| Single-step edit | Supported by same recipe | VERIFIED_SOURCE |
| Multi-step edit | Slicer places many steps under one "Slice" undo | VERIFIED_SOURCE |
| Unrelated steps/tracks | Selective setParam / toggle only touched tracks | VERIFIED_SOURCE (sample practice) |
| Cancel before commit | UI Back without Apply (Slicer) leaves pattern unchanged | VERIFIED_SOURCE |
| Invalid target | Toast + early return (e.g. no sample trigs) | VERIFIED_SOURCE |
| Lock release on failure | Callers unlock in normal paths; failure mid-lock needs care | INFERRED risk |
| MIDI-specific undo | Not tested | BLOCKED |
| Native redo API | Soft-key “Redo” string only | UNKNOWN |

## MIDI characterization (blocked)

Required G1 checks (single-step, multi-step, undo restore, unrelated preservation) were **not run**.

## Adapter requirements if atomic undo unavailable

If future discovery shows MIDI writes do not participate in `fw_undo_push`:

1. Document gap.  
2. Prefer stock path first.  
3. Only then consider CFW-side snapshot of touched steps (heap buffer) — last resort; not proposed for production without evidence.

## Conclusion

Transaction pattern is **ready to reuse** for MIDI once write path is verified. MIDI undo integrity itself is **unproven**.
