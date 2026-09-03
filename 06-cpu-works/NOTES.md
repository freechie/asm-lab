# 06 — How the CPU executes code

```
make run LESSON=06
make asm LESSON=06
make asm LESSON=06 FUNC=vector_add_autovec
```

## Latency and throughput

An instruction's latency is how many cycles you wait for its result. Throughput is how many of the same instruction the core can finish per cycle when they do not depend on each other.

The dependent add chain in `cpu.c` forces each add to wait on the previous one. The CPU retires about one add per cycle no matter how many add units it has.

Eight independent chains feed eight separate accumulators. The out-of-order core issues several adds in the same cycle, so the cost per add drops below one cycle. The KEEP barriers in the loop limit how much overlap you see, but the gap still shows.

## Branch prediction

clang would turn `if (v >= 128) sum += v` into `csel` or NEON `cmgt`. Then there
is no branch to mispredict. `branch_scan` stores to a `volatile` on the taken
path so the compiler must emit `cmp` and `b.lt`. Dump it and look for those.

Sorted data puts all the taken branches at the end of the array. The hardware predictor learns that pattern quickly and mispredicts rarely.

Random order makes the taken or not-taken outcome hard to guess. The array is
scanned once and is large, so the predictor cannot memorize a short repeating
pattern. Each misprediction drains the pipeline.

## Divide latency

Integer divide on ARM64 is a `udiv` with a much longer latency than `add`. A dependent divide chain exposes that stall directly.

The control benchmark runs the same loop shape with dependent adds so you can compare pipeline depth, not loop overhead.

## SIMD autovectorization

`vector_add_autovec` is a plain element-wise add over float arrays whose length is a multiple of four. At `-O2` clang emits NEON `fadd.4s` that add four floats at once.

Run `make asm LESSON=06 FUNC=vector_add_autovec` and look for `fadd.4s` and `q` registers. If you only see scalar `fadd s0`, try `make asm LESSON=06 OPT=-O3`.

`vector_add_blocked` carries a loop-carried dependency through `acc`. That shape blocks autovectorization and stays scalar on purpose.

## Exercises

1. Compare `add latency, dependent` and `add throughput, 8 independent`. How many cycles per add does each report?
2. How much faster is `branch, data sorted` than `branch, data unsorted` on your machine?
3. Dump `udiv latency, dependent` with `make asm LESSON=06 FUNC=main` and find the inline `udiv` in the timed loop.
4. Contrast the assembly of `vector_add_autovec` and `vector_add_blocked`. What dependency keeps the second one scalar?

## Further reading

Full list with ISBNs: [RESOURCES.md](../RESOURCES.md).

- CS:APP 3e, 5.7–5.14. ISBN 978-0-13-409266-9.
- Bakhvalov 2e. ISBN 979-8-86958-422-9.
- [Why is processing a sorted array faster?](https://stackoverflow.com/questions/11227809/why-is-processing-a-sorted-array-faster-than-processing-an-unsorted-array)
- Dan Luu, [Branch prediction](https://danluu.com/branch-prediction/)
- Slotin, [The cost of branching](https://en.algorithmica.org/hpc/pipelining/branching/)
- Chandler Carruth, [Going Nowhere Faster](https://www.youtube.com/watch?v=2EWejmkKlxs)
- [Dougall Johnson, Apple CPU](https://dougallj.github.io/applecpu/)
