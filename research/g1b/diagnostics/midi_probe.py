#!/usr/bin/env python3
"""G1B diagnostic: MIDI track discrimination + parameter differential discovery on vdev.

Read-only observations and controlled native-UI / verified-API edits on a disposable
virtual project. Does not modify FreePlay production sources.

usage:
  VPLAY_DIR=/tmp/fp-g1b python research/g1b/diagnostics/midi_probe.py \\
      /tmp/fp-g1b/img.ppf [--snap BOOT.snap] [--phase all|tracks|params|mono|poly|undo|midi_tx]
"""
from __future__ import annotations

import argparse
import json
import os
import struct
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'cfw', 'vdev'))
sys.path.insert(0, os.path.join(ROOT, 'cfw', 'tests'))

from vdev_slice import Device, BOOT_MS  # noqa: E402

BTN_TOP, BTN_BOT, BTN_CLICK, BTN_BACK, BTN_PLAY, BTN_SHIFT = 0, 3, 6, 7, 8, 10
BTN_SOFT3 = 9  # often Undo / soft key under display
SEL_COL = 19
FW_STEP_TOGGLE = 0x600de6f1
FW_UNDO_PUSH = 0x600ecfb9
STEP_VT_IS_ON = 0x0c
STEP_VT_SET_PARAM = 0x10
STEP_VT_GET_PARAM = 0x18
STEP_VT_TRACK_TYPE = 0x40
TRK_VT_LENGTH = 0x04
N_PARAMS = 22
SAMPLE_VT = 0x20001ddc
SYNTH_VT = 0x20001d74
INST_VT_SAMPLE = 0x600892e4
APP = 0x20018c18
SEQ = 0x2001e30c
USB_MIDI_TX = 0x600c6688
USB_MIDI_DEV = 0x200175cc

OUT = os.path.join(HERE, '..', 'fixtures')
os.makedirs(OUT, exist_ok=True)


def log(msg):
    print(msg, flush=True)


class MidiDevice(Device):
    def track_type(self, t, step=0):
        st = self.step(t, step)
        return self.vp.call_fn(self.vfn(st, STEP_VT_TRACK_TYPE), st) & 0xFF

    def step_vt(self, t, step=0):
        st = self.step(t, step)
        return self.vp.r32(st)

    def track_len(self, t):
        vp = self.vp
        pv = vp.r32(self.sel + 0x5c)
        trk = vp.call_fn(self.vfn(pv, 0), pv, t)
        return vp.call_fn(self.vfn(trk, TRK_VT_LENGTH), trk)

    def set_param(self, t, i, idx, value):
        st = self.step(t, i)
        self.vp.call_fn(self.vfn(st, STEP_VT_SET_PARAM), st, idx, value & 0xFFFFFFFF)

    def toggle(self, t, i):
        return self.vp.call_fn(FW_STEP_TOGGLE, self.sel, t, i)

    def dump_params(self, t, i):
        return {k: self.param(t, i, k) for k in range(N_PARAMS)}

    def step_bytes(self, t, i):
        st = self.step(t, i)
        return bytes(self.vp.mu.mem_read(st, 0x2A))

    def select_track(self, t):
        """Select pad column 19 on track row t (0-based)."""
        self.s.pad(t % 8, SEL_COL, hold_ms=50, after_ms=200)
        if t >= 8:
            # bank to tracks 9-16 if needed — MODE_BANK at mode+0x112
            mode = self.vp.r32(self.ctrl + 0x24b4)
            bank = self.vp.mu.mem_read(mode + 0x112, 1)[0]
            if not bank:
                # try side pad / bank toggle — soft exploration: hold shift+select
                self.vp.inputs.button(BTN_SHIFT, 0)
                self.s.run_ms(40)
                self.s.pad(t % 8, SEL_COL, hold_ms=50, after_ms=150)
                self.vp.inputs.button(BTN_SHIFT, 1)
                self.s.run_ms(100)

    def page_texts(self, ms=100):
        self.s.run_ms(ms)
        return [a[2] for n, a in self.vp.eve.cmd_log if n == 'TEXT' and len(a) > 2]

    def clear_log(self):
        self.vp.eve.cmd_log.clear()


