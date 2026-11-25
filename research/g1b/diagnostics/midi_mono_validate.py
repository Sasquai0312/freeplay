#!/usr/bin/env python3
"""G1B monophonic MIDI validation harness (verified indices only).

Indices (Play+ 1.3.0, external MIDI tracks, VERIFIED_VDEV):
  6 = MIDI note (0..127)
  0 = Velocity (0..127, write-clamped)
  1 = Note Length (0..3840 units; display steps ≈ value/60; max 64.00)

usage: VPLAY_DIR=/tmp/fp-g1b python midi_mono_validate.py /tmp/fp-g1b/img.ppf
"""
from __future__ import annotations

import json
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
sys.path[:0] = [os.path.join(ROOT, 'cfw', 'vdev'), os.path.join(ROOT, 'cfw', 'tests')]
from vdev_slice import Device  # noqa: E402

FW_STEP_TOGGLE = 0x600de6f1
NOTE, VEL, DUR = 6, 0, 1
MIDI_TRACK = 8


def main():
    ppf = sys.argv[1]
    snap = os.environ.get('VPLAY_SNAP') or os.path.join(
        os.environ.get('VPLAY_DIR', '/tmp/fp-g1b'), os.path.basename(ppf) + '.boot.snap')
    d = Device(ppf, snap)
    d.s.run_ms(150)
    vp = d.vp
    results = []

    def check(name, ok, detail=None):
        results.append({'name': name, 'pass': bool(ok), 'detail': detail})
        print(('PASS' if ok else 'FAIL'), name, detail or '')

    def toggle(t, i):
        vp.call_fn(FW_STEP_TOGGLE, d.sel, t, i)
        d.s.run_ms(20)

    def setp(t, i, idx, v):
        st = d.step(t, i)
        vp.call_fn(d.vfn(st, 0x10), st, idx, int(v) & 0xFFFFFFFF)

    def getp(t, i, idx):
        return d.param(t, i, idx)

    def on(t, i):
        return bool(d.is_on(t, i))

    def track_type(t):
        st = d.step(t, 0)
        return vp.call_fn(d.vfn(st, 0x40), st) & 0xFF

    def ensure_off(t, i):
        if on(t, i):
            toggle(t, i)

    def ensure_on(t, i):
        if not on(t, i):
            toggle(t, i)

    # Discrimination
    seq = vp.r32(vp.profile['app'] + 0x10)
    buf = vp.call_fn(vp.r32(vp.r32(seq) + 8), seq)
    kinds = []
    for t in range(16):
        inst = vp.r32(buf + 0x98 + t * 0xbb4)
        tt = track_type(t)
        kind = 'sample' if tt == 0 else ('external_midi' if inst == 0 else 'assigned_non_sample')
        kinds.append(kind)
    check('tracks 0-7 sample', kinds[:8] == ['sample'] * 8, kinds[:8])
    check('tracks 8-15 external_midi by default', all(k == 'external_midi' for k in kinds[8:]), kinds[8:])
    check('sample not classified as MIDI', track_type(0) == 0 and track_type(8) == 1)

    t = MIDI_TRACK
    # Test A
    ensure_off(t, 2)
    ensure_on(t, 2)
    setp(t, 2, NOTE, 60)
    setp(t, 2, VEL, 100)
    a1 = (getp(t, 2, NOTE), getp(t, 2, VEL))
    setp(t, 2, NOTE, 67)
    a2 = getp(t, 2, NOTE)
    ensure_off(t, 2)
    check('A lifecycle', a1 == (60, 100) and a2 == 67 and not on(t, 2), {'a1': a1, 'a2': a2})

    # Test B
    seqn = [(0, 60, 100), (4, 64, 80), (8, 67, 110), (12, 72, 90)]
    for i in range(16):
        ensure_off(t, i)
    nb = [on(0, i) for i in range(16)]
    for i, n, v in seqn:
        ensure_on(t, i)
        setp(t, i, NOTE, n)
        setp(t, i, VEL, v)
    rb = [(getp(t, i, NOTE), getp(t, i, VEL)) for i, n, v in seqn]
    exp = [(n, v) for i, n, v in seqn]
    extra = [i for i in range(16) if i not in (0, 4, 8, 12) and on(t, i)]
    na = [on(0, i) for i in range(16)]
    check('B four-note', rb == exp and not extra and nb == na, {'rb': rb, 'extra': extra})

    for i, _, _ in seqn:
        ensure_off(t, i)

    # Test C
    okc = True
    details = {}
    for i in (0, 15):
        ensure_off(t, i)
        ensure_on(t, i)
        setp(t, i, NOTE, 60)
        details[f'step{i}'] = getp(t, i, NOTE) == 60
        okc &= details[f'step{i}']
        ensure_off(t, i)
    ensure_on(t, 1)
    for n in (0, 127):
        setp(t, 1, NOTE, n)
        details[f'note{n}'] = getp(t, 1, NOTE) == n
        okc &= details[f'note{n}']
    for v in (1, 127):
        setp(t, 1, VEL, v)
        details[f'vel{v}'] = getp(t, 1, VEL) == v
        okc &= details[f'vel{v}']
    for L in (15, 3840):
        setp(t, 1, DUR, L)
        details[f'dur{L}'] = getp(t, 1, DUR) == L
        okc &= details[f'dur{L}']
    # invalid velocity rejected by clamp
    setp(t, 1, VEL, 5000)
    details['vel_clamp'] = getp(t, 1, VEL) == 127
    okc &= details['vel_clamp']
    ensure_off(t, 1)
    check('C bounds', okc, details)

    path = os.path.join(os.path.dirname(__file__), '..', 'fixtures', 'mono_harness_results.json')
    json.dump({'results': results, 'passed': all(r['pass'] for r in results)}, open(path, 'w'), indent=2)
    print(f"{sum(r['pass'] for r in results)}/{len(results)} passed; wrote {path}")
    return 0 if all(r['pass'] for r in results) else 1


if __name__ == '__main__':
    sys.exit(main())
