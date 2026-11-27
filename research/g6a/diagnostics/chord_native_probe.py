#!/usr/bin/env python3
"""G6A diagnostic: native Chord parameter on an external MIDI step.

Uses the stock UI and the stock getParam/setParam accessors on the frozen
0.2e image. Does not link new firmware and does not write a production Apply path.

The grid bank is switched with the stock bank-toggle function at 0x600cf578
so the Note page is the MIDI page (Pitchbend), not the sample page (Microtune).
"""
from __future__ import annotations

import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'cfw', 'vdev'))
sys.path.insert(0, os.path.join(ROOT, 'cfw', 'tests'))
sys.path.insert(0, os.path.join(ROOT, 'research', 'g1b', 'diagnostics'))

import hle  # noqa: E402
from midi_probe import MidiDevice  # noqa: E402
from unicorn.arm_const import UC_ARM_REG_LR, UC_ARM_REG_R1  # noqa: E402

PPF = os.environ.get('G6A_PPF', '/tmp/fp-g5e/PlayPlus_FreePlay_01.ppf')
SNAP = os.environ.get('VPLAY_SNAP', '/tmp/fp-g5e/PlayPlus_FreePlay_01.ppf.boot.snap')
BANK_TOGGLE = 0x600cf578
GETPARAM = 0x6010acfc
CHORD = 10
NOTE, VEL, DUR = 6, 0, 1
TRACK, STEP = 8, 4
EXPECTED = [
    'None', 'Power 4th', 'Power 5th', 'Major', 'Minor', 'Dim', 'Aug',
    'Sus2', 'Sus4', 'Maj 6th', 'Min 6th', 'Maj 7th', 'Min 7th', 'MinMaj7',
    'Dom 7th', 'Maj 9th', 'Min 9th', 'Maj 11th', 'Min 11th', 'Maj 13th', 'Min 13th',
]

results = []


def check(name, ok, detail=''):
    results.append({'name': name, 'ok': bool(ok), 'detail': detail})
    print(('PASS' if ok else 'FAIL'), name, detail if not ok else '', flush=True)


def label_after(d, n0):
    ts = []
    for n, a in d.vp.eve.cmd_log[n0:]:
        if n == 'TEXT' and len(a) > 2 and a[2]:
            ts.append(a[2])
    for j, t in enumerate(ts):
        if t == 'Chord' and j:
            return ts[j - 1]
    return None