def classify_tracks(d: MidiDevice):
    rows = []
    for t in range(16):
        try:
            tt = d.track_type(t)
            vt = d.step_vt(t)
            ln = d.track_len(t)
            on0 = bool(d.is_on(t, 0))
            rows.append(dict(
                track=t, track_type=tt,
                step_vt=f'0x{vt:08x}',
                vt_kind=('sample' if vt == SAMPLE_VT else 'synth_or_midi' if vt == SYNTH_VT else 'other'),
                length=ln, step0_on=on0,
            ))
        except Exception as e:
            rows.append(dict(track=t, error=str(e)))
    return rows


def diff_dicts(a, b):
    keys = sorted(set(a) | set(b))
    return {k: (a.get(k), b.get(k)) for k in keys if a.get(k) != b.get(k)}


def phase_tracks(d: MidiDevice):
    log('=== PHASE tracks ===')
    rows = classify_tracks(d)
    path = os.path.join(OUT, 'track_classification.json')
    json.dump(rows, open(path, 'w'), indent=2)
    log(f'wrote {path}')
    for r in rows:
        log(f"  t{r.get('track')}: type={r.get('track_type')} vt={r.get('step_vt')} "
            f"kind={r.get('vt_kind')} len={r.get('length')} err={r.get('error')}")

    # Native UI: select sample track 0 vs non-sample track 8; capture knob page titles
    ui_obs = {}
    for t, label in [(0, 'sample_t0'), (8, 'nonsample_t8')]:
        d.select_track(t)
        d.clear_log()
        # Touch Note knob (1) and Volume knob (6)
        d.s.touch(1, after_ms=200)
        note_txt = d.page_texts(150)
        d.s.touch(6, after_ms=200)
        vol_txt = d.page_texts(150)
        d.s.touch(2, after_ms=200)  # Sample / Folder
        samp_txt = d.page_texts(150)
        ui_obs[label] = {
            'note_page_sample': [x for x in note_txt if x][-20:],
            'vol_page_sample': [x for x in vol_txt if x][-20:],
            'sample_page_sample': [x for x in samp_txt if x][-20:],
        }
        log(f'UI {label} note snippets: {ui_obs[label]["note_page_sample"][-8:]}')
        log(f'UI {label} vol snippets: {ui_obs[label]["vol_page_sample"][-8:]}')
        log(f'UI {label} sample snippets: {ui_obs[label]["sample_page_sample"][-8:]}')
    path = os.path.join(OUT, 'track_ui_pages.json')
    json.dump(ui_obs, open(path, 'w'), indent=2)
    return {'tracks': rows, 'ui': ui_obs}


def ensure_empty_step(d: MidiDevice, t, i):
    if d.is_on(t, i):
        d.toggle(t, i)
        d.s.run_ms(50)


def ensure_on_step(d: MidiDevice, t, i):
    if not d.is_on(t, i):
        d.toggle(t, i)
        d.s.run_ms(50)


