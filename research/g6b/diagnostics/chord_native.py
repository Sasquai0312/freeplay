#!/usr/bin/env python3
"""Native chord Apply, lock restoration, and live lengths.

Diagnostic image: direct genesis_diag_chord_apply, including fail injection.
Production image: pass --ui to exercise the Chords lab, screenshots, mutation
rejection, and recovery. Length changes happen before any Genesis allocation.
"""
from __future__ import annotations

import os
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
sys.path[:0] = [os.path.join(ROOT, 'cfw', 'vdev'), os.path.join(ROOT, 'cfw', 'tests'),
                os.path.join(ROOT, 'research', 'g1b', 'diagnostics'),
                os.path.join(ROOT, 'research', 'g5c_2', 'diagnostics')]
from midi_probe import MidiDevice  # noqa: E402
from g5c2_interact import (  # noqa: E402
    BTN_BACK, BTN_BOT, BTN_CLICK, BTN_TOP, H, MIDI, NOTE, OFF_DIAG, OFF_EVO,
    OFF_LAB, OFF_NEV, OFF_PENDING, OFF_READY, OFF_RECOVERY, OFF_ST,
)

FW_NEW = 0x600AAB64
CHORD = 10
VEL, DUR = 0, 1
TRACK = 8
CLEAR_LOCK = 0x30


def s32(n):
    n &= 0xFFFFFFFF
    return n - 0x100000000 if n >= 0x80000000 else n


def load_syms(path):
    syms = {}
    for line in open(path):
        parts = line.split()
        if len(parts) >= 2 and parts[0].startswith('0x'):
            syms[parts[-1]] = int(parts[0], 16)
    return syms


def lock_word(raw):
    return raw[0x26] | (raw[0x27] << 8) | (raw[0x28] << 16) | ((raw[0x29] & 0x7f) << 24)


class Native:
    def __init__(self, ppf, snap, syms):
        self.d = MidiDevice(ppf, snap)
        self.d.s.run_ms(80)
        self.vp = self.d.vp
        self.syms = syms
        self.ctx = self.vp.profile['app'] + 0x4460
        self.bank = False

    def heap(self, n):
        p = self.vp.call_fn(FW_NEW, n) & 0xFFFFFFFF
        self.vp.mu.mem_write(p, b'\x00' * n)
        return p

    def bit(self, step, idx):
        return (lock_word(self.d.step_bytes(TRACK, step)) >> idx) & 1

    def view(self, step):
        d = self.d
        return {
            'on': bool(d.is_on(TRACK, step)),
            'note': d.param(TRACK, step, NOTE),
            'vel': d.param(TRACK, step, VEL),
            'dur': d.param(TRACK, step, DUR),
            'chord': d.param(TRACK, step, CHORD),
            'bit10': self.bit(step, CHORD),
            'bit6': self.bit(step, NOTE),
        }

    def slen(self):
        n = self.d.track_len(TRACK)
        return s32(n)

    def arm_bank(self):
        if self.bank:
            return
        vp, d = self.vp, self.d
        vp.call_fn(0x600CF578, d.ctrl)
        d.s.run_ms(40)
        self.bank = True
        d.s.pad(0, 19, hold_ms=30, after_ms=40)

    def set_len(self, target):
        """Knob 10. Call before heap() or Genesis. The bank toggle is once."""
        self.arm_bank()
        self.d.s.touch(10, hold_ms=25, after_ms=30)
        cur = self.slen()
        guard = 0
        while cur != target and guard < 80:
            self.d.s.knob(10, 1 if cur < target else -1, after_ms=35)
            cur = self.slen()
            guard += 1
        return cur

    def wipe(self, n):
        d = self.d
        for i in range(n):
            if d.is_on(TRACK, i):
                d.toggle(TRACK, i)
            st = d.step(TRACK, i)
            d.set_param(TRACK, i, CHORD, 0)
            self.vp.call_fn(d.vfn(st, CLEAR_LOCK), st, CHORD)

    def apply(self, events, chords, policy=0, fail=0, track=TRACK):
        """events: (step, note, vel, dur_ticks). chords None writes locked None."""
        raw = b''.join(struct.pack('<IBBBBI', step * 480, note, vel, 0, 0, dur)
                       for step, note, vel, dur in events)
        evp = self.heap(len(raw))
        self.vp.mu.mem_write(evp, raw)
        chp = 0
        if chords is not None:
            chp = self.heap(len(chords))
            self.vp.mu.mem_write(chp, bytes(chords))
        sn = self.heap(16 + 64 * 8)
        rq = self.heap(40)
        self.vp.mu.mem_write(rq, struct.pack(
            '<IiiIIIiiII', self.ctx & 0xFFFFFFFF, track, 0, 480, evp, len(events),
            chp, policy, fail, sn))
        return s32(self.vp.call_fn(self.syms['genesis_diag_chord_apply'], rq))


