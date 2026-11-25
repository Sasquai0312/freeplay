# 04 — Real-Time MIDI and Arpeggiator Feasibility

Pinned: FreePlay `5835b33`.

## Observed MIDI / transport surfaces

| Topic | Finding | Class |
|---|---|---|
| MIDI note-on/off processing | No CFW hooks into MIDI voice or USB/jack MIDI parsers | UNKNOWN |
| Incoming MIDI events | Not hooked; input listener covers pads/knobs/encoder only | VERIFIED_SOURCE (hooks list); UNKNOWN (MIDI RX path) |
| Outgoing MIDI events | `usb_midi_tx` stubbed in vdev HLE; not wrapped by CFW | VERIFIED_SOURCE (`hle.py`); UNKNOWN (jack MIDI TX) |
| Internal sequencer dispatch | Audio IRQ renders voices; CFW touches sample gain / live targets only | VERIFIED_SOURCE (`mod.c`, `cfw_reader_gain`) |
| Clock sync | Tempo float + buffer tempo setter; no clock-source hooks | VERIFIED_SOURCE (`tap.c`); UNKNOWN (ext clock) |
| Transport start/stop/continue | `SEQ_VT_SET_PLAYING` / `IS_PLAYING`; Play button stock | VERIFIED_SOURCE |
| Playback scheduling | Stock; CFW reads playhead / playing flag | VERIFIED_SOURCE |
| Pattern changes | `fw_pattern_switch` symbol present; unused by generative code | VERIFIED_SOURCE |
| Track muting | Not used in CFW | UNKNOWN |
| Note lifecycle (MIDI) | Sample tracks: no note-off (documented). MIDI note-off: UNKNOWN | VERIFIED_SOURCE (sample); UNKNOWN (MIDI) |
| Timing resolution | Step grid 1–64; audio block 128 frames; ms counter `fw_millis` | VERIFIED_SOURCE |
| CPU / memory | Cave 16K + ext 64K; `cfw_ram` full; audio IRQ must be fast | VERIFIED_SOURCE |

## Approach A — Offline arpeggiator (write into native sequence)

**Idea:** Generate arp steps into a buffer; Apply with toggle + setParam + undo.

| Aspect | Assessment |
|---|---|
| Integration | Reuse Slicer edit recipe; Genesis menu; select MIDI track |
| Evidence | Slicer Apply VERIFIED_SOURCE; MIDI params UNKNOWN |
| Risks | Wrong param indices corrupt steps; synth/MIDI confusion; poly packing |
| Unresolved | MIDI param map; polyphony; optional Chance/Repeat |
| Feasibility | **Highest among three**, gated on MIDI param discovery |

Class: VERIFIED_SOURCE (mechanism) + UNKNOWN (MIDI payload).

## Approach B — Real-time arpeggiator on existing sequencer notes

**Idea:** As the sequencer plays a MIDI track, transform scheduled notes into arp streams in real time.

| Aspect | Assessment |
|---|---|
| Integration | Requires hook in MIDI note schedule/dispatch path |
| Evidence | No such hook in FreePlay; sample note-on hooks only (`cfw_tgt_note`) |
| Risks | Audio/MIDI IRQ latency; stuck notes; desync with stock note-offs |
| Unresolved | Dispatch function addresses; note-off pairing; per-track enable |
| Feasibility | **Not evidenced.** Treat as future RE project, not G1 |

Class: UNKNOWN / BLOCKED (missing interfaces).

## Approach C — Real-time arpeggiator on held / incoming MIDI notes

**Idea:** Hold pads or external MIDI notes; CFW emits arp note-ons/offs on the clock.

| Aspect | Assessment |
|---|---|
| Integration | Need MIDI TX injection + clock tick + note state machine |
| Evidence | Pad input VERIFIED; MIDI TX path UNKNOWN; no CFW MIDI out wrapper |
| Risks | Stuck notes on Stop; USB/jack differences; CPU in hot path |
| Unresolved | TX API, channel routing, interaction with native sequencer |
| Feasibility | **Speculative.** Requires substantial RE beyond FreePlay |

Class: UNKNOWN.

## Comparative recommendation

| Rank | Approach | G0 judgment |
|---|---|---|
| 1 | A Offline write | Pursue after MIDI param map |
| 2 | C Held-note RT | Deferred; needs MIDI TX RE |
| 3 | B Sequencer-transform RT | Deferred; hardest / least evidence |

Do **not** assume B or C are feasible. Stock Repeat Type already offers limited arp-like behavior on steps (public manual — INFERRED product feature); Genesis should differentiate by generative control and Apply workflow rather than duplicating Repeat blindly.

## Real-time constraints if later pursued

From FreePlay practice (`DEVELOPING.md`, `mod.c`):

- Anything in audio IRQ: no FS, no alloc, bounded work.  
- Prefer main-loop tick (`cfw_app_update`) for UI and non-audio generation.  
- Always guarantee note-offs on Stop / track mute / menu exit (requirement; implementation UNKNOWN until TX path known).  

Class: VERIFIED_SOURCE (constraints); UNKNOWN (MIDI note-off API).