def phase_params(d: MidiDevice, track=8):
    """Controlled differential: change one native UI control; observe getParam deltas."""
    log(f'=== PHASE params (track {track}) ===')
    results = []
    d.select_track(track)
    # Use disposable step 3
    step = 3
    ensure_empty_step(d, track, step)
    ensure_on_step(d, track, step)
    d.s.run_ms(100)
    base = d.dump_params(track, step)
    base_bytes = d.step_bytes(track, step).hex()
    log(f'baseline params: { {k:v for k,v in base.items() if v is not None} }')
    results.append({'event': 'baseline', 'params': base, 'bytes': base_bytes, 'on': bool(d.is_on(track, step))})

    # Experiment 1: Note knob (knob 1) — turn +8 clicks worth
    d.s.pad(track % 8, step % 16, hold_ms=40, after_ms=80)  # focus step
    d.s.touch(1, after_ms=150)
    before = d.dump_params(track, step)
    d.s.knob(1, 8, after_ms=250)
    after = d.dump_params(track, step)
    results.append({
        'event': 'note_knob_plus8',
        'diff': diff_dicts(before, after),
        'after': {k: v for k, v in after.items() if v is not None},
        'ui': d.page_texts(80)[-15:],
    })
    log(f'note_knob_plus8 diff: {results[-1]["diff"]}')

    # Experiment 2: independent note value via another delta
    before = d.dump_params(track, step)
    d.s.knob(1, -3, after_ms=250)
    after = d.dump_params(track, step)
    results.append({
        'event': 'note_knob_minus3',
        'diff': diff_dicts(before, after),
        'after': {k: v for k, v in after.items() if v is not None},
    })
    log(f'note_knob_minus3 diff: {results[-1]["diff"]}')

    # Experiment 3: Volume knob (6) — Velocity on MIDI
    d.s.touch(6, after_ms=150)
    before = d.dump_params(track, step)
    d.s.knob(6, 10, after_ms=250)
    after = d.dump_params(track, step)
    results.append({
        'event': 'volume_knob_plus10',
        'diff': diff_dicts(before, after),
        'after': {k: v for k, v in after.items() if v is not None},
        'ui': d.page_texts(80)[-15:],
    })
    log(f'volume_knob_plus10 diff: {results[-1]["diff"]}')

    # Experiment 4: Pan / Note Length (knob 6 bottom / shift?) — touch then shift+knob or second param
    # Stock: Volume/Pan share knob 6; second param via bottom soft key or shift
    d.s.tap(BTN_BOT, after_ms=150)  # often selects second param on page
    before = d.dump_params(track, step)
    d.s.knob(6, 6, after_ms=250)
    after = d.dump_params(track, step)
    results.append({
        'event': 'volpage_second_param_plus6',
        'diff': diff_dicts(before, after),
        'after': {k: v for k, v in after.items() if v is not None},
        'ui': d.page_texts(80)[-15:],
    })
    log(f'volpage_second_param_plus6 diff: {results[-1]["diff"]}')

    # Experiment 5: restore by emptying step
    ensure_empty_step(d, track, step)
    results.append({
        'event': 'cleared',
        'on': bool(d.is_on(track, step)),
        'params': {k: v for k, v in d.dump_params(track, step).items() if v is not None},
    })

    # Cross-check sample track 0 with same note knob (should hit param 6)
    ensure_on_step(d, 0, 5)
    d.select_track(0)
    d.s.pad(0, 5, hold_ms=40, after_ms=80)
    d.s.touch(1, after_ms=150)
    before = d.dump_params(0, 5)
    d.s.knob(1, 5, after_ms=250)
    after = d.dump_params(0, 5)
    results.append({
        'event': 'sample_t0_note_knob_plus5',
        'diff': diff_dicts(before, after),
        'after': {k: v for k, v in after.items() if v is not None},
    })
    log(f'sample note diff: {results[-1]["diff"]}')
    ensure_empty_step(d, 0, 5)

    path = os.path.join(OUT, f'param_diff_t{track}.json')
    json.dump(results, open(path, 'w'), indent=2)
    log(f'wrote {path}')
    return results


