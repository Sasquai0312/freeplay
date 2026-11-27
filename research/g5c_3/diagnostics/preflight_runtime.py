#!/usr/bin/env python3
"""Heap lifetime, emulator stack sample, and recovery-lock checks for Alpha 1.

Boots nothing. Point VPLAY_SNAP at a snapshot of this image and use a private
VPLAY_DIR so the card image is not shared with another suite.

  VPLAY_SNAP=/tmp/fp-g5c3/PlayPlus_FreePlay_01.ppf.boot.snap \\
  VPLAY_DIR=/tmp/fp-g5c3-rt \\
    .venv/bin/python research/g5c_3/diagnostics/preflight_runtime.py \\
    /tmp/fp-g5c3/PlayPlus_FreePlay_01.ppf
"""
from __future__ import annotations

import os
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
sys.path[:0] = [os.path.join(ROOT, 'cfw', 'vdev'), os.path.join(ROOT, 'cfw', 'tests'),
                os.path.join(ROOT, 'research', 'g5c_2', 'diagnostics')]
from unicorn import UC_HOOK_CODE, arm_const as A  # noqa: E402
from g5c2_interact import (  # noqa: E402
    BTN_BACK, BTN_BOT, BTN_CLICK, BTN_TOP, H, OFF_DIAG, OFF_EVO_MODE, OFF_NEV,
    OFF_RECOVERY, OFF_ST, WMODE_GENESIS,
)

EXT_LO = 0x60190B00
CAVE_LO = 0x6018CA40
LOCK_DEPTH = 0x2001DAF4
RESET_SP = 0x20060000
UI_SIZE = 0xC80  # 3200: G6B appends chord-lab fields after gen_evo
FW_NEW = 0x600AAB64
LAB_TURING, LAB_MARKOV, LAB_MUTATE = 3, 4, 5


