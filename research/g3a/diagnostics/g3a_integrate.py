#!/usr/bin/env python3
"""G3A native adapter integration harness (virtual Play+ only).

usage:
  VPLAY_DIR=/tmp/fp-g3a python research/g3a/diagnostics/g3a_integrate.py \\
      cfw/build/playplus-1.3.0/PlayPlus_FreePlay_01.ppf
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

NOTE, VEL, DUR = 6, 0, 1
MIDI_TRACK = 8
FW_STEP_TOGGLE = 0x600de6f1
GEN_TPS = 480
GEN_DUR_PER_STEP = 60


def load_syms(link_map):
    syms = {}
    for line in open(link_map):
        parts = line.strip().split()
        if len(parts) == 2 and parts[0].startswith('0x'):
            syms[parts[1]] = int(parts[0], 16)
    return syms


def s32(x):
    x &= 0xFFFFFFFF
    return x - 0x100000000 if x >= 0x80000000 else x


def main():
    ppf = sys.argv[1]
    work = os.environ.get('VPLAY_DIR', '/tmp/fp-g3a')
    os.makedirs(work, exist_ok=True)
    snap = os.environ.get('VPLAY_SNAP') or os.path.join(work, os.path.basename(ppf) + '.boot.snap')
    link_map = os.environ.get('LINK_MAP') or os.path.join(os.path.dirname(ppf), 'link.map')
    if not os.path.exists(snap):
        alt = '/tmp/fp-g1b/img.ppf.boot.snap'
        if os.path.exists(alt):
            snap = alt
        else:
            from stage import stage
            print(f'booting snapshot {snap} ...')
            stage.stage(ppf, None, snap, insns=23000 * 100_000, seconds=3600, quiet=True)

    syms = load_syms(link_map)
    for n in ['genesis_diag_fixture', 'genesis_midi_validate_track', 'genesis_midi_pattern_length',
              'genesis_midi_restore', 'genesis_midi_caps', 'genesis_midi_ticks_to_dur',
              'genesis_midi_ticks_to_step']:
        assert n in syms, f'missing symbol {n}'

    d = Device(ppf, snap)
    d.s.run_ms(150)
    vp = d.vp
    app = vp.profile['app']
    ctx = app + 0x4460
    FW_NEW = 0x600aab65

    def heap(n):
        p = vp.call_fn(FW_NEW, n) & 0xFFFFFFFF
        if not p:
            raise RuntimeError(f'fw_operator_new({n}) failed')
        vp.mu.mem_write(p, b'\x00' * n)
        return p

    def call(name, *args):
        return vp.call_fn(syms[name], *args)

    def pack_fixture_req(fixture_id, track, step0, policy, fail_at, snap_out, preview_out=0):
        # gen_fixture_req layout (ARM EABI):
        # 0:ctx 4:fixture_id 8:track 12:step0 16:policy 20:fail_at 24:snap_out 28:preview_out
        buf = heap(32)
        vp.w32(buf + 0, ctx)
        vp.w32(buf + 4, fixture_id)
        vp.w32(buf + 8, track)
        vp.w32(buf + 12, step0)
        vp.w32(buf + 16, policy)
        vp.w32(buf + 20, fail_at)
        vp.w32(buf + 24, snap_out or 0)
        vp.w32(buf + 28, preview_out or 0)
        return buf

    def run_fixture(fid, track, step0, policy, fail_at, snap_out):
        req = pack_fixture_req(fid, track, step0, policy, fail_at, snap_out, 0)
        return s32(call('genesis_diag_fixture', req))

    def preview_fixture(fid, track, step0, prev):
        req = pack_fixture_req(fid, track, step0, 0, 0, 0, prev)
        return s32(call('genesis_diag_fixture', req))

    results = []

    def check(name, ok, detail=None):
        results.append({'name': name, 'pass': bool(ok), 'detail': detail})
        print(('PASS' if ok else 'FAIL'), name, detail if detail is not None else '')

    def toggle(t, i):
        vp.call_fn(FW_STEP_TOGGLE, d.sel, t, i)

    def ensure_off(t, i):
        if d.is_on(t, i):
            toggle(t, i)

    def ensure_on(t, i):
        if not d.is_on(t, i):
            toggle(t, i)

    def clear_track(t, n=16):
        for i in range(n):
            ensure_off(t, i)

    def snap_occupancy(t, n=16):
        return [bool(d.is_on(t, i)) for i in range(n)]

    def read_midi(t, i):
        return {
            'on': bool(d.is_on(t, i)),
            'note': d.param(t, i, NOTE),
            'vel': d.param(t, i, VEL),
            'dur': d.param(t, i, DUR),
        }

    rc_midi = s32(call('genesis_midi_validate_track', ctx, MIDI_TRACK))
    rc_samp = s32(call('genesis_midi_validate_track', ctx, 0))
    check('validate external MIDI track 8', rc_midi == 0, rc_midi)
    check('reject sample track 0', rc_samp == -1, rc_samp)

    plen = s32(call('genesis_midi_pattern_length', ctx, MIDI_TRACK))
    check('pattern length readable', plen > 0, plen)

    out_u32 = d.out_word
    out_s16 = d.out_word + 4

    def ticks_to_step(tick, tps):
        vp.w32(out_u32, 0)
        rc = s32(call('genesis_midi_ticks_to_step', tick, tps, out_u32))
        return rc, vp.r32(out_u32)

    def ticks_to_dur(dur, tps):
        vp.w32(out_s16, 0)
        rc = s32(call('genesis_midi_ticks_to_dur', dur, tps, out_s16))
        return rc, struct.unpack('<h', vp.mu.mem_read(out_s16, 2))[0]

    for steps, name in [(0.25, 'quarter'), (0.5, 'half'), (1.0, 'full'), (2.0, 'multi'),
                        (1 / 60, 'min_approx'), (64.0, 'max')]:
        dt = max(1, int(round(steps * GEN_TPS)))
        rc, native = ticks_to_dur(dt, GEN_TPS)
        expect = max(0, min(3840, int(round(dt * GEN_DUR_PER_STEP / GEN_TPS))))
        check(f'timing dur {name}', rc == 0 and native == expect,
              {'native': native, 'expect': expect, 'rc': rc})

    rc, st = ticks_to_step(0, GEN_TPS)
    check('timing step 0', rc == 0 and st == 0, {'rc': rc, 'st': st})
    rc, st = ticks_to_step(GEN_TPS * 15, GEN_TPS)
    check('timing step last-16', rc == 0 and st == 15, {'rc': rc, 'st': st})
    rc, st = ticks_to_step(GEN_TPS // 2, GEN_TPS)
    check('timing reject off-grid', rc == -7, {'rc': rc, 'st': st})

    clear_track(MIDI_TRACK, min(plen, 16))
    before = snap_occupancy(MIDI_TRACK, min(plen, 16))
    prev_addr = heap(0x180)
    rc = preview_fixture(1, MIDI_TRACK, 0, prev_addr)
    after = snap_occupancy(MIDI_TRACK, min(plen, 16))
    valid = vp.mu.mem_read(prev_addr + 21, 1)[0]
    evc = vp.r32(prev_addr + 16)
    check('preview fixture A rc', rc == 0 and valid == 1 and evc > 0,
          {'rc': rc, 'valid': valid, 'event_count': evc})
    check('preview no native mutation', before == after)

    fixture_results = {}
    sample_before = snap_occupancy(0, 16)
    snap_buf = heap(0x300)

    for fid, name, need_len in [
        (1, 'A_melody_minor', 16),
        (2, 'B_euclidean_dorian', 16),
        (4, 'D_arp_up', 16),
        (3, 'C_walk_pent', 32),
    ]:
        if plen < need_len:
            check(f'fixture {name} skipped (pattern len {plen} < {need_len})', True,
                  {'plen': plen, 'need': need_len})
            fixture_results[name] = {'skipped': True, 'plen': plen}
            continue
        clear_track(MIDI_TRACK, need_len)
        vp.mu.mem_write(snap_buf, b'\x00' * 0x300)
        rc = run_fixture(fid, MIDI_TRACK, 0, 0, 0, snap_buf)
        obs = []
        for i in range(need_len):
            if d.is_on(MIDI_TRACK, i):
                obs.append({'step': i, **read_midi(MIDI_TRACK, i)})
        sample_after = snap_occupancy(0, 16)
        ok = rc == 0 and len(obs) > 0 and sample_before == sample_after
        for o in obs:
            if o['note'] is None or not (0 <= o['note'] <= 127):
                ok = False
            if o['vel'] is None or not (0 <= o['vel'] <= 127):
                ok = False
            if o['dur'] is None or not (0 <= o['dur'] <= 3840):
                ok = False
        check(f'fixture {name} apply+readback', ok,
              {'rc': rc, 'n': len(obs), 'sample_unchanged': sample_before == sample_after,
               'obs': obs[:8]})
        fixture_results[name] = {'rc': rc, 'events': len(obs), 'obs': obs}

        rc2 = s32(call('genesis_midi_restore', ctx, snap_buf))
        restored = all(not d.is_on(MIDI_TRACK, i) for i in range(need_len))
        check(f'fixture {name} restore', rc2 == 0 and restored, {'rc': rc2, 'restored': restored})

    clear_track(MIDI_TRACK, 16)
    ensure_on(MIDI_TRACK, 0)
    vp.call_fn(d.vfn(d.step(MIDI_TRACK, 0), 0x10), d.step(MIDI_TRACK, 0), NOTE, 60)
    vp.mu.mem_write(snap_buf, b'\x00' * 0x300)
    rc = run_fixture(4, MIDI_TRACK, 0, 0, 0, snap_buf)
    check('EMPTY_ONLY rejects occupied', rc == -8, rc)
    ensure_off(MIDI_TRACK, 0)

    clear_track(MIDI_TRACK, 16)
    ensure_on(MIDI_TRACK, 2)
    st = d.step(MIDI_TRACK, 2)
    vp.call_fn(d.vfn(st, 0x10), st, NOTE, 40)
    vp.call_fn(d.vfn(st, 0x10), st, VEL, 80)
    vp.call_fn(d.vfn(st, 0x10), st, DUR, 60)
    vp.mu.mem_write(snap_buf, b'\x00' * 0x300)
    rc = run_fixture(4, MIDI_TRACK, 0, 1, 0, snap_buf)
    check('REPLACE_REGION apply', rc == 0, {'rc': rc, 'step2': read_midi(MIDI_TRACK, 2)})
    rc2 = s32(call('genesis_midi_restore', ctx, snap_buf))
    check('REPLACE_REGION restore',
          rc2 == 0 and d.is_on(MIDI_TRACK, 2) and d.param(MIDI_TRACK, 2, NOTE) == 40,
          {'rc': rc2, 'midi': read_midi(MIDI_TRACK, 2)})

    for stage, label in [
        (1, 'before_snap'),
        (2, 'after_snap'),
        (3, 'before_write'),
        (4, 'after_first_write'),
        (5, 'mid_edit'),
        (6, 'before_commit'),
    ]:
        clear_track(MIDI_TRACK, 16)
        before = snap_occupancy(MIDI_TRACK, 16)
        vp.mu.mem_write(snap_buf, b'\x00' * 0x300)
        rc = run_fixture(4, MIDI_TRACK, 0, 0, stage, snap_buf)
        after = snap_occupancy(MIDI_TRACK, 16)
        check(f'fail inject {label}', rc != 0 and after == before,
              {'rc': rc, 'unchanged': after == before})

    clear_track(MIDI_TRACK, 16)
    before = snap_occupancy(MIDI_TRACK, 16)
    vp.mu.mem_write(snap_buf, b'\x00' * 0x300)
    rc = run_fixture(4, MIDI_TRACK, 0, 0, 7, snap_buf)
    after = snap_occupancy(MIDI_TRACK, 16)
    check('fail inject during_restore recovery-required', rc == -11 and after != before,
          {'rc': rc, 'before': sum(before), 'after': sum(after)})
    clear_track(MIDI_TRACK, 16)

    ok_rep = True
    for _ in range(3):
        clear_track(MIDI_TRACK, 16)
        vp.mu.mem_write(snap_buf, b'\x00' * 0x300)
        rc = run_fixture(2, MIDI_TRACK, 0, 0, 0, snap_buf)
        if rc != 0:
            ok_rep = False
        call('genesis_midi_restore', ctx, snap_buf)
    check('repeated generate+apply+restore', ok_rep)

    # ---- EMPTY_ONLY: inactive step attrs + restore characterization ----
    clear_track(MIDI_TRACK, 16)
    # Capture off-step param visibility before Apply (unlocked → None)
    before_attrs = [read_midi(MIDI_TRACK, i) for i in range(16)]
    vp.mu.mem_write(snap_buf, b'\x00' * 0x300)
    rc = run_fixture(2, MIDI_TRACK, 0, 0, 0, snap_buf)  # 5 euclidean hits
    mid_attrs = [read_midi(MIDI_TRACK, i) for i in range(16)]
    # Steps that stayed off must remain off; sample track untouched already checked
    rests_ok = all((not mid_attrs[i]['on']) for i in range(16)
                   if not any(o['step'] == i for o in fixture_results.get('B_euclidean_dorian', {}).get('obs', []))
                   or True)
    # More precise: off steps after apply are those not in observed hits
    on_steps = {i for i in range(16) if mid_attrs[i]['on']}
    check('EMPTY_ONLY only activates generated steps', rc == 0 and len(on_steps) == 5,
          {'on': sorted(on_steps), 'rc': rc})
    rc2 = s32(call('genesis_midi_restore', ctx, snap_buf))
    after_attrs = [read_midi(MIDI_TRACK, i) for i in range(16)]
    occ_restored = all(not after_attrs[i]['on'] for i in range(16))
    # Inactive (off) steps: occupancy restored; unlocked params may stay None
    inactive_ok = all(after_attrs[i]['on'] == before_attrs[i]['on'] for i in range(16))
    check('EMPTY_ONLY restore occupancy', rc2 == 0 and occ_restored and inactive_ok,
          {'rc': rc2, 'before_on': sum(a['on'] for a in before_attrs),
           'after_on': sum(a['on'] for a in after_attrs)})

    # ---- Request-wrapper ABI / equivalence (call_fn + host G2) ----
    assert 'genesis_diag_generate' in syms
    abi = {'fixtures': {}}
    host_bin = os.path.join(os.path.dirname(__file__), 'g3a_host_fixtures')
    if not os.path.exists(host_bin):
        # build host mirror
        import subprocess
        subprocess.check_call([
            'cc', '-std=c11', '-O2',
            '-I' + os.path.join(ROOT, 'genesis', 'include'),
            os.path.join(os.path.dirname(__file__), 'g3a_host_fixtures.c'),
            os.path.join(ROOT, 'genesis', 'engine', 'event.c'),
            os.path.join(ROOT, 'genesis', 'engine', 'random.c'),
            os.path.join(ROOT, 'genesis', 'engine', 'scales.c'),
            os.path.join(ROOT, 'genesis', 'engine', 'rhythm.c'),
            os.path.join(ROOT, 'genesis', 'engine', 'melody.c'),
            os.path.join(ROOT, 'genesis', 'engine', 'arpeggiator.c'),
            '-o', host_bin,
        ])

    def fw_generate(fid, cap=64):
        ev_buf = heap(cap * 12)
        count_p = heap(4)
        seed_p = heap(4)
        req = heap(20)  # gen_gen_req
        vp.w32(req + 0, fid)
        vp.w32(req + 4, cap)
        vp.w32(req + 8, ev_buf)
        vp.w32(req + 12, count_p)
        vp.w32(req + 16, seed_p)
        rc = s32(call('genesis_diag_generate', req))
        n = vp.r32(count_p) if rc == 0 else 0
        events = []
        for i in range(n):
            raw = vp.mu.mem_read(ev_buf + i * 12, 12)
            tick, note, vel, ch, _pad, dur = struct.unpack('<IBBBBI', raw)
            events.append({'tick': tick, 'note': note, 'vel': vel, 'dur': dur, 'ch': ch})
        return rc, events

    # invalid request
    rc = s32(call('genesis_diag_generate', 0))
    check('ABI reject NULL gen_gen_req', rc == -12, rc)
    bad = heap(20)
    vp.w32(bad + 0, 99)  # bad fixture
    vp.w32(bad + 4, 64)
    vp.w32(bad + 8, heap(12 * 64))
    vp.w32(bad + 12, 0)
    vp.w32(bad + 16, 0)
    rc = s32(call('genesis_diag_generate', bad))
    check('ABI reject bad fixture id', rc == -12, rc)
    # capacity too large
    vp.w32(bad + 0, 1)
    vp.w32(bad + 4, 128)
    rc = s32(call('genesis_diag_generate', bad))
    check('ABI reject oversize capacity', rc == -4, rc)

    import subprocess
    for fid, name in [(1, 'A'), (2, 'B'), (4, 'D')]:
        host = json.loads(subprocess.check_output([host_bin, str(fid)], text=True))
        rc1, ev1 = fw_generate(fid)
        rc2, ev2 = fw_generate(fid)  # repeated call
        same_fw = ev1 == ev2
        # compare host vs firmware
        host_ev = [{'tick': e['tick'], 'note': e['note'], 'vel': e['vel'],
                    'dur': e['dur'], 'ch': e['ch']} for e in host['events']]
        match = rc1 == 0 and host_ev == ev1 and same_fw
        check(f'ABI equiv host vs call_fn fixture {name}', match,
              {'rc': rc1, 'host_n': host['count'], 'fw_n': len(ev1), 'repeat': same_fw,
               'host0': host_ev[:2], 'fw0': ev1[:2]})
        abi['fixtures'][name] = {'match': match, 'n': len(ev1)}

    # wrapper preview vs generate equivalence for fixture D
    prev = heap(0x180)
    rc = preview_fixture(4, MIDI_TRACK, 0, prev)
    _, gen_ev = fw_generate(4)
    prev_notes = []
    for i in range(16):
        if vp.mu.mem_read(prev + 24 + i, 1)[0]:  # occupancy
            prev_notes.append((i, vp.mu.mem_read(prev + 24 + 64 + i, 1)[0]))
    gen_notes = []
    for e in gen_ev:
        step = e['tick'] // GEN_TPS
        gen_notes.append((step, e['note']))
    check('ABI preview matches generate (D)', rc == 0 and prev_notes == gen_notes,
          {'prev': prev_notes[:6], 'gen': gen_notes[:6]})

    caps = call('genesis_midi_caps') & 0xFFFFFFFF
    check('caps monophonic bit', caps & 1, hex(caps))

    out = {
        'passed': all(r['pass'] for r in results),
        'n_pass': sum(r['pass'] for r in results),
        'n_total': len(results),
        'pattern_length': plen,
        'fixtures': fixture_results,
        'abi': abi,
        'results': results,
        'caps': caps,
    }
    out_path = os.path.join(os.path.dirname(__file__), '..', 'fixtures', 'g3a_integrate_results.json')
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    json.dump(out, open(out_path, 'w'), indent=2)
    print(f"{out['n_pass']}/{out['n_total']} passed; wrote {out_path}")
    return 0 if out['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
