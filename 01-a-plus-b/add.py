#!/usr/bin/env python3
"""Lesson 01 - what `a + b` costs in Python.

Run the C side first (`make run LESSON=01`), then this, and put the two
ns/op numbers next to each other.
"""

import dis
import platform
import sys
import time


def add(a, b):
    return a + b


def best_ns_per_call(fn, args, n=2_000_000, trials=7):
    """Fastest of several trials, in nanoseconds per call."""
    rng = range(n)
    best = float("inf")
    for _ in range(trials):
        t0 = time.perf_counter_ns()
        for _ in rng:
            fn(*args)
        best = min(best, (time.perf_counter_ns() - t0) / n)
    return best


def loop_overhead_ns(n=2_000_000, trials=7):
    """The cost of the timing loop with no call in it, so we can subtract it."""
    rng = range(n)
    best = float("inf")
    for _ in range(trials):
        t0 = time.perf_counter_ns()
        for _ in rng:
            pass
        best = min(best, (time.perf_counter_ns() - t0) / n)
    return best


def main():
    print(f"\n=== 01 - a + b in Python {platform.python_version()} "
          f"on {platform.machine()} ===\n")
    print(f"  interpreter: {sys.executable}\n")

    print("  The bytecode CPython compiles `return a + b` into:\n")
    instructions = list(dis.get_instructions(add))
    for ins in instructions:
        arg = f"  ({ins.argrepr})" if ins.argrepr else ""
        print(f"    {ins.offset:>4}  {ins.opname}{arg}")

    n_ops = len(instructions)
    print(f"\n    {n_ops} bytecode instructions, of which exactly one "
          f"(a BINARY_OP) is the addition.")
    print("    The exact listing shifts between Python versions: newer ones")
    print("    fuse common pairs into superinstructions, so you may see one")
    print("    LOAD_FAST_BORROW_LOAD_FAST_BORROW where 3.12 showed two loads.")

    print("\n  cost\n")
    call = best_ns_per_call(add, (2, 3))
    overhead = loop_overhead_ns()
    net = call - overhead
    print(f"    {'a + b through a call':<38} {call:>10.2f} ns/op")
    print(f"    {'empty loop (subtracted below)':<38} {overhead:>10.2f} ns/op")
    print(f"    {'net cost of the call + add':<38} {net:>10.2f} ns/op")

    # Integers from -5 to 256 are preallocated singletons in CPython, so
    # small-number arithmetic skips the allocator. Larger values do not, and
    # the difference is visible.
    big = best_ns_per_call(add, (10**18, 10**18)) - overhead
    huge = best_ns_per_call(add, (10**40, 10**40)) - overhead
    print(f"    {'net, operands ~1e18 (allocates)':<38} {big:>10.2f} ns/op")
    print(f"    {'net, operands ~1e40 (multi-digit)':<38} {huge:>10.2f} ns/op")

    cycle_ns = 0.31
    print(f"\n  A C `add` is one instruction, about {cycle_ns} ns on this chip.")
    print(f"  Python's addition of two small ints costs roughly "
          f"{net / cycle_ns:.0f}x that.\n")

    print("  Where the time goes. `a + b` in C is one instruction because the")
    print("  compiler knew at compile time that both operands were 32-bit")
    print("  integers in registers. CPython knows none of that until runtime,")
    print("  so every addition must:")
    print(f"    1. fetch and dispatch {n_ops} bytecodes through the interpreter loop")
    print("    2. look up the + implementation on the operands' types")
    print("    3. check both objects really are integers")
    print("    4. unbox the machine integers out of the PyLongObject headers")
    print("    5. do the add, and check for overflow into a bigger integer")
    print("    6. allocate a new PyLongObject to hold the result")
    print("    7. adjust reference counts on everything involved")
    print("  Step 5 is the C program's entire workload. The rest is the tax")
    print("  for not knowing the types ahead of time.\n")

    print("  Read the source of steps 2-7 yourself:")
    print("    CPython Python/ceval.c        the dispatch loop")
    print("    CPython Objects/longobject.c  long_add and x_add")
    print("  Lesson 04 counts the machine instructions all of this takes.\n")

    if sys.implementation.name == "cpython" and sys.version_info >= (3, 11):
        print("  Note: 3.11+ specializes hot bytecode at runtime. After this")
        print("  loop warms up, BINARY_OP quietly rewrites itself into")
        print("  BINARY_OP_ADD_INT, which skips steps 2 and 3. That is why the")
        print("  measured cost is lower than the bytecode listing suggests.\n")


if __name__ == "__main__":
    main()
