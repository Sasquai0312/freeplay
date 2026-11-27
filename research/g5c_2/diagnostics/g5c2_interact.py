#!/usr/bin/env python3
"""G5C.2 Evolution Lab interaction on the virtual Play+.

Boots nothing itself. Point VPLAY_SNAP at a snapshot of THIS image.

  VPLAY_SNAP=/tmp/fp-g5c2/PlayPlus_FreePlay_01.ppf.boot.snap \\
    .venv/bin/python research/g5c_2/diagnostics/g5c2_interact.py \\
    /tmp/fp-g5c2/PlayPlus_FreePlay_01.ppf
"""
from __future__ import annotations

import json
import os
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
sys.path[:0] = [os.path.join(ROOT, 'cfw', 'vdev'), os.path.join(ROOT, 'cfw', 'tests'),
                os.path.join(ROOT, 'cfw', 'tools')]
from vdev_slice import Device  # noqa: E402
import flows  # noqa: E402

BTN_BACK, BTN_TOP, BTN_BOT, BTN_CLICK, BTN_SHIFT = 7, 0, 3, 6, 10
MIDI = 8
NOTE = 6
MENU_GENESIS = 4
FW_STEP_TOGGLE = 0x600DE6F1
GN_MAGIC = 0x47454E31
WMODE_GENESIS = 7

OFF_ST, OFF_LAB, OFF_PARAM, OFF_TRACK, OFF_LEN = 4, 5, 6, 7, 8
OFF_RECOVERY, OFF_NEV, OFF_APPLIED, OFF_DIAG = 28, 29, 30, 32
OFF_PAGE, OFF_NATIVE = 33, 34
OFF_EVO_MODE, OFF_REG, OFF_LOCK, OFF_STYLE = 2000, 2001, 2003, 2004
OFF_MDISP, OFF_MREV, OFF_MROT = 2007, 2012, 2013
OFF_MPITCH, OFF_MVEL, OFF_MGATE = 2006, 2008, 2009
OFF_MINS, OFF_MREM, OFF_MPROT, OFF_MSTEP = 2010, 2011, 2017, 2018
OFF_PSTEP, OFF_EVO = 2019, 2028
OFF_PHASE, OFF_PENDING, OFF_READY = 3119, 3121, 3122
OFF_EVENTS = 464


def load_syms(path):
    syms = {}
    for line in open(path):
        parts = line.strip().split()
        if len(parts) == 2 and parts[0].startswith('0x'):
            syms[parts[1]] = int(parts[0], 16)
    return syms


