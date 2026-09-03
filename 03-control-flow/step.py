#!/usr/bin/env python3
"""Run lldb against build/control to inspect abs_if."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "build" / "control"
TIMEOUT = 30


def run_lldb(commands: list[str]) -> subprocess.CompletedProcess[str]:
    cmd = ["lldb", "-b"]
    for line in commands:
        cmd.extend(["-o", line])
    cmd.append(str(BIN))
    return subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        timeout=TIMEOUT,
        check=False,
    )


def main() -> None:
    if not BIN.is_file():
        print("build/control not found. Run: make run LESSON=03", file=sys.stderr)
        sys.exit(1)

    print("=== lldb on abs_if ===\n")

    live = run_lldb(
        [
            "settings set target.process.stop-on-sharedlibrary-events false",
            "b abs_if",
            "run step",
            "disassemble --frame",
            "register read w0",
            "si",
            "register read w0",
            "quit",
        ]
    )
    text = live.stdout + live.stderr
    print(text)

    hit = "stop reason" in text.lower() and "abs_if" in text
    if live.returncode != 0 or not hit:
        print("\n=== fallback: static disassembly of abs_if ===\n")
        static = run_lldb(["disassemble --name abs_if", "quit"])
        print(static.stdout + static.stderr)

    sys.exit(0)


if __name__ == "__main__":
    main()
