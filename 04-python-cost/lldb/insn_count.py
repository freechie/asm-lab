#!/usr/bin/env python3
"""Count machine instructions in one PyNumber_Add via lldb.

Imported by count.py. Not a Makefile driver (this file lives under lldb/).
"""

from __future__ import annotations

import shutil
import subprocess
import sys
import sysconfig
from pathlib import Path

HERE = Path(__file__).resolve().parent
HELPER = HERE / "count_until_return.py"


def _file_kind(path: Path) -> str:
    r = subprocess.run(["file", "-b", str(path)], capture_output=True, text=True)
    return r.stdout.lower()


def has_symbol(binary: Path, name: str = "_PyNumber_Add") -> bool:
    if not binary.is_file():
        return False
    r = subprocess.run(
        ["nm", "-gU", str(binary)], capture_output=True, text=True
    )
    return name in r.stdout


def is_launchable(path: Path) -> bool:
    """A dylib holds PyNumber_Add but lldb cannot `run` it."""
    kind = _file_kind(path)
    return "executable" in kind and "library" not in kind


def candidates() -> list[Path]:
    cellar = Path("/opt/homebrew/opt/python@3.14/Frameworks/Python.framework")
    out = [
        cellar / "Versions/3.14/Resources/Python.app/Contents/MacOS/Python",
        cellar / "Versions/Current/Resources/Python.app/Contents/MacOS/Python",
        Path(sys.executable).resolve(),
    ]
    prefix = sysconfig.get_config_var("PYTHONFRAMEWORKPREFIX")
    if prefix:
        fw = Path(prefix) / "Python.framework/Versions/Current"
        out.append(fw / "Resources/Python.app/Contents/MacOS/Python")
    py = shutil.which("python3")
    if py:
        out.append(Path(py).resolve())
    seen: set[Path] = set()
    uniq: list[Path] = []
    for p in out:
        if p in seen:
            continue
        seen.add(p)
        uniq.append(p)
    return uniq


def nearby_python_dylibs(exe: Path) -> list[Path]:
    found = []
    for parent in exe.parents:
        cand = parent / "Python"
        if cand.is_file() and cand != exe:
            found.append(cand)
        if parent.name.endswith(".framework"):
            break
    return found


def pick_interpreter() -> Path | None:
    for p in candidates():
        if not p.is_file() or not is_launchable(p):
            continue
        if has_symbol(p) or any(has_symbol(d) for d in nearby_python_dylibs(p)):
            return p
    return None


def main() -> int:
    print("\n=== 04 - machine instructions in one PyNumber_Add ===\n")
    interp = pick_interpreter()
    if interp is None:
        print("  No launchable Python with a PyNumber_Add symbol was found.")
        print("  Homebrew python@3.14's Python.app binary is the usual fix.")
        print("  Bytecode and wall-time measurements above still stand.\n")
        return 0

    lldb = shutil.which("lldb")
    if not lldb:
        print("  lldb not on PATH. skip instruction count.\n")
        return 0

    print(f"  interpreter: {interp}")
    cmd = [
        lldb,
        "-b",
        "-o",
        f"command script import {HELPER}",
        "-o",
        "breakpoint set --name PyNumber_Add",
        "-o",
        "run -c 'a=2;b=3;a+b'",
        "-o",
        "count_until_return",
        "--",
        str(interp),
    ]
    print("  stepping one PyNumber_Add. a few seconds.\n")
    proc = subprocess.run(cmd, capture_output=True, text=True)
    text = proc.stdout + proc.stderr
    n = None
    for line in text.splitlines():
        if line.startswith("INSTRUCTIONS "):
            n = int(line.split()[1])
    if n is None:
        print("  Could not single-step this interpreter (attach denied or exec stub).")
        print("  Measurements 1 and 2 still stand. Last lldb lines:")
        for line in text.strip().splitlines()[-8:]:
            print("   ", line)
        print()
        return 0

    print(f"    PyNumber_Add(2, 3) retired {n} machine instructions")
    print("    C `return a + b;` retires 1 (`add w0, w0, w1`).")
    print(f"    ratio: {n}x in instruction count, not in wall time.")
    print("    Wall time is lower because the CPU overlaps work, and 3.11+")
    print("    specializes the bytecode so later calls skip some of this path.\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