def phase_mono(d: MidiDevice, track=8, note_idx=None, vel_idx=None):
    """Monophonic lifecycle using verified indices only."""
    log('=== PHASE mono ===')
    report = {'track': track, 'note_idx': note_idx, 'vel_idx': vel_idx, 'tests': []}
    if note_idx is None:
        report['blocked'] = 'note_idx unknown — run params phase first'
        log(report['blocked'])
        return report

    def read_note(step):
        return d.param(track, step, note_idx)

    def write_note(step, n):
        d.set_param(track, step, note_idx, n)

    def write_vel(step, v):
        if vel_idx is not None:
            d.set_param(track, step, vel_idx, v)

    # Test A
    step = 2
    ensure_empty_step(d, track, step)
    ensure_on_step(d, track, step)
    write_note(step, 60)
    d.s.run_ms(30)
    a1 = read_note(step)
    write_note(step, 67)
    d.s.run_ms(30)
    a2 = read_note(step)
    ensure_empty_step(d, track, step)
    a3 = (bool(d.is_on(track, step)), read_note(step))
    report['tests'].append({
        'name': 'A_lifecycle',
        'insert_60': a1, 'modify_67': a2, 'after_remove': a3,
        'pass': a1 == 60 and a2 == 67 and a3[0] is False,
    })
    log(f"Test A: {report['tests'][-1]}")

    # Test B — 16-step fixture
    seq = [(0, 60, 100), (4, 64, 80), (8, 67, 110), (12, 72, 90)]
    # clear 0..15
    for i in range(16):
        ensure_empty_step(d, track, i)
    # neighbor track untouched marker
    neighbor = 0 if track != 0 else 1
    neigh_before = [d.is_on(neighbor, i) for i in range(16)]
    for i, n, v in seq:
        ensure_on_step(d, track, i)
        write_note(i, n)
        if vel_idx is not None:
            write_vel(i, v)
    d.s.run_ms(50)
    readback = []
    for i, n, v in seq:
        got_n = read_note(i)
        got_v = d.param(track, i, vel_idx) if vel_idx is not None else None
        readback.append({'step': i, 'note': got_n, 'vel': got_v, 'expect_n': n, 'expect_v': v})
    empty_others = [i for i in range(16) if i not in {0, 4, 8, 12} and d.is_on(track, i)]
    neigh_after = [d.is_on(neighbor, i) for i in range(16)]
    ok = all(r['note'] == r['expect_n'] for r in readback)
    if vel_idx is not None:
        ok = ok and all(r['vel'] == r['expect_v'] for r in readback)
    ok = ok and not empty_others and neigh_before == neigh_after
    report['tests'].append({
        'name': 'B_four_note',
        'readback': readback,
        'extra_ons': empty_others,
        'neighbor_preserved': neigh_before == neigh_after,
        'pass': ok,
    })
    log(f"Test B pass={ok} readback={readback}")

    # cleanup B
    for i, _, _ in seq:
        ensure_empty_step(d, track, i)

    # Test C boundaries
    bounds = {}
    # first/last step in length
    ln = min(d.track_len(track), 64)
    for i in (0, ln - 1):
        ensure_empty_step(d, track, i)
        ensure_on_step(d, track, i)
        write_note(i, 60)
        bounds[f'step_{i}'] = read_note(i) == 60
        ensure_empty_step(d, track, i)
    for n in (0, 127):
        ensure_on_step(d, track, 1)
        write_note(1, n)
        bounds[f'note_{n}'] = read_note(1) == n
    ensure_empty_step(d, track, 1)
    if vel_idx is not None:
        ensure_on_step(d, track, 1)
        for v in (1, 127):
            write_vel(1, v)
            bounds[f'vel_{v}'] = d.param(track, 1, vel_idx) == v
        ensure_empty_step(d, track, 1)
    # invalid track rejection (host-side): sample track must not be treated as MIDI
    bounds['sample_track_type0'] = d.track_type(0) == 0
    bounds['midi_candidate_type1'] = d.track_type(track) == 1
    report['tests'].append({'name': 'C_bounds', 'bounds': bounds, 'pass': all(bounds.values())})
    log(f"Test C: {bounds}")

    path = os.path.join(OUT, 'mono_validation.json')
    json.dump(report, open(path, 'w'), indent=2)
    return report


def phase_poly(d: MidiDevice, track=8, note_idx=None):
    log('=== PHASE poly ===')
    report = {'supported': False, 'evidence': []}
    if note_idx is None:
        report['blocked'] = 'note_idx unknown'
        return report
    step = 6
    ensure_empty_step(d, track, step)
    ensure_on_step(d, track, step)
    d.set_param(track, step, note_idx, 60)
    before = d.step_bytes(track, step).hex()
    # Probe: is there a second note slot via higher param indices or raw bytes?
    # Try setting param indices that might be chord slots (exploratory READ of all params after UI poly if any)
    # Without a native poly UI path, attempt setParam on unused indices is FORBIDDEN as speculative writes.
    # Instead: dump all params + bytes after single note; look for multi-note UI (Hold+Note) if present.
    d.select_track(track)
    d.s.pad(track % 8, step % 16, hold_ms=40, after_ms=80)
    d.s.touch(1, after_ms=150)
    # Hold shift and add notes? Product may use polyphonic pad entry — try second note via knob only changes primary.
    d.s.knob(1, 4, after_ms=200)
    after = d.dump_params(track, step)
    after_b = d.step_bytes(track, step).hex()
    report['evidence'].append({
        'single_note_params': {k: v for k, v in after.items() if v is not None},
        'bytes_before': before,
        'bytes_after_knob': after_b,
        'note_get': d.param(track, step, note_idx),
    })
    # Conclusion: without a verified multi-note write path, mark unsupported
    report['supported'] = False
    report['limitation'] = (
        'No verified native UI path found in this session to place multiple MIDI notes in one step; '
        'getParam exposes a single note index. Monophonic path retained.'
    )
    ensure_empty_step(d, track, step)
    path = os.path.join(OUT, 'poly_validation.json')
    json.dump(report, open(path, 'w'), indent=2)
    log(f'poly: {report["limitation"]}')
    return report


