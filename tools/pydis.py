#!/usr/bin/env python3
"""pydis.py - put Python and C measurements in the same units.

Disassembles a Python function to bytecode, counts the ops, and times it in
nanoseconds per call so the number can sit next to the C figures from timer.h.

    python3 tools/pydis.py                    # demo on a + b
    python3 tools/pydis.py -m mymod -f myfunc
"""

import argparse
import ast
import dis
import importlib
import platform
import statistics
import sys
import time


def loop_overhead_ns(n, trials=7):
    """Cost of the timing loop with no call in it."""
    rng = range(n)
    best = float("inf")
    for _ in range(trials):
        t0 = time.perf_counter_ns()
        for _ in rng:
            pass
        best = min(best, (time.perf_counter_ns() - t0) / n)
    return best


def disassemble(fn):
    """Print bytecode and return the instruction count."""
    instructions = list(dis.get_instructions(fn))
    width = max(len(i.opname) for i in instructions)
    for ins in instructions:
        arg = "" if ins.argrepr == "" else f"  ({ins.argrepr})"
        print(f"    {ins.offset:>4}  {ins.opname:<{width}}{arg}")
    return len(instructions)


def time_ns_per_call(fn, args, target_ns=200_000_000):
    """Time fn(*args), returning the best ns/call over several trials.

    Calibrates the inner repeat count so each trial runs long enough to dwarf
    the clock's ~41 ns granularity, then keeps the minimum: interference can
    only ever make a measurement slower.
    """
    n = 1000
    while True:
        t0 = time.perf_counter_ns()
        for _ in range(n):
            fn(*args)
        elapsed = time.perf_counter_ns() - t0
        if elapsed >= target_ns // 10 or n >= 100_000_000:
            break
        n *= 10

    trials = []
    for _ in range(7):
        t0 = time.perf_counter_ns()
        for _ in range(n):
            fn(*args)
        trials.append((time.perf_counter_ns() - t0) / n)

    # Subtract the empty `for` loop, not a second Python function. Calling an
    # empty `def` can cost more than a specialized `a + b`, which made net
    # work go negative.
    overhead = loop_overhead_ns(n)
    return {
        "n": n,
        "raw_ns": min(trials),
        "overhead_ns": overhead,
        "net_ns": min(trials) - overhead,
        "median_ns": statistics.median(trials),
    }


def report(name, fn, args):
    print(f"\n=== {name} ===\n")
    count = disassemble(fn)
    print(f"\n    bytecode instructions: {count}")

    t = time_ns_per_call(fn, args)
    print(f"    calls timed:           {t['n']:,}")
    print(f"    per call:              {t['raw_ns']:.2f} ns")
    print(f"    empty loop (subtracted): {t['overhead_ns']:.2f} ns")
    print(f"    net work:                {t['net_ns']:.2f} ns")
    return t


def demo():
    print(f"Python {platform.python_version()} on {platform.machine()}")
    print(f"interpreter: {sys.executable}")

    def add(a, b):
        return a + b

    t = report("a + b  (function call included)", add, (2, 3))

    print("\n--- what the C version costs, for scale ---")
    print("    C `return a + b;` compiles to a single `add w0, w0, w1`,")
    print("    which retires in one cycle: about 0.31 ns on this machine.")
    print(f"    ratio (net Python / 0.31 ns C): {t['net_ns'] / 0.31:.0f}x")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-m", "--module", help="module to import")
    ap.add_argument("-f", "--func", help="function within that module")
    ap.add_argument("args", nargs="*", help="literal args, evaluated")
    a = ap.parse_args()

    if not a.module:
        demo()
        return

    sys.path.insert(0, ".")
    mod = importlib.import_module(a.module)
    fn = getattr(mod, a.func)
    report(f"{a.module}.{a.func}", fn, tuple(ast.literal_eval(x) for x in a.args))


if __name__ == "__main__":
    main()