class H:
    def __init__(self, ppf, snap):
        self.d = Device(ppf, snap)
        self.d.s.run_ms(150)
        self.vp = self.d.vp
        self.base = 0
        self.bank = False

    def mark(self):
        return len(self.vp.eve.cmd_log)

    def since(self, m):
        out = []
        for n, a in self.vp.eve.cmd_log[m:]:
            if n == 'TEXT' and len(a) > 2:
                out.append(a[2])
        return out

    def texts(self):
        return self.since(0)

    def shot(self, path):
        self.d.s.run_ms(80)
        self.d.s.shot(path, scale=2)

    def ui(self):
        magic = struct.pack('<I', GN_MAGIC)
        hits = []
        for base, size in ((0x20000000, 0x80000), (0x20200000, 0x80000)):
            try:
                blob = self.vp.mu.mem_read(base, size)
            except Exception:
                continue
            start = 0
            while True:
                i = blob.find(magic, start)
                if i < 0:
                    break
                addr = base + i
                b = self.vp.mu.mem_read(addr, 16)
                if b[4] <= 3 and b[5] < 7 and b[6] < 46 and b[7] < 16 and 1 <= b[8] <= 64:
                    hits.append(addr)
                start = i + 4
        self.base = hits[0] if hits else 0
        return self.base

    def rb(self, off, n):
        return bytes(self.vp.mu.mem_read(self.base + off, n))

    def u8(self, off):
        return self.rb(off, 1)[0]

    def poke(self, off, data):
        self.vp.mu.mem_write(self.base + off, data)

    def wmode(self):
        cfw = self.d.cfw_ram
        if self.vp.r32(cfw + 144) != 0x56494557:
            return 0
        return self.vp.mu.mem_read(cfw + 144 + 41, 1)[0]

    def use_midi(self):
        """Select column, row 0. With bank on this is MIDI track 8."""
        self.d.s.pad(0, 19, hold_ms=30, after_ms=80)

    def open_genesis(self):
        self.vp.w32(self.d.sel + 0x0C, 1 << MIDI)
        flows.open_menu(self.d.s)
        self.d.s.run_ms(180)
        m = self.mark()
        flows.enter_item(self.d.s, MENU_GENESIS, after_ms=350)
        self.d.s.run_ms(250)
        self.ui()
        return self.since(m)

    def tap(self, btn, ms=250):
        m = self.mark()
        self.d.s.tap(btn, after_ms=ms)
        return self.since(m)

    def shift_turns(self, n):
        self.vp.inputs.button(BTN_SHIFT, 0)
        self.d.s.run_ms(20)
        for _ in range(abs(n)):
            self.d.s.turn(1 if n > 0 else -1, after_ms=40)
        self.vp.inputs.button(BTN_SHIFT, 1)
        self.d.s.run_ms(30)

    def turns(self, n):
        m = self.mark()
        for _ in range(abs(n)):
            self.d.s.turn(1 if n > 0 else -1, after_ms=40)
        return self.since(m)

    def focus(self, label, limit=48):
        for _ in range(limit):
            m = self.mark()
            self.shift_turns(1)
            shown = self.since(m)
            if label in shown:
                return True
        return False

    def goto_lab(self, lab, limit=8):
        """Top cycles Melody, Euclid, Arp, Turing, Markov, Mutate, Chords."""
        for _ in range(limit):
            if self.u8(OFF_LAB) == lab:
                return True
            self.tap(BTN_TOP, 160)
        return self.u8(OFF_LAB) == lab

    def arm(self):
        """Bank + track select + length knob, before Genesis opens. Button 2 is swallowed once the lab is up."""
        if self.bank:
            return
        vp, d = self.vp, self.d
        vp.inputs.button(BTN_SHIFT, 0)
        d.s.run_ms(25)
        vp.inputs.button(2, 0)
        d.s.run_ms(35)
        vp.inputs.button(2, 1)
        d.s.run_ms(15)
        vp.inputs.button(BTN_SHIFT, 1)
        d.s.run_ms(40)
        self.bank = True
        d.s.pad(0, 19, hold_ms=30, after_ms=40)
        d.s.touch(10, hold_ms=25, after_ms=30)

    def slen(self):
        vp = self.vp
        pv = vp.r32(self.d.sel + 0x5C)
        trk = vp.call_fn(self.d.vfn(pv, 0), pv, MIDI)
        n = vp.call_fn(self.d.vfn(trk, 4), trk) & 0xFFFFFFFF
        return n - 0x100000000 if n >= 0x80000000 else n

    def set_len(self, target):
        if not self.bank:
            self.arm()
        cur = self.slen()
        guard = 0
        while cur != target and guard < 80:
            self.d.s.knob(10, 1 if cur < target else -1, after_ms=35)
            cur = self.slen()
            guard += 1
        return cur

    def clear(self, n):
        for i in range(n):
            if self.d.is_on(MIDI, i):
                self.vp.call_fn(FW_STEP_TOGGLE, self.d.sel, MIDI, i)

    def fill(self, n):
        for i in range(n):
            if not self.d.is_on(MIDI, i):
                self.vp.call_fn(FW_STEP_TOGGLE, self.d.sel, MIDI, i)

    def committed(self):
        return self.rb(OFF_EVO, 100)

    def events(self):
        n = self.u8(OFF_NEV)
        return self.rb(OFF_EVENTS, n * 12)

    def notes(self, n):
        out = []
        for i in range(n):
            out.append((bool(self.d.is_on(MIDI, i)), int(self.d.param(MIDI, i, NOTE) or 0)))
        return out