def phase_undo(d: MidiDevice, track=8, note_idx=None):
    log('=== PHASE undo ===')
    report = {'tests': []}
    if note_idx is None:
        report['blocked'] = 'note_idx unknown'
        return report
    step = 7
    ensure_empty_step(d, track, step)
    # Soft-key Undo after pad toggle (stock behavior) — observe
    ensure_on_step(d, track, step)
    d.set_param(track, step, note_idx, 60)
    on_before = bool(d.is_on(track, step))
    note_before = d.param(track, step, note_idx)
    # Soft key 9 is often Undo on pattern screen
    d.clear_log()
    d.s.tap(BTN_SOFT3, after_ms=300)
    texts = d.page_texts(100)
    on_after = bool(d.is_on(track, step))
    note_after = d.param(track, step, note_idx)
    report['tests'].append({
        'name': 'softkey9_after_edit',
        'on_before': on_before, 'note_before': note_before,
        'on_after': on_after, 'note_after': note_after,
        'ui': texts[-10:],
        'pass': None,  # observational
    })
    # Native undo via calling fw path is not available without ctrl undo ring push first.
    # Characterize: edit_begin + toggle + setParam pattern used by slicer (source-verified).
    report['tests'].append({
        'name': 'source_pattern',
        'mechanism': 'cfw_edit_begin -> fw_step_toggle / setParam -> soft Undo',
        'confidence': 'VERIFIED_SOURCE for sample; MIDI soft-undo observational above',
    })
    ensure_empty_step(d, track, step)
    path = os.path.join(OUT, 'undo_validation.json')
    json.dump(report, open(path, 'w'), indent=2)
    return report


def phase_midi_tx(d: MidiDevice):
    log('=== PHASE midi_tx observability ===')
    from unicorn import arm_const as A
    events = []

    def capture(vp):
        # r0 may be device; log PC/LR and a peek of possible packet buffer if in regs
        r0, r1, r2, r3 = [vp.mu.reg_read(r) for r in (
            A.UC_ARM_REG_R0, A.UC_ARM_REG_R1, A.UC_ARM_REG_R2, A.UC_ARM_REG_R3)]
        events.append({'r0': r0, 'r1': r1, 'r2': r2, 'r3': r3, 'ms': vp.ms})
        # skip send like production stub when no device
        if vp.r32(USB_MIDI_DEV) == 0:
            vp.mu.reg_write(A.UC_ARM_REG_R0, 0)
            vp.mu.reg_write(A.UC_ARM_REG_PC, vp.mu.reg_read(A.UC_ARM_REG_LR))

    # Re-hook (stage already installed stub); overwrite hook
    d.vp.hook_addr(USB_MIDI_TX, capture)
    # Play briefly
    d.s.tap(BTN_PLAY, hold_ms=40, after_ms=500)
    # Ensure a trig exists on track 0 to cause activity
    ensure_on_step(d, 0, 0)
    d.s.tap(BTN_PLAY, hold_ms=40, after_ms=1500)
    d.s.tap(BTN_PLAY, hold_ms=40, after_ms=200)  # stop
    report = {
        'usb_midi_dev': d.vp.r32(USB_MIDI_DEV),
        'tx_events_captured': len(events),
        'events_sample': events[:20],
        'interpretation': (
            'A=sequencer storage may be verified separately; '
            'B=dispatch observability depends on usb_midi_tx being entered; '
            'C=physical MIDI out cannot be established in vdev.'
        ),
    }
    path = os.path.join(OUT, 'midi_tx_obs.json')
    json.dump(report, open(path, 'w'), indent=2)
    log(f"midi_tx events={len(events)} usb_midi_dev={report['usb_midi_dev']}")
    return report


