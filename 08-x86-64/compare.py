#!/usr/bin/env python3
"""Side-by-side add_called dumps for ARM64 and x86-64."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
ASMDUMP = REPO / "tools" / "asmdump.sh"
ADD_C = REPO / "01-a-plus-b" / "add.c"


def dump(arch: str) -> str:
    cmd = [str(ASMDUMP), "-f", "add_called", "-a", arch, str(ADD_C)]
    proc = subprocess.run(cmd, cwd=REPO, capture_output=True, text=True, check=False)
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr)
        raise SystemExit(proc.returncode)
    return proc.stdout


def main() -> int:
    arm = dump("arm64")
    x86 = dump("x86_64")

    print("=== ARM64 (add_called) ===")
    print(arm, end="" if arm.endswith("\n") else "\n")
    print()
    print("=== x86-64 (add_called) ===")
    print(x86, end="" if x86.endswith("\n") else "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