def main():
    ppf = sys.argv[1]
    snap = os.environ['VPLAY_SNAP']
    shot_dir = os.path.join(os.path.dirname(__file__), '..', 'screenshots')
    os.makedirs(shot_dir, exist_ok=True)
    results = []

    def check(name, ok, detail=None):
        results.append({'name': name, 'pass': bool(ok), 'detail': detail})
        print(('PASS' if ok else 'FAIL'), name, '' if detail is None else detail, flush=True)

    h = H(ppf, snap)
    h.arm()
    opened = h.open_genesis()
    h.use_midi()
    check('open melody', h.u8(OFF_LAB) == 0 and 'Melody' in h.texts()[-40:], opened[-8:])
    h.shot(os.path.join(shot_dir, '01_melody.png'))

    t = h.tap(BTN_TOP)
    check('tap euclid', h.u8(OFF_LAB) == 1 and any('Euclid' in x for x in t), t[-6:])
    h.shot(os.path.join(shot_dir, '01b_euclid.png'))
    t = h.tap(BTN_TOP)
    check('tap arp', h.u8(OFF_LAB) == 2 and any('Arp' in x for x in t), t[-6:])
    h.shot(os.path.join(shot_dir, '01c_arp.png'))
    t = h.tap(BTN_TOP)
    check('tap turing', h.u8(OFF_LAB) == 3 and h.u8(OFF_EVO_MODE) == 0 and
          any(x == 'Turing' for x in h.texts()[-30:]), t[-6:])
    h.shot(os.path.join(shot_dir, '02_turing_config.png'))
    t = h.tap(BTN_TOP)
    check('tap markov', h.u8(OFF_LAB) == 4 and h.u8(OFF_EVO_MODE) == 1 and
          any(x == 'Markov' for x in h.texts()[-30:]), t[-6:])
    h.shot(os.path.join(shot_dir, '02b_markov.png'))
    t = h.tap(BTN_TOP)
    check('tap mutate', h.u8(OFF_LAB) == 5 and h.u8(OFF_EVO_MODE) == 2 and
          any(x == 'Mutate' for x in h.texts()[-30:]), t[-6:])
    h.shot(os.path.join(shot_dir, '02c_mutate.png'))
    t = h.tap(BTN_TOP)
    check('tap chords', h.u8(OFF_LAB) == 6 and any(x == 'Chords' for x in h.texts()[-30:]), t[-6:])
    h.shot(os.path.join(shot_dir, '02d_chords.png'))
    t = h.tap(BTN_TOP)
    check('tap wrap to melody', h.u8(OFF_LAB) == 0 and h.u8(OFF_EVO_MODE) == 2, t[-4:])
    for _ in range(3):
        h.tap(BTN_TOP, 180)
    check('back on turing', h.u8(OFF_LAB) == 3 and h.u8(OFF_EVO_MODE) == 0)

    # Phrase length clamp on the live pattern. Phrase is not the Turing register length.
    check('focus phrase', h.focus('Phrase'))
    h.turns(-40)
    check('length floor 1', h.u8(OFF_LEN) == 1, h.u8(OFF_LEN))
    native = h.u8(OFF_NATIVE)
    h.turns(80)
    check('length ceiling is native', h.u8(OFF_LEN) == native, (h.u8(OFF_LEN), native))

    # Turing preview does not advance the committed register, and repeats.
    notes_before = h.notes(16)
    t = h.tap(BTN_CLICK, 500)
    ev1 = h.events()
    reg1 = h.committed()
    check('turing preview', h.u8(OFF_ST) == 1 and h.u8(OFF_NEV) > 0 and
          any('Preview' in x or x == 'Turing' for x in t), t[-8:])
    check('preview writes no notes', h.notes(16) == notes_before)
    h.shot(os.path.join(shot_dir, '03_turing_preview.png'))
    h.tap(BTN_CLICK, 500)
    check('repeat preview identical', h.events() == ev1 and h.committed() == reg1)
    check('repeat preview writes no notes', h.notes(16) == notes_before)

    # Editing invalidates. Mode switch drops the proposal.
    h.tap(BTN_BACK, 200)  # leave preview, stay in the lab
    check('back from preview stays', h.wmode() == WMODE_GENESIS and h.u8(OFF_ST) == 0)
    h.tap(BTN_CLICK, 450)
    check('focus evolve', h.focus('Evolve'))
    h.turns(1)
    check('edit invalidates preview', h.u8(OFF_ST) == 0 and h.u8(OFF_NEV) == 0)
    h.tap(BTN_CLICK, 450)
    check('markov is a direct lab', h.goto_lab(4) and h.u8(OFF_LAB) == 4)
    check('markov discards turing proposal', h.u8(OFF_EVO_MODE) == 1 and h.u8(OFF_ST) == 0)
    h.shot(os.path.join(shot_dir, '04_markov_step.png'))

    names = ['Step', 'Motif', 'Bass', 'Expl', 'Learn']
    check('focus style', h.focus('Style'))
    for _ in range(6):
        if h.u8(OFF_STYLE) == 0:
            break
        h.turns(-1)
    for i, name in enumerate(names):
        check('style ' + name, h.u8(OFF_STYLE) == i, h.u8(OFF_STYLE))
        if name == 'Learn':
            h.shot(os.path.join(shot_dir, '05_markov_learn.png'))
        if i + 1 < len(names):
            h.turns(1)

    # Empty learned source.
    h.clear(h.slen() if h.slen() > 0 else 16)
    h.d.s.run_ms(40)
    t = h.tap(BTN_CLICK, 500)
    check('empty learn', any('No source notes' in x for x in t), t[-8:])
    h.d.s.run_ms(80)
    check('learn error shown', 'Error' in h.texts()[-25:] or 'No model' in h.texts()[-25:],
          h.texts()[-12:])
    h.shot(os.path.join(shot_dir, '06_markov_empty.png'))
    check('learn did not publish', h.u8(OFF_PENDING) == 0 and h.u8(OFF_READY) == 0)

    # Back acknowledges the error and stays on the lab.
    h.tap(BTN_BACK, 180)
    check('error back stays', h.wmode() == WMODE_GENESIS and h.u8(OFF_ST) == 0)
    h.focus('Style')
    for _ in range(6):
        if h.u8(OFF_STYLE) == 0:
            break
        h.turns(-1)
    t = h.tap(BTN_CLICK, 500)
    check('preset after empty', h.u8(OFF_STYLE) == 0 and h.u8(OFF_ST) == 1,
          (h.u8(OFF_STYLE), h.u8(OFF_ST), t[-4:]))
    check('preset is not pending', h.u8(OFF_PENDING) == 0)

    # Mutation controls. The lab itself selects the shared engine.
    h.tap(BTN_BACK, 180)
    check('mutate is a direct lab', h.goto_lab(5))
    check('mutate mode', h.u8(OFF_LAB) == 5 and h.u8(OFF_EVO_MODE) == 2, h.u8(OFF_EVO_MODE))
    h.shot(os.path.join(shot_dir, '07_mutate_config.png'))

    check('focus rot', h.focus('RotBy'))
    h.turns(-1)
    rot = struct.unpack('<b', bytes([h.u8(OFF_MROT)]))[0]
    check('signed rotate -1', rot == -1, rot)
    h.turns(-40)
    rot = struct.unpack('<b', bytes([h.u8(OFF_MROT)]))[0]
    check('rotate floor', rot == -(h.u8(OFF_LEN) - 1), rot)
    h.turns(80)
    rot = struct.unpack('<b', bytes([h.u8(OFF_MROT)]))[0]
    check('rotate ceiling', rot == h.u8(OFF_LEN) - 1, rot)
    # return to 0
    h.turns(-(h.u8(OFF_LEN) - 1))

    check('focus attr', h.focus('Attr'))
    h.turns(1)
    check('attr pitch bit', h.u8(OFF_MPROT) == 1 and any(x == 'P' for x in h.texts()[-12:]),
          h.u8(OFF_MPROT))
    h.turns(2)
    check('attr pitch+rhythm', h.u8(OFF_MPROT) == 3 and any(x == 'P+R' for x in h.texts()[-12:]),
          h.u8(OFF_MPROT))
    h.turns(12)
    check('attr all bits', h.u8(OFF_MPROT) == 15 and any(x == 'P+R+V+D' for x in h.texts()[-12:]))
    h.turns(-15)
    check('attr none', h.u8(OFF_MPROT) == 0 and any(x == 'None' for x in h.texts()[-12:]))

    # Multiple protected steps via the step pads already swallowed by Genesis.
    h.d.s.pad(0, 1, hold_ms=30, after_ms=80)
    h.d.s.pad(0, 4, hold_ms=30, after_ms=80)
    bits = h.rb(OFF_PSTEP, 8)
    check('pads protect 1 and 4', (bits[0] & 0x12) == 0x12, list(bits))
    h.shot(os.path.join(shot_dir, '08_mutate_protect.png'))
    h.d.s.pad(0, 1, hold_ms=30, after_ms=60)
    check('pad clears step 1', (h.u8(OFF_PSTEP) & 0x02) == 0)
    check('focus clear', h.focus('Clr'))
    t = h.turns(1)
    check('clear steps', h.rb(OFF_PSTEP, 8) == bytes(8) and any('Steps clear' in x for x in t),
          t[-4:])

    # Unsupported reverse + protection, then a full grid that cannot accept a move.
    h.fill(1)
    h.d.s.pad(0, 0, hold_ms=30, after_ms=60)
    h.focus('Rev')
    h.turns(1)
    t = h.tap(BTN_CLICK, 500)
    check('reverse plus step blocked', any('Rev/rot blocked' in x for x in t), t[-8:])
    h.shot(os.path.join(shot_dir, '09_mutate_blocked.png'))
    h.tap(BTN_BACK, 180)
    h.focus('Rev')
    if h.u8(OFF_MREV):
        h.turns(-1)
    h.focus('Clr')
    h.turns(1)

    nlen = h.slen()
    h.fill(nlen if 0 < nlen <= 64 else 16)
    for off, val in ((OFF_MPITCH, 0), (OFF_MDISP, 100), (OFF_MVEL, 0), (OFF_MGATE, 0),
                     (OFF_MINS, 0), (OFF_MREM, 0), (OFF_MREV, 0), (OFF_MROT, 0)):
        h.poke(off, bytes([val]))
    src = h.notes(16)
    t = h.tap(BTN_CLICK, 600)
    check('full grid no displacement', any('No note change' in x for x in t) and h.u8(OFF_ST) == 1,
          t[-8:])
    check('mutation preview writes no notes', h.notes(16) == src)

    # Lengths. Phrase 1 on the current pattern, then fresh sessions for 32 and 64.
    h.tap(BTN_BACK, 150)
    check('turing lab again', h.goto_lab(3) and h.u8(OFF_EVO_MODE) == 0)
    h.focus('Phrase')
    h.turns(-40)
    t = h.tap(BTN_CLICK, 500)
    check('phrase length 1', h.u8(OFF_LEN) == 1 and h.u8(OFF_ST) == 1 and h.u8(OFF_NEV) <= 1, 
          (h.u8(OFF_LEN), h.u8(OFF_NEV), t[-4:]))

    # Out-of-range phrase length is clamped back to the live pattern. 128 is not kept.
    h.tap(BTN_BACK, 120)
    h.poke(OFF_LEN, bytes([128]))
    h.tap(BTN_CLICK, 400)
    check('len 128 clamped', h.u8(OFF_LEN) == h.u8(OFF_NATIVE) and h.u8(OFF_LEN) <= 64,
          (h.u8(OFF_LEN), h.u8(OFF_NATIVE)))
    h.poke(OFF_LEN, bytes([0]))
    h.tap(BTN_CLICK, 400)
    check('len 0 becomes default or native', 1 <= h.u8(OFF_LEN) <= 64, h.u8(OFF_LEN))

    def length_session(target, shot_name):
        s = H(ppf, snap)
        got = s.set_len(target)
        s.open_genesis()
        s.use_midi()
        s.focus('Phrase')
        for _ in range(80):
            if s.u8(OFF_LEN) >= target:
                break
            s.turns(1)
        for _ in range(3):
            s.tap(BTN_TOP, 160)
        s.shot(os.path.join(shot_dir, shot_name))
        t = s.tap(BTN_CLICK, 700)
        ok = got == target and s.u8(OFF_NATIVE) == target and s.u8(OFF_ST) == 1
        check('native %d' % target, ok, (got, s.u8(OFF_NATIVE), s.u8(OFF_LEN), s.u8(OFF_NEV), t[-4:]))
        if target > 16:
            # Encoder pages the preview. One turn should reveal page 1.
            s.turns(1)
            check('page %d' % target, s.u8(OFF_PAGE) == 1, s.u8(OFF_PAGE))
        return s

    length_session(32, '10_len32.png')
    s64 = length_session(64, '11_len64.png')
    # Step 16 is on page 1. The step cursor selects that page; the pad toggles it.
    s64.tap(BTN_BACK, 150)
    check('mutate lab for step 16', s64.goto_lab(5) and s64.u8(OFF_EVO_MODE) == 2)
    s64.focus('Step')
    s64.turns(16)
    check('step cursor page', s64.u8(OFF_MSTEP) == 16 and s64.u8(OFF_PAGE) == 1,
          (s64.u8(OFF_MSTEP), s64.u8(OFF_PAGE)))
    s64.d.s.pad(0, 0, hold_ms=30, after_ms=80)
    check('protect step 16', (s64.rb(OFF_PSTEP, 8)[2] & 0x01) == 1, list(s64.rb(OFF_PSTEP, 8)))

    # Apply once, reject a second apply, then a restore failure.
    a = H(ppf, snap)
    a.arm()
    a.open_genesis()
    a.use_midi()
    for _ in range(3):
        a.tap(BTN_TOP, 160)
    reg = a.committed()
    a.tap(BTN_CLICK, 500)
    t = a.tap(BTN_BOT, 600)
    check('apply once', a.u8(OFF_APPLIED) == 1 and a.u8(OFF_PHASE) == 4 and
          any('Applied' in x for x in t), t[-6:])
    check('apply advanced register', a.committed() != reg)
    after = a.notes(16)
    t = a.tap(BTN_BOT, 250)
    check('second apply refused', any('Generate first' in x for x in t) and a.notes(16) == after,
          t[-4:])

    # Fresh preview, then stage-7 restore failure: writes may stick, state must not advance.
    a.clear(a.slen() if 0 < a.slen() <= 64 else 16)
    a.tap(BTN_CLICK, 500)
    a.poke(OFF_DIAG, bytes([7]))
    t = a.tap(BTN_BOT, 600)
    check('restore failure toast', any('Pattern may differ' in x for x in t), t[-6:])
    check('restore sets recovery', a.u8(OFF_RECOVERY) == 1 and a.u8(OFF_ST) == 2)
    check('restore does not apply session', a.u8(OFF_PHASE) != 4)
    check('restore dropped preview', a.u8(OFF_NEV) == 0 and a.u8(OFF_PENDING) == 0)
    t = a.tap(BTN_BOT, 250)
    check('retry blocked', any('Recovery required' in x for x in t), t[-4:])
    hshot = os.path.join(shot_dir, '12_restore_failure.png')
    a.shot(hshot)
    # Unshifted Back leaves and keeps the lock.
    a.tap(BTN_BACK, 200)
    check('error back keeps recovery', a.wmode() == WMODE_GENESIS and a.u8(OFF_RECOVERY) == 1 and
          a.u8(OFF_ST) == 0)
    a.tap(BTN_BACK, 300)
    check('second back leaves', a.wmode() != WMODE_GENESIS)
    a.open_genesis()
    t = a.tap(BTN_BOT, 250)
    check('lock survives leave', a.u8(OFF_RECOVERY) == 1 and any('Recovery required' in x for x in t),
          t[-4:])
    lab_locked = a.u8(OFF_LAB)
    t = a.tap(BTN_TOP, 200)
    check('lab change cannot clear recovery', a.u8(OFF_RECOVERY) == 1 and a.u8(OFF_LAB) == lab_locked and
          any('Recovery required' in x for x in t), t[-4:])
    # Shift+Back must not clear the lock or claim the pattern was restored.
    a.vp.inputs.button(BTN_SHIFT, 0)
    a.d.s.run_ms(20)
    t = a.tap(BTN_BACK, 300)
    a.vp.inputs.button(BTN_SHIFT, 1)
    a.d.s.run_ms(30)
    check('shift back keeps lock', a.u8(OFF_RECOVERY) == 1 and a.wmode() == WMODE_GENESIS and
          any('Recovery required' in x for x in t) and not any('Check pattern' in x for x in t),
          t[-4:])
    # The production UI has no unlock. The exported setter is the harness-only release
    # so the rest of this session can keep testing. Then Back returns ST_ERR to config.
    syms = load_syms(os.path.join(os.path.dirname(ppf), 'link.map'))
    a.vp.call_fn(syms['cfw_genesis_set_recovery'], 0)
    a.tap(BTN_BACK, 200)

    # Turing reset changes the register immediately and writes no notes.
    check('turing after unlock', a.goto_lab(3) and a.u8(OFF_EVO_MODE) == 0)
    a.focus('Phrase')
    a.turns(-40)
    a.clear(a.slen() if 0 < a.slen() <= 64 else 16)
    a.tap(BTN_CLICK, 400)
    a.tap(BTN_BOT, 500)
    reg = a.committed()
    notes = a.notes(16)
    a.focus('Reset')
    t = a.turns(1)
    check('reset toast', any('Register reset' in x for x in t), t[-4:])
    check('reset changes register', a.committed() != reg)
    check('reset writes no notes', a.notes(16) == notes)
    check('reset is not apply', a.u8(OFF_PHASE) != 4 or a.committed() != reg)

    # Lock is visible, then leave back to the stock screen.
    a.focus('Lock')
    a.turns(1)
    a.d.s.run_ms(60)
    check('lock visible', a.u8(OFF_LOCK) == 1 and any(x == 'LOCK' for x in a.texts()[-20:]))
    a.shot(os.path.join(shot_dir, '13_turing_lock.png'))
    a.tap(BTN_BACK, 250)
    check('returned to stock', a.wmode() != WMODE_GENESIS)
    a.shot(os.path.join(shot_dir, '14_stock.png'))

    # Knob past 64 cannot select a 128-step pattern.
    b = H(ppf, snap)
    b.arm()
    top = b.set_len(96)
    check('native length stops at 64', top <= 64, top)
    check('native 128 unavailable', top != 128, top)

    outp = os.path.join(os.path.dirname(__file__), '..', 'interact_results.json')
    json.dump(results, open(outp, 'w'), indent=2)
    failed = [r for r in results if not r['pass']]
    print('TOTAL', len(results), 'FAIL', len(failed), flush=True)
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