def main():
    ppf = sys.argv[1]
    snap = os.environ['VPLAY_SNAP']
    results = []

    def check(name, ok, detail=''):
        results.append(bool(ok))
        print(f"{'PASS' if ok else 'FAIL'} {name}" + (f': {detail}' if detail else ''), flush=True)

    syms = {}
    map_path = os.path.join(os.path.dirname(ppf), 'link.map')
    for line in open(map_path):
        parts = line.split()
        if len(parts) >= 2 and parts[0].startswith('0x'):
            syms[parts[-1]] = int(parts[0], 16)
    do_generate = syms['do_generate']
    h = H(ppf, snap)
    h.arm()
    magic, ext_len = struct.unpack('<II', bytes(h.vp.mu.mem_read(EXT_LO, 8)))
    ext_hi = EXT_LO + ext_len
    check('extension header', magic == 0x58574643 and EXT_LO < ext_hi <= 0x601A0B00,
          f'{magic:#x} len {ext_len}')
    allocs = []

    def on_new(vp):
        allocs.append(vp.mu.reg_read(A.UC_ARM_REG_R0) & 0xFFFFFFFF)

    h.vp.hook_addr(FW_NEW, on_new)
    h.open_genesis()
    h.use_midi()
    for _ in range(8):
        if h.u8(5) == LAB_TURING:
            break
        h.tap(BTN_TOP, 100)
    h.ui()
    ptr = h.base
    ui_allocs = [n for n in allocs if n == UI_SIZE]
    check('one UI allocation on first open', len(ui_allocs) == 1 and ptr != 0,
          f'ptr {ptr:#x} sizes {allocs}')

    def ensure_lab(lab):
        """Re-enter Genesis and land on one of the three Evolution labs."""
        if h.wmode() != WMODE_GENESIS:
            h.d.s.tap(0, after_ms=180)
            h.d.s.run_ms(80)
        if h.wmode() != WMODE_GENESIS:
            h.open_genesis()
            h.use_midi()
        h.ui()
        return h.wmode() == WMODE_GENESIS and h.goto_lab(lab)

    def one_cycle(mode):
        lab = LAB_TURING + mode
        if not ensure_lab(lab) or h.u8(OFF_EVO_MODE) != mode:
            return False
        before = h.notes(16)
        h.tap(BTN_CLICK, 160)
        preview = (h.u8(5) == lab and h.u8(OFF_EVO_MODE) == mode and h.u8(OFF_ST) == 1 and
                   h.u8(OFF_NEV) > 0 and h.notes(16) == before)
        if not preview:
            print(f'preview fail mode {mode} st={h.u8(OFF_ST)} nev={h.u8(OFF_NEV)} '
                  f'lab={h.u8(5)} eng={h.u8(OFF_EVO_MODE)} trk={h.u8(7)}', flush=True)
        h.tap(BTN_BACK, 80)
        cancelled = h.wmode() == WMODE_GENESIS and h.u8(OFF_ST) == 0 and h.notes(16) == before
        h.tap(BTN_BACK, 100)
        left = h.wmode() != WMODE_GENESIS
        return preview and cancelled and left

    def run_mode(mode, n):
        if not ensure_lab(LAB_TURING + mode):
            print(f'engine {mode} not reached', flush=True)
            return 0
        ok = 0
        for i in range(n):
            if not one_cycle(mode):
                print(f'cycle fail mode {mode} i {i}', flush=True)
                break
            ok += 1
        return ok

    n0 = run_mode(0, 34)
    n1 = run_mode(1, 33)
    if not ensure_lab(LAB_MUTATE):
        n2 = 0
    else:
        h.use_midi()
        h.fill(8)
        n2 = run_mode(2, 33)
    check('100 open generate cancel close', n0 + n1 + n2 == 100, f'{n0}+{n1}+{n2}')
    h.ui()
    ui_allocs = [n for n in allocs if n == UI_SIZE]
    check('UI pointer stable', h.base == ptr, f'{h.base:#x} vs {ptr:#x}')
    check('no second UI object', len(ui_allocs) == 1, str(ui_allocs))
    check('object still magic after cycles', h.u8(0) == 0x31)

    def sample(label):
        if h.wmode() != WMODE_GENESIS:
            check(label, False, 'not open')
            return
        seen = {'min': 0xFFFFFFFF, 'at': 0, 'entry': 0}

        def on_code(mu, addr, size, _):
            sp = mu.reg_read(A.UC_ARM_REG_SP)
            if sp < seen['min']:
                seen['min'] = sp
                seen['at'] = addr

        def on_gen(vp):
            seen['entry'] = vp.mu.reg_read(A.UC_ARM_REG_SP)

        hk = h.vp.mu.hook_add(UC_HOOK_CODE, on_code, begin=CAVE_LO, end=ext_hi - 1)
        h.vp.hook_addr(do_generate, on_gen)
        h.tap(BTN_CLICK, 200)
        h.vp.mu.hook_del(hk)
        drop = (seen['entry'] - seen['min']) if seen['entry'] and seen['min'] < seen['entry'] else 0
        below_reset = RESET_SP - seen['min'] if seen['min'] < RESET_SP else 0
        check(label, seen['entry'] != 0 and seen['min'] > 0x20010000,
              f'entry {seen["entry"]:#x} min {seen["min"]:#x} drop {drop} '
              f'below_reset {below_reset} at {seen["at"]:#x}')
        h.tap(BTN_BACK, 80)

    if not ensure_lab(LAB_TURING):
        check('turing sample mode', False)
    sample('stack sample turing preview')
    h.use_midi()
    h.fill(8)
    check('mutation mode for stack sample', ensure_lab(LAB_MUTATE), str(h.u8(5)))
    sample('stack sample mutation preview')

    # Stage 7 on an empty Turing region. Occupied regions return before fail index 7.
    if not ensure_lab(LAB_TURING):
        check('restore probe mode', False)
    else:
        h.use_midi()
        h.clear(h.u8(8) if 1 <= h.u8(8) <= 64 else 16)
        h.tap(BTN_CLICK, 220)
        check('preview before restore probe', h.u8(OFF_ST) == 1, str(h.u8(OFF_ST)))
        depth_before = struct.unpack('<i', bytes(h.vp.mu.mem_read(LOCK_DEPTH, 4)))[0]
        h.poke(OFF_DIAG, bytes([7]))
        h.tap(BTN_BOT, 400)
        depth_after = struct.unpack('<i', bytes(h.vp.mu.mem_read(LOCK_DEPTH, 4)))[0]
        check('edit lock released after restore error', depth_before == 0 and depth_after == 0,
              f'{depth_before} -> {depth_after}')
        check('diag poke cleared only inside apply', h.u8(OFF_DIAG) == 0 and h.u8(OFF_RECOVERY) == 1,
              f'diag {h.u8(OFF_DIAG)} recovery {h.u8(OFF_RECOVERY)}')

    failed = len(results) - sum(results)
    print(f'TOTAL {len(results)} FAIL {failed}', flush=True)
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
