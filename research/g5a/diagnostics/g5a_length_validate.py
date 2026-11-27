#!/usr/bin/env python3
"""G5A native length, preview pages, and Apply readback on the virtual Play+.

Length is changed with the stock Track Length knob before any firmware heap
allocation. Adapter calls allocate, and that leaves the knob inert, so each
destination length gets its own boot.

usage:
  VPLAY_SNAP=/tmp/fp-g5a/PlayPlus_FreePlay_01.ppf.boot.snap \\
    .venv/bin/python research/g5a/diagnostics/g5a_length_validate.py \\
    /tmp/fp-g5a/PlayPlus_FreePlay_01.ppf
"""
from __future__ import annotations

import json
import os
import struct
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
sys.path[:0] = [os.path.join(ROOT, 'cfw', 'vdev'), os.path.join(ROOT, 'cfw', 'tests'),
                os.path.join(ROOT, 'cfw', 'tools')]
from vdev_slice import Device  # noqa: E402
import flows  # noqa: E402

BTN_BACK, BTN_TOP, BTN_BOT, BTN_CLICK, BTN_SHIFT = 7, 0, 3, 6, 10
MIDI = 8
NOTE, VEL, DUR = 6, 0, 1
MENU_GENESIS = 4
FW_STEP_TOGGLE = 0x600de6f1
FW_NEW = 0x600aab65
GN_MAGIC = 0x47454E31
OFF_TRACK, OFF_LEN, OFF_LAB = 7, 8, 5
OFF_ROOT, OFF_SCALE = 11, 12
OFF_ALG, OFF_PULSES, OFF_DIAG = 13, 20, 32
OFF_NATIVE, OFF_SEED = 34, 36


def s32(x):
    x &= 0xFFFFFFFF
    return x - 0x100000000 if x >= 0x80000000 else x


def load_syms(path):
    syms = {}
    for line in open(path):
        parts = line.strip().split()
        if len(parts) == 2 and parts[0].startswith('0x'):
            syms[parts[1]] = int(parts[0], 16)
    return syms


