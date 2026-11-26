#!/usr/bin/env python3
"""Sum freestanding Thumb .text, .rodata, and .data. Skips unwind and notes."""
import subprocess
import sys
from pathlib import Path


def section_bytes(path):
    out = subprocess.check_output(["llvm-size", "-A", str(path)], text=True)
    size = 0
    for line in out.splitlines():
        parts = line.split()
        if len(parts) < 2 or not parts[1].isdigit():
            continue
        name = parts[0]
        if name.startswith((".ARM", ".comment", ".note", ".llvm")):
            continue
        if name.startswith(".text") or name.startswith(".rodata") or name == ".data":
            size += int(parts[1])
    return size


def main(argv):
    paths = sorted(Path(argv[1] if len(argv) > 1 else "build/thumb").glob("*.o"))
    total = 0
    print("=== Thumb object sizes (-Os, text+rodata+data) ===")
    for path in paths:
        size = section_bytes(path)
        print("%s %d" % (path.name, size))
        total += size
    print("TOTAL_TEXT_RODATA_DATA %d" % total)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
