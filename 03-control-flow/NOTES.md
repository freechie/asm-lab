# 03 — Control flow and data structures

```
make run LESSON=03
make asm LESSON=03
python3 03-control-flow/step.py
tools/asmdump.sh -r -f jump_table 03-control-flow/control.c
```

## If statements

`abs_if` compares the argument against zero. The compiler emits `cmp` and a conditional branch such as `b.ge` to skip the negation. The taken path uses `neg` or `sub` to form the absolute value. The fall-through path moves the input into `w0` unchanged.

Run `python3 03-control-flow/step.py` after `make run LESSON=03`. The script breaks on `abs_if`, prints mixed disassembly, reads `w0`, single-steps once, and reads `w0` again.

## For loops

`sum_for` is a counted loop. `KEEP(sum)` inside the body stops `-O2` from
replacing it with the closed-form `n*(n-1)/2`. Dump it. You should see an add,
a compare, and a backward branch.

Pointer loops like `scale_array` combine address arithmetic with `ldr`/`str`. The index `i` often disappears at `-O2` and the address register advances by four each trip.

## Struct field access

`struct Point` has two 32-bit fields laid out contiguously. `struct_field` returns `p.y`, which is four bytes after `p.x`. The assembly loads from `[base + #4]` without multiplying, because the offset is known at compile time.

## Switch and jump tables

`jump_table` switches on a dense range `0..7`. Each case only returns a
constant, so clang does not emit an indirect branch. It emits a table of
those constants and one load:

```
ldr w0, [x8, w0, uxtw #2]
```

`w0` is the case value. The table lives in a literal section. Raw dumps keep
that data. The Makefile has no `-r` flag. Use:

```
tools/asmdump.sh -r -f jump_table 03-control-flow/control.c
```

Consecutive return values (100, 101, 102, ...) fold into arithmetic and the
table disappears. That is why the cases here are irregular.

## Exercises

1. Rewrite `min_of_two` from lesson 02 as a ternary in a new function and compare its assembly to the `if` version.
2. Change `jump_table` to use sparse case values like `1`, `10`, `100`. Does the jump table disappear?
3. In lldb, set a breakpoint on `scale_array` and watch how the address register changes across iterations.
4. Add a third field to `struct Point` and re-dump `struct_field`. How does the offset to `y` change?

## Further reading

Full list with ISBNs: [RESOURCES.md](../RESOURCES.md).

- CS:APP 3e, 3.6–3.9. ISBN 978-0-13-409266-9.
- Pyeatt and Ughetta, *ARM 64-Bit Assembly Language*. ISBN 978-0-12-819221-4.
- ARM, [A64 registers](https://developer.arm.com/documentation/102374/0103/Registers-in-AArch64---general-purpose-registers) and the control-flow pages in the same guide
- Matt Godbolt, [What Has My Compiler Done for Me Lately?](https://www.youtube.com/watch?v=bSkpMdDe4g4)
