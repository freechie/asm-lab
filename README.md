# Assembly, CPU, and Performance Lab

A hands-on curriculum for reading ARM64 assembly, watching my M1 Pro
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

## Resources

Read the lesson `NOTES.md` first. The lists below are what to open next.
Buy the North American hardcover of CS:APP. The global paperback uses a
different (and buggy) problem set.

### Must-read books

| Book | ISBN-13 | Why |
|---|---|---|
| Randal E. Bryant and David R. O'Hallaron, *Computer Systems: A Programmer's Perspective*, 3rd ed., Pearson, 2016. **North American hardcover.** | 978-0-13-409266-9 | The textbook this lab is built on. Ch. 3 is assembly. Ch. 5 is optimization. Ch. 6 is the memory hierarchy. |
| Brian W. Kernighan and Dennis M. Ritchie, *The C Programming Language*, 2nd ed., Prentice Hall, 1988. | 978-0-13-110362-7 | Pointers, structs, and what a C `int` actually is. Short. |
| Larry D. Pyeatt and William Ughetta, *ARM 64-Bit Assembly Language*, Newnes, 2019. | 978-0-12-819221-4 | Registers, addressing, NEON. Complements the Apple-focused free book below. |
| Anthony Shaw, *CPython Internals: Your Guide to the Python 3 Interpreter*, Real Python, 2021. | 978-1-77509-334-3 | How bytecode becomes `PyNumber_Add`. Lessons 01 and 04. |
| Denis Bakhvalov, *Performance Analysis and Tuning on Modern CPUs*, 2nd ed., 2024. | 979-8-86958-422-9 | Profiling, pipelining, and how to trust a timer. Also a [free PDF](https://github.com/dendibakh/perf-book). |
| David A. Patterson and John L. Hennessy, *Computer Organization and Design: The Hardware/Software Interface*, ARM Edition, Morgan Kaufmann, 2016. | 978-0-12-801733-3 | How a pipeline, caches, and virtual memory are built. Heavier than CS:APP. |
| Noam Nisan and Shimon Schocken, *The Elements of Computing Systems*, 2nd ed., MIT Press, 2021. | 978-0-262-53980-7 | Gates to a working computer. Optional background if "register" still feels abstract. |

Do not buy CS:APP ISBN 978-1-292-10176-7 (global paperback). The authors warn
that its homework problems are often wrong.

### Must-watch

- [Casey Muratori, Performance-Aware Programming](https://www.computerenhance.com/p/table-of-contents). The mindset for this whole lab. Start with the prologue.
- [Ben Eater, Building an 8-bit breadboard computer](https://www.youtube.com/playlist?list=PLowKtXNTBypGqImE405J2565dvjafglHU). What a CPU is, from clocks and flags up.
- [Matt Godbolt, What Has My Compiler Done for Me Lately? (CppCon 2017)](https://www.youtube.com/watch?v=bSkpMdDe4g4). How to read compiler output. Pair with [godbolt.org](https://godbolt.org).
- [Chandler Carruth, Going Nowhere Faster (CppCon 2017)](https://www.youtube.com/watch?v=2EWejmkKlxs). Loops, `cmov` vs branches, and why a benchmark can lie.
- [Nand2Tetris](https://www.nand2tetris.org). The course that goes with the Nisan/Schocken book.
- CMU 15-213, [Introduction to Computer Systems](https://www.cs.cmu.edu/~213/). The course that goes with CS:APP. Use the current lecture notes and labs.

### Must-read articles

Open these even if you skip the rest. Papers count. Blog posts that earned
their keep count. Marketing posts do not.

| Article | Why |
|---|---|
| Ulrich Drepper, [What Every Programmer Should Know About Memory](https://www.akkadia.org/drepper/cpumemory.pdf) (2007) | Caches, lines, and DRAM. Still the paper for lesson 07. |
| Todd Mytkowicz et al., [Producing Wrong Data Without Doing Anything Obviously Wrong!](https://dl.acm.org/doi/10.1145/1508284.1508275) (ASPLOS 2009). [PDF](https://dijkstra.eecs.umich.edu/kleach/eecs481/w21/readings/producing-wrong-data.pdf) | Link order and environment size can invert a conclusion. Why lesson 05 exists. |
| Igor Ostrovsky, [Gallery of Processor Cache Effects](https://igoro.com/archive/gallery-of-processor-cache-effects/) (2010) | Cache experiments you can run. The ancestor of lesson 07. |
| Dan Luu, [Branch prediction](https://danluu.com/branch-prediction/) | How predictors work and why a mispredict costs ~20 cycles in the cartoon model. |
| Ned Batchelder, [Facts and myths about Python names and values](https://nedbatchelder.com/text/names.html) | Why `a + b` in Python is not the C `add`. Pair with lesson 01. |
| ARM, [Learn the architecture: A64 Instruction Set Architecture](https://developer.arm.com/documentation/102374/0103/Overview) | How to map clang's ARM64 dump back to C. Lookup, not a cover-to-cover read. |
| Andrei Frumusanu, [The 2020 Mac Mini Unleashed: Putting Apple Silicon M1 To The Test](https://www.anandtech.com/show/16252/mac-mini-apple-m1-tested) (AnandTech) | Measured L2, DRAM latency, and single-core bandwidth on this family of chips. |
| Jake Edge, [Faster CPython at PyCon, part one](https://lwn.net/Articles/930705/) (LWN, 2023) | PEP 659 in prose: specializing `BINARY_OP` is why 3.11+ is faster than the bytecode listing. |

### Always-open tools

- [Compiler Explorer](https://godbolt.org). Paste the lesson's C. Switch ARM64 and x86-64.
- [pkivolowitz/asm_book](https://github.com/pkivolowitz/asm_book). Free ARM64 book for Apple Silicon.
- Apple, [Writing ARM64 code for Apple platforms](https://developer.apple.com/documentation/xcode/writing-arm64-code-for-apple-platforms).
- [ARM Architecture Reference Manual, A-profile](https://developer.arm.com/documentation/ddi0487/latest/). Look up one instruction at a time. Do not read it through.
- [Agner Fog, optimization manuals](https://www.agner.org/optimize/). x86-focused. Latency vs throughput and calling conventions transfer.
- Sergey Slotin, [Algorithms for Modern Hardware](https://en.algorithmica.org/hpc/).
- [Dougall Johnson, Apple CPU](https://dougallj.github.io/applecpu/). M1 pipeline facts. `m1cycles` for real counters.
- [Colin Scott, Latency Numbers Every Programmer Should Know](https://colin-scott.github.io/personal_website/research/interactive_latency.html). Orders of magnitude, with a year slider. Confirm against what this machine actually prints.

### By lesson

#### 00 — Know your machine

**Read**
- CS:APP ch. 1 (skim) and ch. 6.1–6.2. What a cache line and a hierarchy are.
- Apple, [Writing ARM64 code for Apple platforms](https://developer.apple.com/documentation/xcode/writing-arm64-code-for-apple-platforms). Page size and ABI notes for this chip.
- [Dougall Johnson, Apple CPU](https://dougallj.github.io/applecpu/). Confirm L1/L2 numbers against what `cpuinfo` printed.

**Articles**
- Frumusanu, [Putting Apple Silicon M1 To The Test](https://www.anandtech.com/show/16252/mac-mini-apple-m1-tested). Cache sizes and DRAM latency for this SoC family.
- [Colin Scott, latency numbers](https://colin-scott.github.io/personal_website/research/interactive_latency.html). L1 vs DRAM as a ratio, not a trivia list.
- Slotin, [Modern Hardware](https://en.algorithmica.org/hpc/complexity/hardware/). Why "count the operations" is the wrong cost model.

**Watch**
- Ben Eater playlist, first 4–5 videos. Clock, registers, and why "one cycle" means something.
- Casey Muratori, [Welcome to Performance-Aware Programming](https://www.computerenhance.com/p/welcome-to-the-performance-aware).

#### 01 — `a + b` in three languages

**Read**
- CS:APP 3.1–3.5. Compiling C to assembly. `add` and `ret`.
- Shaw, *CPython Internals*, chapters on objects and the evaluation loop.
- CPython source: `Python/ceval.c`, `Objects/longobject.c`. Skim `long_add`.
- [PEP 659](https://peps.python.org/pep-0659/) (specializing adaptive interpreter). Why 3.11+ is faster than the bytecode listing.

**Articles**
- Ned Batchelder, [Facts and myths about Python names and values](https://nedbatchelder.com/text/names.html). Names point at objects. C `int` is the value.
- Victor Skvortsov, [How the CPython VM works](https://tenthousandmeters.com/blog/python-behind-the-scenes-1-how-the-cpython-vm-works/). The eval loop that `dis` is listing.
- Victor Skvortsov, [How Python integers work](https://tenthousandmeters.com/blog/python-behind-the-scenes-8-how-python-integers-work/). `PyLongObject` and why `long_add` is not `add w0, w0, w1`.
- LWN, [Faster CPython at PyCon, part one](https://lwn.net/Articles/930705/). What specialization changed after the bytecode you dumped.

**Watch**
- Godbolt, [What Has My Compiler Done for Me Lately?](https://www.youtube.com/watch?v=bSkpMdDe4g4).
- Open the lesson's `add` in [Compiler Explorer](https://godbolt.org) with ARM64 clang `-O2` and x86-64 clang `-O2`.

**Book to own**
- Shaw, *CPython Internals*, ISBN 978-1-77509-334-3.

#### 02 — Reading ARM64

**Read**
- CS:APP 3.4–3.7. Registers, arithmetic, the stack. The book is x86-64. Map `%rdi` to `x0` as you go.
- [pkivolowitz/asm_book](https://github.com/pkivolowitz/asm_book), early chapters.
- Pyeatt and Ughetta, *ARM 64-Bit Assembly Language*, ch. 1–4. ISBN 978-0-12-819221-4.
- Apple, [Writing ARM64 code for Apple platforms](https://developer.apple.com/documentation/xcode/writing-arm64-code-for-apple-platforms). Integer argument registers.
- [AAPCS64](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst). The calling convention, from the spec.

**Articles**
- ARM, [Learn the architecture: A64 Instruction Set Architecture](https://developer.arm.com/documentation/102374/0103/Overview). Instruction format, W vs X, and how assembler maps to C.
- Josh Lospinoso, [ARM64 Is Not Just x64 With More Registers](https://lospino.so/blog/abi-series/arm64-is-not-just-x64/). AAPCS64 vs Apple vs Windows. Why `x18` is off-limits here.

**Watch**
- Casey Muratori, Performance-Aware Programming, Part 1 (Reading ASM).
- Godbolt talk again, now looking at `w0` vs `x0` on ARM64 in Compiler Explorer.

#### 03 — Control flow and data structures

**Read**
- CS:APP 3.6–3.9. Condition codes, loops, `switch`, structs.
- Pyeatt and Ughetta, chapters on branches and memory addressing.
- Apple [LLDB](https://developer.apple.com/documentation/xcode/using-the-terminal-to-debug-your-apps) / `help si` inside lldb.

**Articles**
- ARM A64 guide, [Registers in AArch64](https://developer.arm.com/documentation/102374/0103/Registers-in-AArch64---general-purpose-registers) then the control-flow pages in the same guide. `b.cond`, `cbz`, `csel`.
- Godbolt talk notes in the [CppCon 2017 video](https://www.youtube.com/watch?v=bSkpMdDe4g4): how `-O2` turns `if` and counted loops into compare-and-branch or predication. Watch with the lesson's dump open.

**Watch**
- CS:APP / 15-213 lectures on machine-level control flow (current slides on the [course site](https://www.cs.cmu.edu/~213/)).
- Godbolt talk, the section on how `if` and loops look after `-O2`.

#### 04 — Python's real instruction cost

**Read**
- Shaw, *CPython Internals*. Evaluation loop, `PyNumber_Add`, `PyLongObject`.
- CPython `Python/ceval.c` and `Objects/longobject.c` again, now with the instruction count in hand.
- [PEP 659](https://peps.python.org/pep-0659/).
- Slot in [Algorithms for Modern Hardware](https://en.algorithmica.org/hpc/) on why interpreters are slow: dispatch and type checks, not the add.

**Articles**
- Victor Skvortsov, [How the CPython VM works](https://tenthousandmeters.com/blog/python-behind-the-scenes-1-how-the-cpython-vm-works/) and [How Python integers work](https://tenthousandmeters.com/blog/python-behind-the-scenes-8-how-python-integers-work/). Map the ~330-instruction `PyNumber_Add` count onto these.
- LWN, [Faster CPython at PyCon, part one](https://lwn.net/Articles/930705/). Adaptive opcodes vs the generic `BINARY_OP` path.
- CPython, [InternalDocs/interpreter.md](https://github.com/python/cpython/blob/main/InternalDocs/interpreter.md). The eval loop as the maintainers describe it.

**Watch**
- Real Python / Shaw talks on CPython internals if you prefer video to the book. The book is the must-read.
- Godbolt talk, then contrast: C has no dispatch loop.

**Book to own**
- Shaw, ISBN 978-1-77509-334-3.

#### 05 — Timing pitfalls

**Read**
- CS:APP 5.1–5.3. What "measuring performance" even means.
- Bakhvalov, *Performance Analysis and Tuning on Modern CPUs*, 2nd ed., early chapters on timers, noise, and how to take a minimum. ISBN 979-8-86958-422-9. [Free PDF](https://github.com/dendibakh/perf-book).
- Agner Fog, [Optimizing software in C++](https://www.agner.org/optimize/optimizing_cpp.pdf), the benchmarking section.
- [Dougall Johnson](https://dougallj.github.io/applecpu/) and `m1cycles`. Optional next step after `PMCCNTR_EL0` `SIGILL`.

**Articles**
- Mytkowicz, Diwan, Hauswirth, Sweeney, [Producing Wrong Data Without Doing Anything Obviously Wrong!](https://dl.acm.org/doi/10.1145/1508284.1508275). [PDF](https://dijkstra.eecs.umich.edu/kleach/eecs481/w21/readings/producing-wrong-data.pdf). Measurement bias from layout.
- Denis Bakhvalov, [How to get consistent results when benchmarking on Linux?](https://easyperf.net/blog/2019/08/02/Perf-measurement-environment-on-Linux). Frequency, isolation, statistics. The macOS analogue is: plug in, `make doctor`, take a min.
- Bakhvalov, [Code alignment issues](https://easyperf.net/blog/2018/01/18/Code_alignment_issues). A 32-byte shift can move a number. Same family of bug as Mytkowicz.

**Watch**
- Carruth, [Going Nowhere Faster](https://www.youtube.com/watch?v=2EWejmkKlxs). Compilers deleting your loop.
- Casey Muratori, Part 2 (Basic Profiling).

#### 06 — How the CPU executes code

**Read**
- CS:APP 5.7–5.14. Latency, throughput, branch prediction, SIMD.
- Bakhvalov, pipeline and SIMD chapters. ISBN 979-8-86958-422-9.
- Agner Fog, [The microarchitecture of Intel, AMD and VIA CPUs](https://www.agner.org/optimize/). Concepts transfer. Apple numbers come from Dougall, not Agner.
- [Dougall Johnson, Apple CPU](https://dougallj.github.io/applecpu/). Integer pipes and NEON on M1.
- The classic write-up: [Why is processing a sorted array faster than an unsorted array?](https://stackoverflow.com/questions/11227809/why-is-processing-a-sorted-array-faster-than-processing-an-unsorted-array)

**Articles**
- Dan Luu, [Branch prediction](https://danluu.com/branch-prediction/). Predictors from 2-bit counters up. Read after the sorted/unsorted run.
- Slotin, [Instruction-level parallelism](https://en.algorithmica.org/hpc/pipelining/), then [The cost of branching](https://en.algorithmica.org/hpc/pipelining/branching/) and [Branchless programming](https://en.algorithmica.org/hpc/pipelining/branchless/). The same experiments as this lesson, with graphs.
- Dougall Johnson, [Apple M1: Load and Store Queue Measurements](https://dougallj.wordpress.com/2021/04/08/apple-m1-load-and-store-queue-measurements/). How wide the window actually is on this chip.

**Watch**
- Carruth, [Going Nowhere Faster](https://www.youtube.com/watch?v=2EWejmkKlxs). Branches vs `cmov` / `csel`.
- Casey Muratori, Part 5 (Computation), as it is published.

**Book to own**
- CS:APP ch. 5, ISBN 978-0-13-409266-9.
- Patterson and Hennessy, ARM Edition, ch. 4 (The Processor), ISBN 978-0-12-801733-3, if you want the pipeline drawn out.

#### 07 — Memory hierarchy

**Read**
- CS:APP ch. 6. Entire chapter. Then rerun the chase table.
- Drepper, [What Every Programmer Should Know About Memory](https://www.akkadia.org/drepper/cpumemory.pdf). Sections on caches, lines, and DRAM.
- Slot in [Algorithms for Modern Hardware](https://en.algorithmica.org/hpc/), memory and cache chapters.
- Patterson and Hennessy, ARM Edition, ch. 5. ISBN 978-0-12-801733-3.

**Articles**
- Ostrovsky, [Gallery of Processor Cache Effects](https://igoro.com/archive/gallery-of-processor-cache-effects/). Cache-line stride, associativity, and false sharing as runnable snippets.
- Slotin, [RAM and CPU caches](https://en.algorithmica.org/hpc/cpu-cache/) and [Memory latency](https://en.algorithmica.org/hpc/cpu-cache/latency/). Why a random pointer chase is the measurement, not a sequential scan.
- Herb Sutter, [Eliminate False Sharing](https://herbsutter.com/2009/05/15/effective-concurrency-eliminate-false-sharing/) (Dr. Dobb's, 2009). [Full column](https://jacobfilipp.com/DrDobbs/articles/DDJ/2009/0905/0905ec01/0905ec01.html). Padding vs the 128-byte line this chip actually uses.
- Frumusanu, [M1 tested](https://www.anandtech.com/show/16252/mac-mini-apple-m1-tested). Compare the knees you measured with the published L2 / DRAM numbers.

**Watch**
- Casey Muratori, Part 3 (Moving Data).
- 15-213 lectures on the memory hierarchy.

**Paper, not a book**
- Drepper 2007. No ISBN. It is still the one.

#### 08 — Same code on x86-64

**Read**
- CS:APP ch. 3, now in its native dialect. `%rdi`, `%rsi`, `leal`.
- Agner Fog, [Calling conventions](https://www.agner.org/optimize/) (manual 5).
- Apple, [Writing ARM64 code for Apple platforms](https://developer.apple.com/documentation/xcode/writing-arm64-code-for-apple-platforms), then contrast with System V AMD64.
- [AAPCS64](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst) vs the x86-64 SysV ABI.

**Articles**
- [System V AMD64 ABI](https://gitlab.com/x86-psABIs/x86-64-ABI) (psABI sources; [latest PDF](https://gitlab.com/x86-psABIs/x86-64-ABI/-/jobs/artifacts/master/raw/x86-64-ABI/abi.pdf?job=build)). Argument registers: `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`.
- Apple, [About the Rosetta translation environment](https://developer.apple.com/documentation/apple-silicon/about-the-rosetta-translation-environment). What translates, what does not (AVX-512).
- Koh M. Nakagawa, [Reverse-engineering Rosetta 2, part 1](https://ffri.github.io/ProjectChampollion/part1/). AOT files and the arm64-ized SysV register map. Optional, after you have compared the two dumps.
- Lospinoso, [ARM64 Is Not Just x64 With More Registers](https://lospino.so/blog/abi-series/arm64-is-not-just-x64/). Why the two ABIs are not a rename table.

**Watch**
- Godbolt, [What Has My Compiler Done for Me Lately?](https://www.youtube.com/watch?v=bSkpMdDe4g4), with the compiler set to x86-64 and then ARM64 on the same C.
- Casey Muratori, Part 1, the x86 decoding sections.

### If you only buy three books

1. CS:APP 3e North American hardcover, ISBN **978-0-13-409266-9**.
2. Shaw, *CPython Internals*, ISBN **978-1-77509-334-3**.
3. Bakhvalov 2e, ISBN **979-8-86958-422-9**, or the [free PDF](https://github.com/dendibakh/perf-book) if you do not want paper.

Add Pyeatt/Ughetta (ISBN **978-0-12-819221-4**) if ARM64 assembly is the part that is not sticking.