def infer_indices(param_results):
    """Infer note/velocity indices from differential results."""
    note_idx = vel_idx = dur_idx = None
    conf = {}
    for ev in param_results:
        diff = ev.get('diff') or {}
        if ev.get('event', '').startswith('note_knob') and len(diff) == 1:
            k = int(next(iter(diff)))
            note_idx = k
            conf['note'] = 'VERIFIED_VDEV'
        if ev.get('event') == 'volume_knob_plus10' and len(diff) == 1:
            k = int(next(iter(diff)))
            vel_idx = k
            conf['velocity'] = 'VERIFIED_VDEV'
        if ev.get('event') == 'volpage_second_param_plus6' and len(diff) == 1:
            k = int(next(iter(diff)))
            dur_idx = k
            conf['duration'] = 'VERIFIED_VDEV'
    # sample cross-check
    for ev in param_results:
        if ev.get('event') == 'sample_t0_note_knob_plus5':
            diff = ev.get('diff') or {}
            if 6 in {int(k) for k in diff} or (len(diff) == 1 and int(next(iter(diff))) == 6):
                conf['sample_note6'] = 'VERIFIED_VDEV'
    return note_idx, vel_idx, dur_idx, conf


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('ppf')
    ap.add_argument('--snap')
    ap.add_argument('--phase', default='all')
    ap.add_argument('--track', type=int, default=8)
    a = ap.parse_args()
    work = os.environ.setdefault('VPLAY_DIR', '/tmp/fp-g1b')
    os.makedirs(work, exist_ok=True)
    snap = a.snap or os.path.join(work, os.path.basename(a.ppf) + '.boot.snap')
    if not os.path.exists(snap):
        import stage
        log(f'booting to {snap} ...')
        stage.stage(a.ppf, None, snap, insns=BOOT_MS * 100_000, seconds=3600, quiet=True)

    summary = {'phases': {}}
    d = MidiDevice(a.ppf, snap)
    d.s.run_ms(200)

    note_idx = vel_idx = dur_idx = None
    conf = {}

    if a.phase in ('all', 'tracks'):
        summary['phases']['tracks'] = phase_tracks(d)

    if a.phase in ('all', 'params'):
        # Fresh device after heavy UI
        d = MidiDevice(a.ppf, snap)
        d.s.run_ms(200)
        pr = phase_params(d, track=a.track)
        summary['phases']['params'] = pr
        note_idx, vel_idx, dur_idx, conf = infer_indices(pr)
        summary['inferred'] = dict(note_idx=note_idx, vel_idx=vel_idx, dur_idx=dur_idx, confidence=conf)
        log(f'inferred indices: {summary["inferred"]}')
        json.dump(summary['inferred'], open(os.path.join(OUT, 'inferred_indices.json'), 'w'), indent=2)

    if a.phase in ('all', 'mono'):
        if note_idx is None and os.path.exists(os.path.join(OUT, 'inferred_indices.json')):
            inf = json.load(open(os.path.join(OUT, 'inferred_indices.json')))
            note_idx, vel_idx, dur_idx = inf.get('note_idx'), inf.get('vel_idx'), inf.get('dur_idx')
        d = MidiDevice(a.ppf, snap)
        d.s.run_ms(200)
        summary['phases']['mono'] = phase_mono(d, track=a.track, note_idx=note_idx, vel_idx=vel_idx)

    if a.phase in ('all', 'poly'):
        if note_idx is None and os.path.exists(os.path.join(OUT, 'inferred_indices.json')):
            inf = json.load(open(os.path.join(OUT, 'inferred_indices.json')))
            note_idx = inf.get('note_idx')
        d = MidiDevice(a.ppf, snap)
        d.s.run_ms(200)
        summary['phases']['poly'] = phase_poly(d, track=a.track, note_idx=note_idx)

    if a.phase in ('all', 'undo'):
        if note_idx is None and os.path.exists(os.path.join(OUT, 'inferred_indices.json')):
            inf = json.load(open(os.path.join(OUT, 'inferred_indices.json')))
            note_idx = inf.get('note_idx')
        d = MidiDevice(a.ppf, snap)
        d.s.run_ms(200)
        summary['phases']['undo'] = phase_undo(d, track=a.track, note_idx=note_idx)

    if a.phase in ('all', 'midi_tx'):
        d = MidiDevice(a.ppf, snap)
        d.s.run_ms(200)
        summary['phases']['midi_tx'] = phase_midi_tx(d)

    path = os.path.join(OUT, 'probe_summary.json')
    # prune huge nested for summary file
    slim = {k: v for k, v in summary.items() if k != 'phases'}
    slim['phase_keys'] = list(summary['phases'])
    slim['inferred'] = summary.get('inferred')
    if 'mono' in summary['phases']:
        slim['mono'] = summary['phases']['mono']
    if 'poly' in summary['phases']:
        slim['poly'] = summary['phases']['poly']
    if 'undo' in summary['phases']:
        slim['undo'] = summary['phases']['undo']
    if 'midi_tx' in summary['phases']:
        slim['midi_tx'] = summary['phases']['midi_tx']
    json.dump(slim, open(path, 'w'), indent=2)
    log(f'wrote {path}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
