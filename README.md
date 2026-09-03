# Assembly, CPU, and Performance Lab

A hands-on curriculum for reading ARM64 assembly, watching your M1 Pro
execute it, and measuring why `a + b` costs about 100x more in Python than
in C.

## Setup

You need `clang`, `python3`, `make`, `lldb`, and `otool`. Rosetta 2 is
required only to **run** an x86-64 binary (`make run LESSON=NN ARCH=x86_64`).
Dumping x86-64 assembly does not need it.

```
make list
make run LESSON=00
```

Optional. `PYTHON=/path/to/python3` selects the interpreter the Python
lessons measure. Shell functions and pyenv shims are invisible to make.

## Curriculum

| Lesson | Command | What you learn |
|---|---|---|
| 00 | `make run LESSON=00` | Cache sizes, 128-byte lines, measured clock |
| 01 | `make run LESSON=01` | `a + b` in C, hand-written ARM64, and Python |
| 02 | `make run LESSON=02` | Registers, loads, the calling convention |
| 03 | `make run LESSON=03` | Control flow, structs, lldb single-step |
| 04 | `make run LESSON=04` | Bytecode vs wall time vs instructions retired |
| 05 | `make run LESSON=05` | Why timings lie, and how the harness is built |
| 06 | `make run LESSON=06` | ILP, branch prediction, stalls, NEON |
| 07 | `make run LESSON=07` | L1/L2 knees, 128-byte lines, false sharing |
| 08 | `make run LESSON=08` | The same C, dumped as ARM64 and x86-64 |

Each lesson directory has a `NOTES.md`. Read it after you run the code.

```
make asm LESSON=02          # cleaned ARM64 assembly
make asm LESSON=02 DIFF=1   # -O0 beside -O2
make x86 LESSON=01          # the same source as x86-64
make all                    # every lesson, in order
```

## How it is put together

`make run LESSON=NN` compiles each `.c` file that contains `int main`, links
any `.S` in that directory, runs the binary, then runs every `.py` in the
directory.

`tools/timer.h` is the shared timing harness. Cycles are nanoseconds times a
calibrated clock. Apple Silicon will not let user space read `PMCCNTR_EL0`.

`tools/asmdump.sh` compiles to assembly and drops directives so what remains
is the code the CPU runs.

## Known constraint

The true cycle counter raises `SIGILL` from user space on this chip. Lesson
05 covers `xctrace` and Dougall Johnson's `kperf` work as optional next
steps.

Per-lesson books, articles, and videos are in [RESOURCES.md](RESOURCES.md).
