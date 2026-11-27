#!/usr/bin/env python3
"""G6B preflight: chord names, lock bits, and live 16/32/64 lengths.

Uses the frozen 0.2e image. Length changes use the stock Track Length knob
before any Genesis heap allocation. Chord writes use setParam only.
"""
from __future__ import annotations

import json
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
sys.path[:0] = [os.path.join(ROOT, 'cfw', 'vdev'), os.path.join(ROOT, 'cfw', 'tests'),
                os.path.join(ROOT, 'research', 'g1b', 'diagnostics')]
from midi_probe import MidiDevice  # noqa: E402

PPF = os.environ.get('G6B_PPF', '/tmp/fp-g5e/PlayPlus_FreePlay_01.ppf')
SNAP = os.environ.get('VPLAY_SNAP', '/tmp/fp-g5e/PlayPlus_FreePlay_01.ppf.boot.snap')
BANK = 0x600cf578
CHORD, NOTE = 10, 6
TRACK = 8
NAMES = [
    'None', 'Power 4th', 'Power 5th', 'Major', 'Minor', 'Dim', 'Aug',
    'Sus2', 'Sus4', 'Maj 6th', 'Min 6th', 'Maj 7th', 'Min 7th', 'MinMaj7',
    'Dom 7th', 'Maj 9th', 'Min 9th', 'Maj 11th', 'Min 11th', 'Maj 13th', 'Min 13th',
]


def lock_word(raw):
    return raw[0x26] | (raw[0x27] << 8) | (raw[0x28] << 16) | ((raw[0x29] & 0x7f) << 24)


def bit(raw, idx):
    return (lock_word(raw) >> idx) & 1


def boot():
    d = MidiDevice(PPF, SNAP)
    d.vp.call_fn(BANK, d.ctrl)
    d.s.run_ms(40)
    return d


def main():
    out = {'names': [], 'locks': [], 'lengths': []}
    d = boot()
    if not d.is_on(TRACK, 4):
        d.toggle(TRACK, 4)
    d.s.run_ms(20)
    raw = d.step_bytes(TRACK, 4)
    out['locks'].append({
        'when': 'fresh toggle',
        'get': d.param(TRACK, 4, CHORD),
        'bit10': bit(raw, CHORD),
        'word': f'{lock_word(raw):08x}',
    })
    d.vp.inputs.pad(0, 4, 0)
    d.s.run_ms(60)
    d.s.touch(7, after_ms=80)
    seen = []
    for expect in NAMES:
        # value 0 is already showing; subsequent names need a detent
        if expect != 'None':
            d.s.knob(7, 1, after_ms=70)
        val = d.param(TRACK, 4, CHORD)
        raw = d.step_bytes(TRACK, 4)
        seen.append({'id': val, 'expect': expect, 'bit10': bit(raw, CHORD)})
    out['names'] = seen
    extra = d.param(TRACK, 4, CHORD)
    d.s.knob(7, 1, after_ms=70)
    out['past_end'] = {'before': extra, 'after_extra_detent': d.param(TRACK, 4, CHORD)}

    # explicit None versus unlocked
    d.set_param(TRACK, 4, CHORD, 3)
    raw = d.step_bytes(TRACK, 4)
    locked_major = {'get': d.param(TRACK, 4, CHORD), 'bit10': bit(raw, CHORD)}
    d.set_param(TRACK, 4, CHORD, 0)
    raw = d.step_bytes(TRACK, 4)
    locked_none = {'get': d.param(TRACK, 4, CHORD), 'bit10': bit(raw, CHORD)}
    d.toggle(TRACK, 4)  # off
    raw = d.step_bytes(TRACK, 4)
    off = {'on': d.is_on(TRACK, 4), 'get': d.param(TRACK, 4, CHORD), 'bit10': bit(raw, CHORD)}
    d.toggle(TRACK, 4)  # on again
    raw = d.step_bytes(TRACK, 4)
    retoggle = {'on': d.is_on(TRACK, 4), 'get': d.param(TRACK, 4, CHORD), 'bit10': bit(raw, CHORD),
                'note': d.param(TRACK, 4, NOTE)}
    out['locks'].extend([
        {'when': 'set 3', **locked_major},
        {'when': 'set 0', **locked_none},
        {'when': 'toggled off', **off},
        {'when': 'toggled on again', **retoggle},
    ])

    # click and shift-click while the pad is held, looking for an unlock
    d.set_param(TRACK, 4, CHORD, 12)
    d.vp.inputs.pad(0, 4, 0)
    d.s.run_ms(40)
    d.s.touch(7, after_ms=40)
    d.s.tap(6, after_ms=80)  # click
    raw = d.step_bytes(TRACK, 4)
    out['click'] = {'get': d.param(TRACK, 4, CHORD), 'bit10': bit(raw, CHORD)}
    d.vp.inputs.button(10, 0)
    d.s.run_ms(30)
    d.s.tap(6, after_ms=80)
    d.vp.inputs.button(10, 1)
    d.s.run_ms(30)
    raw = d.step_bytes(TRACK, 4)
    out['shift_click'] = {'get': d.param(TRACK, 4, CHORD), 'bit10': bit(raw, CHORD)}

    # live lengths. Fresh boots so the Track Length knob still moves.
    for target, steps in ((32, (0, 15, 16, 31)), (64, (0, 15, 16, 31, 32, 47, 48, 63))):
        d = boot()
        # boot() already called the stock bank toggle. Select the MIDI track, then knob 10.
        d.s.pad(0, 19, hold_ms=30, after_ms=50)
        d.s.touch(10, hold_ms=25, after_ms=30)
        cur = d.track_len(TRACK)
        guard = 0
        while cur != target and guard < 80:
            d.s.knob(10, 1 if cur < target else -1, after_ms=35)
            cur = d.track_len(TRACK)
            guard += 1
        row = {'target': target, 'length': cur, 'steps': []}
        for s in steps:
            if not d.is_on(TRACK, s):
                d.toggle(TRACK, s)
            d.set_param(TRACK, s, CHORD, 3)
            d.set_param(TRACK, s, NOTE, 60)
            row['steps'].append({
                'step': s,
                'chord': d.param(TRACK, s, CHORD),
                'note': d.param(TRACK, s, NOTE),
                'on': d.is_on(TRACK, s),
            })
        # one past the live length must not be treated as in-pattern
        past = target
        row['past'] = {'step': past, 'length': cur, 'in_range': past < cur}
        out['lengths'].append(row)

    # length stays 16: step 16 is backing storage, not a live step
    d = boot()
    out['len16'] = {
        'length': d.track_len(TRACK),
        'step16_on': d.is_on(TRACK, 16),
        'step16_in_range': 16 < d.track_len(TRACK),
    }

    path = os.path.join(os.path.dirname(__file__), '..', 'fixtures')
    os.makedirs(path, exist_ok=True)
    dest = os.path.join(path, 'chord_preflight.json')
    json.dump(out, open(dest, 'w'), indent=2)
    print(json.dumps(out, indent=2))


if __name__ == '__main__':
    main()