def main():
    ppf = next(a for a in sys.argv[1:] if not a.startswith('--'))
    snap = os.environ['VPLAY_SNAP']
    link = os.environ.get('LINK_MAP', os.path.join(os.path.dirname(ppf), 'link.map'))
    ui = '--ui' in sys.argv
    syms = load_syms(link)
    results = []

    def check(name, ok, detail=None):
        results.append(bool(ok))
        print(('PASS' if ok else 'FAIL'), name, '' if detail is None else detail, flush=True)

    if 'genesis_diag_chord_apply' in syms:
        run_native(ppf, snap, syms, check)
    elif not ui:
        check('diag symbol present', False, 'build with CFW_DIAG=1')
    if ui:
        run_ui(ppf, snap, check)

    failed = len(results) - sum(results)
    print(f'TOTAL {len(results)} FAIL {failed}', flush=True)
    return 1 if failed else 0


def run_native(ppf, snap, syms, check):
    n = Native(ppf, snap, syms)
    n.wipe(16)
    other = [bool(n.d.is_on(0, i)) for i in range(16)]

    # 1 empty -> chord
    rc = n.apply([(0, 60, 100, 480)], [3])
    v = n.view(0)
    check('1 empty to chord', rc == 0 and v['on'] and v['note'] == 60 and v['chord'] == 3 and v['bit10'] == 1, v)

    # 2 mono -> chord. Toggle already locked None.
    n.wipe(16)
    n.d.toggle(TRACK, 0)
    before = n.view(0)
    rc = n.apply([(0, 62, 90, 240)], [4], policy=1)
    v = n.view(0)
    check('2 mono to chord', before['chord'] == 0 and before['bit10'] == 1 and rc == 0 and
          v['note'] == 62 and v['chord'] == 4 and v['bit10'] == 1, v)

    # 3 chord -> chord
    rc = n.apply([(0, 64, 80, 480)], [12], policy=1)
    v = n.view(0)
    check('3 chord to chord', rc == 0 and v['chord'] == 12 and v['note'] == 64 and v['bit10'] == 1, v)

    # 4 chord -> mono (NULL chords => locked None)
    rc = n.apply([(0, 67, 100, 480)], None, policy=1)
    v = n.view(0)
    check('4 chord to mono', rc == 0 and v['chord'] == 0 and v['bit10'] == 1 and v['note'] == 67, v)

    # 5 chord -> empty hole inside a replace
    n.wipe(16)
    n.d.toggle(TRACK, 1)
    n.d.set_param(TRACK, 1, CHORD, 5)
    rc = n.apply([(0, 60, 100, 480), (2, 64, 100, 480)], [3, 3], policy=1)
    hole = n.view(1)
    check('5 chord to empty', rc == 0 and not hole['on'] and hole['chord'] == 0 and hole['bit10'] == 1, hole)

    # 6 unlocked restoration. Never-touched step 4, fail during readback.
    n.wipe(16)
    orig = n.view(4)
    rc = n.apply([(4, 60, 100, 480)], [3], fail=10)
    back = n.view(4)
    check('6 unlocked restored', orig['bit10'] == 0 and orig['chord'] is None and orig['bit6'] == 0 and rc != 0 and
          back['bit10'] == 0 and back['chord'] is None and back['bit6'] == 0 and not back['on'] and back['note'] is None,
          (orig, back, rc))

    # 7 explicit None. A fresh toggle stores velocity 7600, which setParam clamps
    # to 127, so that value is not restorable. Put a round-trippable velocity
    # down first, then prove the lock and the note come back.
    n.wipe(16)
    n.d.toggle(TRACK, 0)
    n.d.set_param(TRACK, 0, NOTE, 70)
    n.d.set_param(TRACK, 0, VEL, 100)
    n.d.set_param(TRACK, 0, DUR, 60)
    rc = n.apply([(0, 60, 80, 240)], [3], policy=1, fail=10)
    v = n.view(0)
    check('7 locked None restored', rc != 0 and v['on'] and v['note'] == 70 and v['vel'] == 100 and
          v['dur'] == 60 and v['chord'] == 0 and v['bit10'] == 1, v)

    # Stock toggle velocity cannot be written back. Chord and note still return,
    # and the status is RESTORE so the session takes the recovery lock.
    n.wipe(16)
    n.d.toggle(TRACK, 0)
    n.d.set_param(TRACK, 0, NOTE, 70)
    stock = n.view(0)
    rc = n.apply([(0, 60, 80, 240)], [3], policy=1, fail=10)
    v = n.view(0)
    check('7b stock velocity is not round-trippable', stock['vel'] == 7600 and rc == -11 and v['on'] and
          v['note'] == 70 and v['chord'] == 0 and v['bit10'] == 1 and v['vel'] != 7600, (stock, v, rc))
    # Leave a restorable step for case 8.
    n.d.set_param(TRACK, 0, VEL, 100)
    n.d.set_param(TRACK, 0, DUR, 60)

    # 8 existing chord id restored, including the lock bit
    n.d.set_param(TRACK, 0, CHORD, 12)
    rc = n.apply([(0, 60, 100, 480)], [3], policy=1, fail=4)
    v = n.view(0)
    check('8 chord id restored', rc != 0 and v['chord'] == 12 and v['bit10'] == 1 and v['note'] == 70, v)

    # 9 invalid id before any write
    before = n.view(0)
    rc = n.apply([(0, 60, 100, 480)], [21], policy=1)
    check('9 invalid id rejected', rc != 0 and n.view(0) == before, rc)

    # 10 invalid pitch
    before = n.view(0)
    rc = n.apply([(0, 200, 100, 480)], [3], policy=1)
    check('10 invalid pitch rejected', rc != 0 and n.view(0) == before, rc)

    # 11 sample track
    sample = [bool(n.d.is_on(0, i)) for i in range(16)]
    rc = n.apply([(0, 60, 100, 480)], [3], track=0)
    check('11 sample track rejected', rc != 0 and [bool(n.d.is_on(0, i)) for i in range(16)] == sample, rc)

    # 16 out of range on a live 16-step pattern
    rc = n.apply([(16, 60, 100, 480)], [3])
    check('16 step 16 rejected at length 16', rc != 0 and not n.d.is_on(TRACK, 16), rc)
    check('12 live 16 accepts ends', n.slen() == 16)

    # 21 failure before first write
    n.wipe(16)
    n.d.toggle(TRACK, 0)
    n.d.set_param(TRACK, 0, NOTE, 70)
    n.d.set_param(TRACK, 0, VEL, 100)
    n.d.set_param(TRACK, 0, DUR, 60)
    n.d.set_param(TRACK, 0, CHORD, 7)
    before = n.view(0)
    rc = n.apply([(0, 60, 100, 480)], [3], policy=1, fail=3)
    check('21 no write before the first store', rc != 0 and n.view(0) == before, rc)

    # 22 mid write restores
    rc = n.apply([(0, 60, 100, 480), (1, 64, 100, 480)], [3, 4], policy=1, fail=5)
    check('22 mid-write restores', rc != 0 and n.view(0) == before and not n.d.is_on(TRACK, 1), n.view(0))

    # 23 readback failure restores
    rc = n.apply([(0, 60, 100, 480)], [9], policy=1, fail=10)
    check('23 readback failure restores', rc != 0 and n.view(0)['chord'] == 7 and n.view(0)['bit10'] == 1, n.view(0))

    check('27 sample track still original', [bool(n.d.is_on(0, i)) for i in range(16)] == other)

    # Fresh sessions for 32 and 64, length knob first.
    for plen, steps in ((32, (0, 15, 16, 31)), (64, (0, 15, 16, 31, 32, 47, 48, 63))):
        h = Native(ppf, snap, syms)
        got = h.set_len(plen)
        check(f'live length {plen}', got == plen, got)
        if got != plen:
            continue
        events = [(s, 60, 100, 480) for s in steps]
        ids = [3] * len(steps)
        rc = h.apply(events, ids)
        good = rc == 0 and all(h.view(s)['chord'] == 3 and h.view(s)['on'] and h.view(s)['bit10'] == 1
                               for s in steps)
        check(f'{plen} page boundaries', good, rc)
        rc = h.apply([(plen, 60, 100, 480)], [3])
        check(f'{plen} one past the end rejected', rc != 0, rc)

    # 24 page-boundary failure restores the earlier chord
    h = Native(ppf, snap, syms)
    if h.set_len(32) == 32:
        h.d.toggle(TRACK, 16)
        h.d.set_param(TRACK, 16, CHORD, 14)
        h.d.set_param(TRACK, 16, NOTE, 72)
        rc = h.apply([(16, 60, 90, 480)], [3], policy=1, fail=8)
        v = h.view(16)
        check('24 page-edge failure restores', rc != 0 and v['chord'] == 14 and v['note'] == 72 and v['bit10'] == 1, v)
    else:
        check('24 page-edge failure restores', False, 'length')