def main():
    d = MidiDevice(PPF, SNAP)
    mode = d.vp.r32(d.ctrl + 0x24b4)
    bank0 = d.vp.mu.mem_read(mode + 0x112, 1)[0]
    d.vp.call_fn(BANK_TOGGLE, d.ctrl)
    d.s.run_ms(40)
    bank1 = d.vp.mu.mem_read(mode + 0x112, 1)[0]
    check('bank toggle reaches MIDI tracks', bank0 == 0 and bank1 == 1, f'{bank0}->{bank1}')

    if not d.is_on(TRACK, STEP):
        d.toggle(TRACK, STEP)
    d.s.run_ms(30)
    d.vp.inputs.pad(0, STEP, 0)
    d.s.run_ms(80)
    n0 = len(d.vp.eve.cmd_log)
    d.s.touch(1, after_ms=120)
    note_page = label_after(d, n0)  # not a chord page
    texts = [a[2] for n, a in d.vp.eve.cmd_log[n0:] if n == 'TEXT' and len(a) > 2]
    check('MIDI note page shows Pitchbend', 'Pitchbend' in texts and 'Microtune' not in texts, str(texts[-8:]))

    n0 = len(d.vp.eve.cmd_log)
    d.s.touch(7, after_ms=100)
    # value 0 is already showing; the touch draws "None" / "Chord"
    opened = [a[2] for n, a in d.vp.eve.cmd_log[n0:] if n == 'TEXT' and len(a) > 2]
    check('knob 7 opens Chord and the fresh value is None',
          'Chord' in opened and 'None' in opened and d.param(TRACK, STEP, CHORD) == 0,
          str(opened[-8:]))
    shots = os.path.join(HERE, '..', 'screenshots')
    os.makedirs(shots, exist_ok=True)
    d.s.shot(os.path.join(shots, 'chord_none.png'))

    seen = []
    ok_labels = True
    want_shot = {'Major': 'chord_major.png', 'Min 7th': 'chord_min7.png', 'Min 13th': 'chord_min13.png'}
    for expect in EXPECTED[1:]:
        n0 = len(d.vp.eve.cmd_log)
        d.s.knob(7, 1, after_ms=90)
        got = label_after(d, n0)
        val = d.param(TRACK, STEP, CHORD)
        seen.append((val, got))
        if got != expect:
            ok_labels = False
        if got in want_shot:
            d.s.shot(os.path.join(shots, want_shot[got]))
    check('knob walks the 21 stock chord names in order', ok_labels and seen[-1] == (20, 'Min 13th'), str(seen))
    n0 = len(d.vp.eve.cmd_log)
    d.s.knob(7, 1, after_ms=90)
    check('knob stops at Min 13th', d.param(TRACK, STEP, CHORD) == 20 and label_after(d, n0) == 'Min 13th')

    note, vel, dur = d.param(TRACK, STEP, NOTE), d.param(TRACK, STEP, VEL), d.param(TRACK, STEP, DUR)
    check('chord edits do not change note, velocity, or duration',
          (note, vel, dur) == (60, 100, 60), f'{note, vel, dur}')
    raw = d.step_bytes(TRACK, STEP)
    check('value 20 is the halfword at step+18', raw[18] == 20 and raw[19] == 0, raw[16:22].hex())

    # direct accessor clamps
    st = d.step(TRACK, STEP)
    def setp(track, step, idx, val):
        obj = d.step(track, step)
        d.vp.call_fn(d.vfn(obj, 0x10), obj, idx, val & 0xFFFFFFFF)

    setp(TRACK, STEP, CHORD, 3)
    check('setParam 3 reads back Major', d.param(TRACK, STEP, CHORD) == 3 and d.step_bytes(TRACK, STEP)[18:20] == b'\x03\x00')
    setp(TRACK, STEP, CHORD, 21)
    check('setParam 21 clamps to 20', d.param(TRACK, STEP, CHORD) == 20)
    setp(TRACK, STEP, CHORD, 255)
    check('setParam 255 clamps to 0', d.param(TRACK, STEP, CHORD) == 0)
    setp(TRACK, STEP, CHORD, 0xFFFFFFFF)
    check('setParam -1 clamps to 0', d.param(TRACK, STEP, CHORD) == 0)

    # neighbor and other track
    check('step 5 was not written', d.param(TRACK, 5, CHORD) is None and not d.is_on(TRACK, 5))
    sample_before = d.param(0, STEP, CHORD)
    setp(TRACK, STEP, CHORD, 4)  # Minor
    check('sample-track param 10 is unchanged by a MIDI chord write',
          d.param(0, STEP, CHORD) == sample_before, f'{sample_before}')
    check('note and velocity still shared', d.param(TRACK, STEP, NOTE) == 60 and d.param(TRACK, STEP, VEL) == 100)

    # restore
    setp(TRACK, STEP, CHORD, 0)
    check('restore writes None', d.param(TRACK, STEP, CHORD) == 0)

    # length: objects exist past the live 16-step pattern; the adapter must still reject them
    plen = d.track_len(TRACK)
    check('live MIDI pattern length is 16', plen == 16, str(plen))
    far = {}
    for s in (0, 15, 16, 31, 32, 63):
        try:
            setp(TRACK, s, CHORD, 3)
            far[s] = d.param(TRACK, s, CHORD)
            setp(TRACK, s, CHORD, 0)
        except Exception as e:
            far[s] = f'err {e}'
    check('chord field is stored on steps 0, 15, 16, 31, 32 and 63',
          all(far.get(s) == 3 for s in (0, 15, 16, 31, 32, 63)), str(far))

    # sample track knob 7 is not the Chord page
    d.vp.call_fn(BANK_TOGGLE, d.ctrl)  # back to tracks 1-8
    d.s.run_ms(40)
    d.s.pad(0, STEP, hold_ms=30, after_ms=80)
    n0 = len(d.vp.eve.cmd_log)
    d.s.touch(7, after_ms=120)
    sample_txt = [a[2] for n, a in d.vp.eve.cmd_log[n0:] if n == 'TEXT' and len(a) > 2]
    check('sample-track knob 7 is not Chord', 'Chord' not in sample_txt, str(sample_txt[-8:]))

    # Audio clock drives the sequencer. The parameter-apply function is 0x6010afd4
    # (r0 = step, r1 = index). Index 10 is included in the generic 0..21 flush.
    d.vp.call_fn(BANK_TOGGLE, d.ctrl)
    d.s.run_ms(40)
    if not d.is_on(TRACK, 0):
        d.toggle(TRACK, 0)
    setp(TRACK, 0, CHORD, 3)
    hle.enable_audio(d.vp)
    callers = {}
    tx = {'n': 0}

    def on_apply(vp):
        idx = vp.mu.reg_read(UC_ARM_REG_R1) & 0xFF
        if idx != CHORD:
            return
        lr = vp.mu.reg_read(UC_ARM_REG_LR) & ~1
        callers[lr] = callers.get(lr, 0) + 1

    d.vp.hook_addr(0x6010afd4, on_apply)
    d.vp.hook_addr(0x600c6688, lambda vp: tx.__setitem__('n', tx['n'] + 1))
    seq = d.vp.r32(d.vp.profile['app'] + 0x10)
    d.s.tap(8, hold_ms=40, after_ms=400)
    playing = d.vp.call_fn(d.vp.r32(d.vp.r32(seq) + 0x28), seq) & 0xFF
    check('transport is running and the parameter flush reads chord index 10',
          playing == 1 and callers.get(0x6010b4f2, 0) > 0, f'playing={playing} callers={callers}')
    check('virtual USB MIDI transmit was not entered while playing', tx['n'] == 0, str(tx))
    print('apply callers', {hex(k): v for k, v in callers.items()}, 'tx', tx['n'])

    out = os.path.join(HERE, '..', 'fixtures')
    os.makedirs(out, exist_ok=True)
    path = os.path.join(out, 'chord_native_probe.json')
    json.dump({'results': results,
               'chord_apply_callers': {f'0x{a:08x}': n for a, n in callers.items()},
               'usb_midi_tx_while_playing': tx['n'], 'labels': EXPECTED},
              open(path, 'w'), indent=2)
    failed = [r for r in results if not r['ok']]
    print(f'{len(results) - len(failed)}/{len(results)} passed')
    return 0 if not failed else 1


if __name__ == '__main__':
    sys.exit(main())
