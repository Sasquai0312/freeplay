#!/usr/bin/env python3
"""Independent Standard MIDI File reader. Checks note-on/note-off balance."""
import struct
import sys
from pathlib import Path


def read_vlq(data, i):
    value = 0
    while True:
        b = data[i]
        i += 1
        value = (value << 7) | (b & 0x7F)
        if not (b & 0x80):
            return value, i


def parse(path):
    data = Path(path).read_bytes()
    if data[:4] != b"MThd":
        raise SystemExit(f"{path}: missing MThd")
    hdr_len = struct.unpack(">I", data[4:8])[0]
    fmt, ntrks, division = struct.unpack(">HHH", data[8:14])
    if fmt != 1 or hdr_len < 6:
        raise SystemExit(f"{path}: expected format 1, got {fmt}")
    i = 8 + hdr_len
    ons = offs = 0
    active = {}
    for _ in range(ntrks):
        if data[i:i + 4] != b"MTrk":
            raise SystemExit(f"{path}: missing MTrk")
        length = struct.unpack(">I", data[i + 4:i + 8])[0]
        i += 8
        end = i + length
        running = None
        while i < end:
            delta, i = read_vlq(data, i)
            status = data[i]
            if status < 0x80:
                if running is None:
                    raise SystemExit(f"{path}: running status without prior status")
                status = running
            else:
                i += 1
                running = status if status < 0xF0 else None
            kind = status & 0xF0
            if kind in (0x80, 0x90):
                note = data[i]
                vel = data[i + 1]
                i += 2
                key = (status & 0x0F, note)
                if kind == 0x90 and vel > 0:
                    ons += 1
                    active[key] = active.get(key, 0) + 1
                else:
                    offs += 1
                    active[key] = active.get(key, 0) - 1
            elif kind in (0xA0, 0xB0, 0xE0):
                i += 2
            elif kind in (0xC0, 0xD0):
                i += 1
            elif status == 0xFF:
                meta = data[i]
                i += 1
                ln, i = read_vlq(data, i)
                i += ln
                running = None
            elif status in (0xF0, 0xF7):
                ln, i = read_vlq(data, i)
                i += ln
            else:
                raise SystemExit(f"{path}: unknown status {status:#x}")
        if i != end:
            raise SystemExit(f"{path}: track length mismatch")
    stuck = {k: v for k, v in active.items() if v != 0}
    if ons != offs or stuck:
        raise SystemExit(f"{path}: unbalanced ons={ons} offs={offs} stuck={stuck}")
    return ons, division


def main(argv):
    paths = [Path(p) for p in argv[1:]]
    if not paths:
        raise SystemExit("usage: check_smf.py file.mid ...")
    for path in paths:
        ons, division = parse(path)
        print(f"{path.name}: note_pairs={ons} division={division}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