def native_dur(ticks, tps=480):
    return (ticks * 60 + tps // 2) // tps


def load_expected():
    src = os.path.join(os.path.dirname(__file__), 'g5a_expect.c')
    bin_path = '/tmp/g5a_expect'
    subprocess.check_call(['cc', '-std=c11', '-I', os.path.join(ROOT, 'genesis', 'include'),
                           '-o', bin_path, src,
                           os.path.join(ROOT, 'genesis', 'engine', 'event.c'),
                           os.path.join(ROOT, 'genesis', 'engine', 'random.c'),
                           os.path.join(ROOT, 'genesis', 'engine', 'scales.c'),
                           os.path.join(ROOT, 'genesis', 'engine', 'rhythm.c'),
                           os.path.join(ROOT, 'genesis', 'engine', 'melody.c'),
                           os.path.join(ROOT, 'genesis', 'engine', 'arpeggiator.c')],
                          cwd=ROOT)
    rows = {}
    for line in subprocess.check_output([bin_path], text=True).splitlines():
        row = json.loads(line)
        rows[row['name']] = row['events']
    return rows


class Harness:
    def __init__(self, ppf, snap, syms, shot, check):
        self.check = check
        self.shot = shot
        self.syms = syms
        self.d = Device(ppf, snap)
        self.d.s.run_ms(150)
        self.vp = self.d.vp
        self.ctx = self.vp.profile['app'] + 0x4460
        self.bank = False

    def slen(self, t):
        vp = self.vp
        pv = vp.r32(self.d.sel + 0x5c)
        trk = vp.call_fn(self.d.vfn(pv, 0), pv, t)
        return s32(vp.call_fn(self.d.vfn(trk, 4), trk))

    def arm(self):
        vp, d = self.vp, self.d
        if not self.bank:
            vp.inputs.button(BTN_SHIFT, 0)
            d.s.run_ms(25)
            vp.inputs.button(2, 0)
            d.s.run_ms(35)
            vp.inputs.button(2, 1)
            d.s.run_ms(15)
            vp.inputs.button(BTN_SHIFT, 1)
            d.s.run_ms(50)
            self.bank = True
        d.s.pad(0, 19, hold_ms=30, after_ms=50)
        d.s.touch(10, hold_ms=25, after_ms=30)

    def set_len(self, target):
        if not self.bank:
            self.arm()
        else:
            self.d.s.touch(10, hold_ms=25, after_ms=30)
        cur = self.slen(MIDI)
        guard = 0
        while cur != target and guard < 80:
            self.d.s.knob(10, 1 if cur < target else -1, after_ms=40)
            cur = self.slen(MIDI)
            guard += 1
        return cur

    def texts(self):
        return [a[2] for n, a in self.vp.eve.cmd_log if n == 'TEXT' and len(a) > 2]

    def since(self, mark):
        out = []
        for n, a in self.vp.eve.cmd_log[mark:]:
            if n == 'TEXT' and len(a) > 2:
                out.append(a[2])
        return out

    def mark(self):
        return len(self.vp.eve.cmd_log)

    def open_genesis(self):
        # The pattern grid's select mask is what Genesis reads on enter.
        # Bit 8 is the MIDI track whose length the knob just set.
        self.vp.w32(self.d.sel + 0x0c, 1 << MIDI)
        flows.open_menu(self.d.s)
        self.d.s.run_ms(180)
        m = self.mark()
        flows.enter_item(self.d.s, MENU_GENESIS, after_ms=350)
        self.d.s.run_ms(250)
        return self.since(m)

    def shift_turns(self, n):
        vp, d = self.vp, self.d
        vp.inputs.button(BTN_SHIFT, 0)
        d.s.run_ms(25)
        for _ in range(abs(n)):
            d.s.turn(1 if n > 0 else -1, after_ms=45)
        vp.inputs.button(BTN_SHIFT, 1)
        d.s.run_ms(40)

    def turns(self, n):
        for _ in range(abs(n)):
            self.d.s.turn(1 if n > 0 else -1, after_ms=45)

    def focus_dest(self):
        """Point the open panel at MIDI track 8 and its live native length."""
        base = self.ui()
        if not base:
            return 0
        plen = self.slen(MIDI)
        self.poke(base, OFF_TRACK, bytes([MIDI]))
        self.poke(base, OFF_NATIVE, bytes([plen & 0xFF]))
        self.poke(base, OFF_LEN, bytes([16 if plen >= 16 else plen]))
        return base

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
                # st, lab, param, track, len must look like a live panel
                if b[4] <= 3 and b[5] < 7 and b[6] < 46 and b[7] < 16 and 1 <= b[8] <= 64:
                    hits.append(addr)
                start = i + 4
        return hits[0] if hits else 0

    def poke(self, addr, off, data):
        self.vp.mu.mem_write(addr + off, data)

    def clear(self, n):
        for i in range(n):
            if self.d.is_on(MIDI, i):
                self.vp.call_fn(FW_STEP_TOGGLE, self.d.sel, MIDI, i)

    def steps(self, n):
        out = []
        for i in range(n):
            out.append({
                'on': bool(self.d.is_on(MIDI, i)),
                'note': int(self.d.param(MIDI, i, NOTE) or 0),
                'vel': int(self.d.param(MIDI, i, VEL) or 0),
                'dur': int(self.d.param(MIDI, i, DUR) or 0),
            })
        return out

    def matches(self, steps, events):
        want = {e['step']: e for e in events}
        bad = []
        for i, st in enumerate(steps):
            if i in want:
                e = want[i]
                if not (st['on'] and st['note'] == e['note'] and st['vel'] == e['vel']
                        and st['dur'] == native_dur(e['dur'])):
                    bad.append((i, st, e['note'], e['vel']))
            elif st['on']:
                bad.append((i, st, None))
        return bad

    def heap(self, n):
        p = self.vp.call_fn(FW_NEW, n) & 0xFFFFFFFF
        self.vp.mu.mem_write(p, b'\x00' * n)
        return p

    def call(self, name, *args):
        return s32(self.vp.call_fn(self.syms[name], *args))


def main():
    ppf = sys.argv[1]
    snap = os.environ['VPLAY_SNAP']
    link = os.environ.get('LINK_MAP', os.path.join(os.path.dirname(ppf), 'link.map'))
    shot = os.path.join(os.path.dirname(__file__), '..', 'screenshots')
    os.makedirs(shot, exist_ok=True)
    expect = load_expected()
    syms = load_syms(link)
    results = []

    def check(name, ok, detail=None):
        results.append({'name': name, 'pass': bool(ok), 'detail': detail})
        print(('PASS' if ok else 'FAIL'), name, '' if ok else detail, flush=True)

    # ----- 64-step session ---------------------------------------------------
    h = Harness(ppf, snap, syms, shot, check)
    boot = [h.slen(t) for t in range(16)]
    check('boot lengths all 16', boot == [16] * 16, boot)
    got = h.set_len(64)
    check('knob reaches 64', got == 64 and h.slen(9) == 16, {'t8': got, 't9': h.slen(9)})
    h.d.s.shot(os.path.join(shot, 'track_length_64.png'))
    opened = h.open_genesis()
    base = h.focus_dest()
    m = h.mark()
    h.shift_turns(1)  # redraw on the Track row
    h.shift_turns(-1)
    shown = h.since(m)
    check('64 dest shows pat 64 and default len 16',
          '64' in shown and '16' in shown and ('9' in shown or 'trk' in shown), shown[-24:])
    h.d.s.shot(os.path.join(shot, 'config_pat64_len16.png'))
    m = h.mark()
    h.shift_turns(2)
    h.turns(4)  # 16 -> 20, a value the pattern length does not already show
    shown = h.since(m)
    check('encoder raises requested length', 'Phrase' in shown and '20' in shown, shown[-16:])
    check('genesis state found', base != 0, hex(base) if base else 0)
    sample = [bool(h.d.is_on(0, i)) for i in range(16)]

    def run_fixture(name, events, n, pages, **fields):
        if not base:
            check(f'{name} skipped', False, 'no ui')
            return
        for key, off, fmt in (
            ('lab', OFF_LAB, 'B'), ('len', OFF_LEN, 'B'), ('root', OFF_ROOT, 'B'),
            ('scale', OFF_SCALE, 'B'), ('alg', OFF_ALG, 'B'), ('pulses', OFF_PULSES, 'B'),
        ):
            if key in fields:
                h.poke(base, off, struct.pack('<' + fmt, fields[key]))
        if 'seed' in fields:
            h.poke(base, OFF_SEED, struct.pack('<H', fields['seed']))
        m = h.mark()
        h.d.s.tap(BTN_CLICK, after_ms=400)
        first = h.since(m)
        h.d.s.shot(os.path.join(shot, f'preview_{name}_p1.png'))
        check(f'{name} page 1', 'Preview' in first and 'pg' in first and '1' in first, first[-18:])
        for page in range(1, pages):
            m = h.mark()
            h.d.s.turn(1, after_ms=70)
            got = h.since(m)
            h.d.s.shot(os.path.join(shot, f'preview_{name}_p{page + 1}.png'))
            pg = got[got.index('pg') + 1] if 'pg' in got else None
            check(f'{name} page {page + 1}', pg == str(page + 1), (got[:8], got[-6:]))
        m = h.mark()
        h.d.s.tap(BTN_BOT, after_ms=500)
        toast = h.since(m)
        steps = h.steps(n)
        bad = h.matches(steps, events)
        check(f'{name} apply', any('Applied' in x for x in toast), toast[-8:])
        check(f'{name} readback', not bad, {'bad': bad[:3], 'on': sum(s['on'] for s in steps)})
        check(f'{name} sample untouched',
              [bool(h.d.is_on(0, i)) for i in range(16)] == sample)
        h.clear(n)
        check(f'{name} cleared', not any(h.d.is_on(MIDI, i) for i in range(n)))

    run_fixture('walk64', expect['walk64'], 64, 4, lab=0, len=64, alg=1, scale=11, root=57, seed=2026)
    run_fixture('arp64', expect['arp64'], 64, 4, lab=2, len=64, root=60, seed=42)
    # leave the panel before further knob use
    for _ in range(4):
        h.d.s.tap(BTN_BACK, after_ms=120)
    flows.close_menu(h.d.s)
    # Step readback leaves this session's Track Length knob inert.
    # Length is still 64, which the adapter checks below use.
    # A later fresh boot checks that the knob can return the track to 16.
    check('length still 64 after UI readback', h.slen(MIDI) == 64, h.slen(MIDI))

    def boundary(plen):
        ok, detail = True, []
        for step in (0, 15, 16, 31, 32, 63, 64, 127):
            buf = h.heap(8)
            rc = h.call('genesis_midi_read_step', h.ctx, MIDI, step, buf)
            valid = step < plen
            good = (rc == 0) if valid else (rc == -2)
            detail.append({'step': step, 'rc': rc, 'valid': valid})
            ok = ok and good
        return ok, detail

    if h.slen(MIDI) == 64:
        check('boundary read at 64', *boundary(64))
        caps = h.call('genesis_midi_caps') & 0xFFFFFFFF
        check('caps include 16/32/64',
              bool(caps & (1 << 16)) and bool(caps & (1 << 17)) and bool(caps & (1 << 18)), hex(caps))
        check('caps omit 128', (caps & (1 << 19)) == 0, hex(caps))

        def apply_req(n, fail_at, policy=0):
            raw = b''.join(struct.pack('<IBBBBI', i * 480, 60, 90, 0, 0, 240) for i in range(n))
            evp = h.heap(len(raw))
            h.vp.mu.mem_write(evp, raw)
            sn = h.heap(16 + 64 * 8)
            rq = h.heap(36)
            h.vp.mu.mem_write(rq, struct.pack('<IiiIIIiiI',
                                              h.ctx & 0xFFFFFFFF, MIDI, 0, 480, evp, n,
                                              policy, fail_at, sn))
            return h.call('genesis_midi_apply_req', rq)

        h.clear(64)
        rc = apply_req(64, 0, 0)
        on = [i for i in range(64) if h.d.is_on(MIDI, i)]
        check('EMPTY_ONLY 64 writes 0..63', rc == 0 and on == list(range(64)), {'rc': rc, 'n': len(on)})
        rc = apply_req(64, 0, 0)
        check('EMPTY_ONLY 64 rejects occupied', rc == -8, rc)
        rc = apply_req(64, 0, 1)
        check('REPLACE_REGION 64', rc == 0 and all(h.d.is_on(MIDI, i) for i in range(64)), rc)
        h.clear(64)
        before = [bool(h.d.is_on(9, i)) for i in range(16)]
        rc = apply_req(64, 8, 0)
        left = [i for i in range(64) if h.d.is_on(MIDI, i)]
        check('64 page-edge failure restores', rc != 0 and not left, {'rc': rc, 'n': len(left)})
        check('track 9 unchanged by 64 edit', [bool(h.d.is_on(9, i)) for i in range(16)] == before)
        h.clear(64)
        rc = apply_req(64, 7, 0)
        left = [i for i in range(64) if h.d.is_on(MIDI, i)]
        check('64 restore-failure keeps writes', rc == -11 and left, {'rc': rc, 'n': len(left)})
        h.clear(64)

    # ----- 32-step session (fresh boot, knob alive) -------------------------
    h = Harness(ppf, snap, syms, shot, check)
    got = h.set_len(32)
    check('knob reaches 32', got == 32 and h.slen(0) == 16, {'t8': got, 't0': h.slen(0)})
    h.d.s.shot(os.path.join(shot, 'track_length_32.png'))
    opened = h.open_genesis()
    h.focus_dest()
    m = h.mark()
    h.shift_turns(1)
    h.shift_turns(-1)
    shown = h.since(m)
    check('32 dest shows pat 32 and default len 16',
          '32' in shown and '16' in shown, shown[-24:])
    h.d.s.shot(os.path.join(shot, 'config_pat32_len16.png'))
    m = h.mark()
    h.shift_turns(2)
    h.turns(16)
    shown = h.since(m)
    check('encoder sets requested length 32', 'Phrase' in shown and shown.count('32') >= 2, shown[-18:])
    m = h.mark()
    h.d.s.tap(BTN_CLICK, after_ms=400)
    prev = h.since(m)
    h.d.s.shot(os.path.join(shot, 'preview_mel32_p1.png'))
    check('melody 32 page 1/2', 'Preview' in prev and 'pg' in prev and '2' in prev, prev[-18:])
    m = h.mark()
    h.d.s.turn(1, after_ms=70)
    page2 = h.since(m)
    h.d.s.shot(os.path.join(shot, 'preview_mel32_p2.png'))
    pg = page2[page2.index('pg') + 1] if 'pg' in page2 else None
    check('melody encoder page 2', pg == '2', page2[-14:])
    # Apply the full preview, not only the visible page.
    m = h.mark()
    h.d.s.tap(BTN_BOT, after_ms=500)
    toast = h.since(m)
    steps = h.steps(32)
    bad = h.matches(steps, expect['mel32'])
    check('melody 32 apply', any('Applied' in x for x in toast), toast[-8:])
    check('melody 32 readback', not bad, {'bad': bad[:3], 'on': sum(s['on'] for s in steps)})
    sample = [bool(h.d.is_on(0, i)) for i in range(16)]
    check('melody sample track unchanged', sample == [bool(h.d.is_on(0, i)) for i in range(16)])
    # REPLACE with seed 43. Apply returned to config on Length.
    h.shift_turns(1)
    h.turns(1)
    h.d.s.tap(BTN_CLICK, after_ms=400)
    m = h.mark()
    h.d.s.tap(BTN_BOT, after_ms=300)
    prompt = h.since(m)
    check('32 occupied prompts replace', any('Replace' in x for x in prompt), prompt[-10:])
    m = h.mark()
    h.d.s.tap(BTN_BOT, after_ms=500)
    toast = h.since(m)
    steps = h.steps(32)
    bad = h.matches(steps, expect['mel32s43'])
    check('REPLACE 32 readback', not bad and any('Applied' in x for x in toast),
          {'bad': bad[:3], 'toast': toast[-6:]})
    h.clear(32)
    check('32 cleared after replace', not any(h.d.is_on(MIDI, i) for i in range(32)))

    base = h.ui()
    h.poke(base, OFF_LAB, bytes([1]))
    h.poke(base, OFF_LEN, bytes([32]))
    h.poke(base, OFF_PULSES, bytes([11]))
    h.poke(base, OFF_SEED, struct.pack('<H', 42))
    m = h.mark()
    h.d.s.tap(BTN_CLICK, after_ms=400)
    prev = h.since(m)
    h.d.s.shot(os.path.join(shot, 'preview_euc32_p1.png'))
    h.d.s.turn(1, after_ms=70)
    h.d.s.shot(os.path.join(shot, 'preview_euc32_p2.png'))
    m = h.mark()
    h.d.s.tap(BTN_BOT, after_ms=500)
    toast = h.since(m)
    steps = h.steps(32)
    bad = h.matches(steps, expect['euc32'])
    check('euclid 32 preview', 'Preview' in prev or 'hits' in prev, prev[-12:])
    check('euclid 32 readback', not bad and any('Applied' in x for x in toast),
          {'bad': bad[:3], 'on': sum(s['on'] for s in steps)})
    h.clear(32)

    # stale: edit step 20 after a 32-step preview
    h.poke(base, OFF_LAB, bytes([0]))
    h.poke(base, OFF_LEN, bytes([32]))
    h.poke(base, OFF_ALG, bytes([0]))
    h.poke(base, OFF_SEED, struct.pack('<H', 42))
    h.d.s.tap(BTN_CLICK, after_ms=400)
    if not h.d.is_on(MIDI, 20):
        h.vp.call_fn(FW_STEP_TOGGLE, h.d.sel, MIDI, 20)
    m = h.mark()
    h.d.s.tap(BTN_BOT, after_ms=250)
    stale = h.since(m)
    check('stale step-20 edit rejected', any('Stale' in x for x in stale), stale[-10:])
    h.clear(32)

    def apply_req(n, fail_at, policy=0):
        raw = b''.join(struct.pack('<IBBBBI', i * 480, 60, 90, 0, 0, 240) for i in range(n))
        evp = h.heap(len(raw))
        h.vp.mu.mem_write(evp, raw)
        sn = h.heap(16 + 64 * 8)
        rq = h.heap(36)
        h.vp.mu.mem_write(rq, struct.pack('<IiiIIIiiI',
                                          h.ctx & 0xFFFFFFFF, MIDI, 0, 480, evp, n,
                                          policy, fail_at, sn))
        return h.call('genesis_midi_apply_req', rq)

    h.clear(32)
    for stage, name in [(1, 'before_snap'), (3, 'before_write'), (4, 'after_first'),
                        (5, 'mid'), (8, 'page_edge'), (9, 'near_end'), (6, 'before_commit')]:
        h.clear(32)
        rc = apply_req(32, stage)
        left = [i for i in range(32) if h.d.is_on(MIDI, i)]
        check(f'32 fail {name} restores', rc != 0 and not left, {'rc': rc, 'left': left[:6]})
    h.clear(32)
    rc = apply_req(32, 7)
    left = [i for i in range(32) if h.d.is_on(MIDI, i)]
    check('32 restore-failure keeps writes', rc == -11 and left, {'rc': rc, 'n': len(left)})
    h.clear(32)
    rc = apply_req(32, 0, 1)
    check('REPLACE adapter 32', rc == 0 and sum(h.d.is_on(MIDI, i) for i in range(32)) == 32, rc)
    # out of range while length is 32
    buf = h.heap(8)
    check('step 32 rejected', h.call('genesis_midi_read_step', h.ctx, MIDI, 32, buf) == -2)
    check('step 0 accepted', h.call('genesis_midi_read_step', h.ctx, MIDI, 0, buf) == 0)
    ev = h.heap(12)
    h.vp.mu.mem_write(ev, struct.pack('<IBBBBI', 32 * 480, 60, 80, 0, 0, 480))
    prevb = h.heap(0x180)
    rq = h.heap(36)
    h.vp.mu.mem_write(rq, struct.pack('<IiiIIIIBBHI',
                                      h.ctx & 0xFFFFFFFF, MIDI, 0, 480, ev, 1, 42, 1, 0, 0, prevb))
    check('preview rejects step 32', h.call('genesis_midi_preview_req', rq) != 0)
    h.clear(32)

    # Fresh boot: the knob can return a 64-step track to 16 with no adapter calls.
    h = Harness(ppf, snap, syms, shot, check)
    check('fresh climb to 64', h.set_len(64) == 64, h.slen(MIDI))
    check('fresh knob restores 16', h.set_len(16) == 16, h.slen(MIDI))
    buf = h.heap(8)
    rc = h.call('genesis_midi_read_step', h.ctx, MIDI, 16, buf)
    check('step 16 rejected at length 16', rc == -2, {'rc': rc, 'len': h.slen(MIDI)})

    n_pass = sum(1 for r in results if r['pass'])
    n_fail = len(results) - n_pass
    out = {'passed': n_fail == 0, 'n_pass': n_pass, 'n_fail': n_fail, 'n_skip': 0,
           'n_total': len(results), 'results': results}
    dest = os.path.join(os.path.dirname(__file__), '..', 'fixtures', 'g5a_length_results.json')
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    json.dump(out, open(dest, 'w'), indent=2)
    print(f'{n_pass}/{len(results)} passed, {n_fail} failed')
    return 0 if n_fail == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