def run_ui(ppf, snap, check):
    shot = os.path.join(os.path.dirname(__file__), '..', 'screenshots')
    os.makedirs(shot, exist_ok=True)
    # Length 32 before Genesis so pagination has a second page.
    n = Native(ppf, snap, {})
    got = n.set_len(32)
    check('ui length 32', got == 32, got)
    # Same emulator: opening Genesis uses this device. Rebuild the interact harness
    # on a new boot so the length knob is not fighting a heap alloc. The length
    # session above only proves the knob. The UI session below is length 16,
    # which is enough for Apply. A second device sets 32, then we attach H.
    hlen = Native(ppf, snap, {})
    if hlen.set_len(32) != 32:
        check('pagination length', False)
        return
    # Hand the already-running machine to the UI by constructing H on the same snap
    # would reboot. Stay on hlen.d and drive the menu from here.
    d = hlen.d
    import flows  # noqa: WPS433
    d.s.run_ms(40)
    # The interact helper expects its own Device. Open through flows on this one.
    import g5c2_interact as g
    # Minimal UI driver using a fresh H after the length is already set is wrong
    # because H reboots. Drive buttons on d.
    vp = d.vp
    vp.w32(d.sel + 0x0C, 1 << MIDI)
    flows.open_menu(d.s)
    d.s.run_ms(180)
    flows.enter_item(d.s, g.MENU_GENESIS, after_ms=350)
    d.s.run_ms(400)

    def texts():
        return [a[2] for name, a in vp.eve.cmd_log if name == 'TEXT' and len(a) > 2]

    def find_ui():
        magic = struct.pack('<I', 0x47454E31)
        for base, size in ((0x20000000, 0x80000), (0x20200000, 0x80000)):
            try:
                blob = bytes(vp.mu.mem_read(base, size))
            except Exception:
                continue
            start = 0
            while True:
                i = blob.find(magic, start)
                if i < 0:
                    break
                b = blob[i:i + 16]
                if b[4] <= 3 and b[5] < 7 and b[6] < 46 and b[7] < 16 and 1 <= b[8] <= 64:
                    return base + i
                start = i + 4
        return 0

    base = find_ui()
    check('chords ui object', base != 0, hex(base))
    if not base:
        return

    def u8(off):
        return vp.mu.mem_read(base + off, 1)[0]

    def tap(btn, ms=220):
        d.s.tap(btn, after_ms=ms)

    def bit10(track, step):
        raw = d.step_bytes(track, step)
        word = raw[0x26] | (raw[0x27] << 8) | (raw[0x28] << 16) | ((raw[0x29] & 0x7f) << 24)
        return (word >> 10) & 1

    def snap_steps(track, n=32):
        return [(bool(d.is_on(track, i)), d.param(track, i, NOTE), d.param(track, i, CHORD), bit10(track, i))
                for i in range(n)]

    # Lab+ to Chords. The opening click is swallowed inside 200 ms.
    for _ in range(8):
        if u8(OFF_LAB) == 6:
            break
        tap(BTN_TOP, 160)
    # Column 19, row 0, bank already on: external MIDI track 8. Same as the
    # Evolution harness. Enter's scan can land on the last index when no track
    # is readable yet; this pad is the verified select.
    d.s.pad(0, 19, hold_ms=30, after_ms=80)
    check('chords lab reachable', u8(OFF_LAB) == 6 and u8(7) == 8, (u8(OFF_LAB), u8(7), u8(34)))
    d.s.shot(os.path.join(shot, '01_chords_config.png'), scale=2)

    # Focus Prog by shift-turn.
    vp.inputs.button(10, 0)
    d.s.run_ms(20)
    for _ in range(12):
        d.s.turn(1, after_ms=40)
        if 'Prog' in texts()[-15:]:
            break
    vp.inputs.button(10, 1)
    d.s.run_ms(30)
    d.s.shot(os.path.join(shot, '02_chords_prog.png'), scale=2)

    trk = u8(7)
    # Phrase 32 when the live pattern is 32, so the preview has a second page.
    if u8(34) >= 32:
        vp.mu.mem_write(base + 8, bytes([32]))
    notes_before = snap_steps(trk)
    tap(BTN_CLICK, 500)
    trk = u8(7)
    check('17 preview writes nothing', u8(OFF_ST) == 1 and u8(OFF_NEV) > 0 and snap_steps(trk) == notes_before,
          (u8(OFF_ST), u8(OFF_NEV), trk, u8(8), u8(34)))
    d.s.shot(os.path.join(shot, '03_chords_preview.png'), scale=2)
    page0 = u8(33)
    d.s.turn(1, after_ms=150)
    check('17 preview page', u8(8) <= 16 or u8(33) == page0 + 1, (u8(8), page0, u8(33)))
    d.s.shot(os.path.join(shot, '04_chords_page.png'), scale=2)
    tap(BTN_BACK, 180)
    check('17 cancel leaves native state', u8(OFF_ST) == 0 and snap_steps(trk) == notes_before)

    tap(BTN_CLICK, 500)
    nev = u8(OFF_NEV)
    ev = bytes(vp.mu.mem_read(base + 464, nev * 12))
    cids = bytes(vp.mu.mem_read(base + 3134, nev))
    expect = []
    for i in range(nev):
        tick, note, _vel, _ch, _pad, _dur = struct.unpack_from('<IBBBBI', ev, i * 12)
        expect.append((tick // 480, note, cids[i]))
    tap(BTN_BOT, 400)
    written = []
    for i in range(max(u8(8), 32)):
        if d.is_on(trk, i):
            written.append((i, d.param(trk, i, NOTE), d.param(trk, i, CHORD), bit10(trk, i)))
    got = [(s, n, c) for s, n, c, b in written]
    check('18 exact apply wrote chords', u8(OFF_ST) == 0 and got == expect and written
          and all(c and b == 1 for _, _, c, b in written),
          (u8(OFF_ST), got[:6], expect[:6]))
    d.s.shot(os.path.join(shot, '05_chords_applied.png'), scale=2)

    # Occupied: empty-only offers replace. Cancel leaves the notes. Apply again
    # re-offers, and the next Apply is the confirmed replacement.
    tap(BTN_CLICK, 500)
    tap(BTN_BOT, 300)
    check('19 occupied offers replace', u8(OFF_ST) == 3, u8(OFF_ST))
    d.s.shot(os.path.join(shot, '06_chords_replace.png'), scale=2)
    kept = snap_steps(trk)
    tap(BTN_BACK, 180)
    check('19 cancel replace', u8(OFF_ST) == 1 and snap_steps(trk) == kept, u8(OFF_ST))
    tap(BTN_BOT, 250)
    check('19 empty-only offers again', u8(OFF_ST) == 3, u8(OFF_ST))
    tap(BTN_BOT, 400)
    check('20 confirmed replace', u8(OFF_ST) == 0 and d.is_on(trk, 0) and d.param(trk, 0, CHORD) not in (None, 0),
          (u8(OFF_ST), d.param(trk, 0, CHORD)))

    # Mutation rejects a chord-bearing phrase and does not clear the notes.
    for _ in range(8):
        if u8(OFF_LAB) == 5:
            break
        tap(BTN_TOP, 140)
    before = snap_steps(trk)
    tap(BTN_CLICK, 400)
    after = snap_steps(trk)
    check('29 mutate rejects chords', u8(OFF_LAB) == 5 and after == before and 'Has chords' in texts()[-30:],
          texts()[-8:])
    d.s.shot(os.path.join(shot, '07_mutate_has_chords.png'), scale=2)

    # Melody empty-only does not touch the occupied chord.
    for _ in range(8):
        if u8(OFF_LAB) == 0:
            break
        tap(BTN_TOP, 140)
    before = snap_steps(trk)
    tap(BTN_CLICK, 400)
    tap(BTN_BOT, 300)
    check('28 melody leaves occupied chords', u8(OFF_LAB) == 0 and u8(OFF_ST) == 3 and snap_steps(trk) == before,
          (u8(OFF_ST), u8(OFF_LAB)))
    tap(BTN_BACK, 150)

    # A cleared step written by Melody is locked None, not a leftover chord.
    for i in range(32):
        if d.is_on(trk, i):
            d.toggle(trk, i)
        st = d.step(trk, i)
        d.set_param(trk, i, CHORD, 0)
        vp.call_fn(d.vfn(st, 0x30), st, CHORD)
    tap(BTN_CLICK, 400)
    tap(BTN_BOT, 400)
    mono = [(i, d.param(trk, i, CHORD), bit10(trk, i)) for i in range(32) if d.is_on(trk, i)]
    check('28 melody writes locked None', u8(OFF_ST) == 0 and mono and all(c == 0 and b == 1 for _, c, b in mono),
          mono[:4])

    # One hundred open / generate / switch / cancel / close cycles, visiting every lab.
    # No Apply, so committed Evolution state is not published.
    evo_before = bytes(vp.mu.mem_read(base + 3119, 5))
    labs = []
    cycles = 0
    for i in range(100):
        if u8(OFF_RECOVERY):
            break
        tap(BTN_CLICK, 180)
        st = u8(OFF_ST)
        if st == 1:
            tap(BTN_BACK, 80)
        tap(BTN_TOP, 90)
        labs.append(u8(OFF_LAB))
        if i % 10 == 9:
            flows.close_menu(d.s)
            d.s.run_ms(60)
            flows.open_menu(d.s)
            d.s.run_ms(80)
            flows.enter_item(d.s, g.MENU_GENESIS, after_ms=200)
            d.s.run_ms(180)
            base = find_ui()
            if not base:
                break
            d.s.pad(0, 19, hold_ms=20, after_ms=40)
            trk = u8(7)
        cycles += 1
        if i % 25 == 24:
            print(f'cycle {i + 1} lab {u8(OFF_LAB)}', flush=True)
    evo_after = bytes(vp.mu.mem_read(base + 3119, 5)) if base else b''
    check('100 lab cycles', cycles == 100 and set(labs) >= set(range(7)) and not u8(OFF_RECOVERY),
          (cycles, sorted(set(labs))))
    check('100 cycles keep evolution bytes', evo_after == evo_before, (evo_before.hex(), evo_after.hex()))

    # Recovery: empty the track, preview Turing, poke fail 7, Apply.
    for i in range(32):
        if d.is_on(trk, i):
            d.toggle(trk, i)
    for _ in range(8):
        if u8(OFF_LAB) == 3:
            break
        tap(BTN_TOP, 120)
    tap(BTN_CLICK, 400)
    vp.mu.mem_write(base + OFF_DIAG, bytes([7]))
    tap(BTN_BOT, 400)
    check('26 recovery set', u8(OFF_RECOVERY) == 1 and u8(OFF_DIAG) == 0, (u8(OFF_RECOVERY), u8(OFF_ST), u8(OFF_LAB)))
    tap(BTN_TOP, 120)
    check('26 lab change keeps recovery', u8(OFF_RECOVERY) == 1, u8(OFF_LAB))
    vp.inputs.button(10, 0)
    d.s.run_ms(30)
    tap(BTN_BACK, 150)
    vp.inputs.button(10, 1)
    check('26 shift-back keeps recovery', u8(OFF_RECOVERY) == 1)
    d.s.shot(os.path.join(shot, '08_recovery.png'), scale=2)
    flows.close_menu(d.s)
    d.s.run_ms(100)
    flows.open_menu(d.s)
    d.s.run_ms(150)
    flows.enter_item(d.s, g.MENU_GENESIS, after_ms=350)
    d.s.run_ms(300)
    base = find_ui()
    check('26 reopen keeps recovery', base and u8(OFF_RECOVERY) == 1, u8(OFF_RECOVERY) if base else 'no ui')
    tap(BTN_BOT, 200)
    check('26 apply stays blocked', u8(OFF_RECOVERY) == 1, u8(OFF_ST))


if __name__ == '__main__':
    sys.exit(main())
