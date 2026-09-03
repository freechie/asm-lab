#!/usr/bin/env python3
"""Lesson 04 - bytecode count and wall time for Python a+b.

The third measurement (machine instructions retired in one PyNumber_Add)
lives in lldb/insn_count.py so a missing debugger does not hide this file's
results.
"""

from __future__ import annotations

import dis
import platform
import subprocess
import sys
import sysconfig
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
C_NS = 0.31  # one add cycle on this chip, same yardstick as lesson 01


def add(a, b):
    return a + b


def best_ns(fn, args, n=1_000_000, trials=7):
    rng = range(n)
    best = float("inf")
    for _ in range(trials):
        t0 = time.perf_counter_ns()
        for _ in rng:
            fn(*args)
        best = min(best, (time.perf_counter_ns() - t0) / n)
    return best


def empty_loop_ns(n=1_000_000, trials=7):
    rng = range(n)
    best = float("inf")
    for _ in range(trials):
        t0 = time.perf_counter_ns()
        for _ in rng:
            pass
        best = min(best, (time.perf_counter_ns() - t0) / n)
    return best


def try_numpy(n=1 << 20):
    try:
        import numpy as np
    except ImportError:
        print("    numpy not installed. skip.")
        return
    a = np.arange(n, dtype=np.int64)
    t0 = time.perf_counter_ns()
    s = int(a.sum())
    ns = (time.perf_counter_ns() - t0) / n
    print(f"    {'numpy.sum 1M int64':<38} {ns:>10.2f} ns/el  checksum {s}")


def try_addmod():
    """Build and import the tiny C extension if the toolchain is present."""
    src = HERE / "addmod.c"
    ext_suffix = sysconfig.get_config_var("EXT_SUFFIX") or ".so"
    dest = HERE / f"addmod{ext_suffix}"
    inc = sysconfig.get_path("include")
    cmd = [
        "clang",
        "-bundle",
        "-undefined",
        "dynamic_lookup",
        f"-I{inc}",
        str(src),
        "-o",
        str(dest),
    ]
    built = subprocess.run(cmd, capture_output=True, text=True)
    if built.returncode != 0:
        print("    C extension did not build:")
        print("   ", (built.stderr or built.stdout).strip().splitlines()[-1:])
        return
    sys.path.insert(0, str(HERE))
    import addmod  # noqa: E402

    ns = best_ns(addmod.add, (2, 3), n=500_000) - empty_loop_ns(n=500_000)
    print(f"    {'C extension addmod.add(2, 3)':<38} {ns:>10.2f} ns/op")


def main():
    print(f"\n=== 04 - Python cost, three ways ===")
    print(f"  interpreter: {sys.executable}")
    print(f"  Python {platform.python_version()} on {platform.machine()}\n")

    print("  1. static bytecode count\n")
    ops = list(dis.get_instructions(add))
    for ins in ops:
        arg = f"  ({ins.argrepr})" if ins.argrepr else ""
        print(f"    {ins.offset:>4}  {ins.opname}{arg}")
    print(f"\n    {len(ops)} bytecode ops. C's add is 1 machine instruction.")
    print("    Bytecode is not a machine instruction. Measurement 3 counts those.\n")

    print("  2. wall time, same units as the C harness\n")
    overhead = empty_loop_ns()
    net = best_ns(add, (2, 3)) - overhead
    big = best_ns(add, (10**18, 10**18)) - overhead
    print(f"    {'net a+b small ints':<38} {net:>10.2f} ns/op")
    print(f"    {'net a+b ~1e18':<38} {big:>10.2f} ns/op")
    print(f"    vs one C add (~{C_NS} ns): {net / C_NS:.0f}x\n")

    n = 1 << 20
    data = list(range(256)) * (n // 256)
    t0 = time.perf_counter_ns()
    s = 0
    for x in data:
        s += x
    ns = (time.perf_counter_ns() - t0) / n
    print(f"    {'Python for-loop sum 1M ints':<38} {ns:>10.2f} ns/el  checksum {s}")

    t0 = time.perf_counter_ns()
    s2 = sum(data)
    ns = (time.perf_counter_ns() - t0) / n
    print(f"    {'Python sum() 1M ints':<38} {ns:>10.2f} ns/el  checksum {s2}")

    try_numpy(n)
    try_addmod()

    print("\n  Escape hatches beat the interpreter. They do it by leaving it.\n")

    sys.path.insert(0, str(HERE / "lldb"))
    import insn_count

    insn_count.main()


if __name__ == "__main__":
    main()
