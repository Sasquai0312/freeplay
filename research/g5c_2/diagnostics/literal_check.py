#!/usr/bin/env python3
"""Regression for the three historical verifier literals.

Each value is a data constant the Thumb verifier would otherwise treat as a
pointer. The formulas below are the evidence for the allow-list entries.
"""
import os
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'cfw', 'tools'))
from verify import KNOWN_CONSTANTS  # noqa: E402

CFW_EXT_MAX = 0x10000
TILE_TEXT_X = 28
SUB = 16


def vertex2f(x, y):
    return 0x40000000 | ((x & 0x7FFF) << 15) | (y & 0x7FFF)


def main():
    # cfw_ext_ok: n = byte_count/4 <= CFW_EXT_MAX/4 iff byte_count <= 0x10003.
    assert (CFW_EXT_MAX // 4) * 4 + 3 == 0x10003

    # pages.c splits DL_VERTEX2F(TILE_TEXT_X * SUB, 0) into a base plus this addend.
    assert 0x40D005B0 + 0xFFA50 == vertex2f(TILE_TEXT_X * SUB, 0) == 0x40E00000

    # genesis_ui state() packs scale, alg, vmin, vmax. Natural minor is 2.
    packed = struct.unpack('<I', bytes([2, 0, 64, 110]))[0]
    assert packed == 0x6E400002

    for key in (0x10003, 0xFFA50, 0x6E400002):
        assert key in KNOWN_CONSTANTS, hex(key)
        assert KNOWN_CONSTANTS[key]
    print('literal checks passed')
    return 0


if __name__ == '__main__':
    sys.exit(main())
