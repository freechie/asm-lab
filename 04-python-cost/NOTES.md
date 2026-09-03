# 04 — Counting Python's real instruction cost

```
make run LESSON=04
```

Three measurements of the same claim. Each is honest about a different thing.

## 1. Bytecode count

`return a + b` compiles to a handful of bytecodes. One of them is the add.
C compiles the same function to one machine instruction. Bytecode is not a
machine instruction. This count is the interpreter's recipe, not the CPU's.

## 2. Wall time

Lesson 01 already timed this. Small-int `a + b` is tens of nanoseconds in
Python and about 0.3 ns in C. The ratio is about 100x. Large integers cost
more because they are heap-allocated.

A Python `for` loop summing a million ints is tens of nanoseconds per element.
The same loop in C is a fraction of a nanosecond. `sum()`, numpy, and a C
extension are faster because they leave the interpreter for the inner loop.

## 3. Machine instructions retired

`count.py` breaks on `PyNumber_Add` under lldb and single-steps until that
frame returns. On this machine that is a few hundred instructions for `2 + 3`.
The C add is one.

This is the number the "~100 more instructions" claim was pointing at. It is
not 100 exactly. It is hundreds, and it moves with the Python version.

The script looks for a framework Python that still has the `PyNumber_Add`
symbol. Homebrew `python@3.14` works. A stripped stub on PATH often does not.
If the symbol is missing, measurements 1 and 2 still stand.

## Escape hatches

numpy and `addmod.c` do the add in C. They are not "faster Python". They are
Python calling C. That is the point.

## Exercises

1. Change `a=2;b=3;a+b` in the lldb run to `a='x';b='y';a+b`. Does
   `PyNumber_Add` still hit, and is the instruction count larger?
2. Compare `make run LESSON=04 PYTHON=/opt/homebrew/bin/python3` against a
   pyenv 3.12. Read the `interpreter:` line before you compare numbers.

## Further reading

Full list with ISBNs: [README.md](../README.md#resources).

- Anthony Shaw, *CPython Internals*. ISBN 978-1-77509-334-3.
- Victor Skvortsov, [How the CPython VM works](https://tenthousandmeters.com/blog/python-behind-the-scenes-1-how-the-cpython-vm-works/), [How Python integers work](https://tenthousandmeters.com/blog/python-behind-the-scenes-8-how-python-integers-work/)
- Jake Edge, [Faster CPython at PyCon, part one](https://lwn.net/Articles/930705/)
- [PEP 659](https://peps.python.org/pep-0659/)
- CPython `Python/ceval.c`, `Objects/longobject.c`
- [Algorithms for Modern Hardware](https://en.algorithmica.org/hpc/)
