# 05 — Timing pitfalls

```
make run LESSON=05
```

## Why KEEP belongs inside the loop

The optimizer can prove that adding 1 exactly N times equals adding N once.
If nothing inside the loop escapes to memory or a register barrier, the whole
loop folds away and the benchmark reports 0.00 ns/op.
`KEEP` after the loop is too late. The fold already happened.

## Two different clocks

`cntvct_el0` ticks at 24 MHz through `CLOCK_UPTIME_RAW`.
One tick is about 41 ns no matter how many digits the timer prints.
The measured core clock is about 3.2 GHz, so one cycle is about 0.31 ns.
Nanoseconds come from the 24 MHz timer. Cycles multiply ns by the measured GHz.

## Why we take the minimum

A benchmark can only be slowed by interrupts, cache misses, or scheduling.
It cannot run faster than the hardware allows.
The fastest trial is the least polluted estimate of true cost.
The mean includes every slow run and reads high.

## No hardware cycle counter in user space

Reading `PMCCNTR_EL0` from a normal program raises `SIGILL` on Apple Silicon.
Cycles in this lab are always nanoseconds times measured GHz, never a direct count.
For real cycle data use `xctrace` or Dougall Johnson's kperf tools.
See [Apple CPU research](https://dougallj.github.io/applecpu/).

## What skews your numbers

Running on battery lets macOS downclock the core.
Landing on an efficiency core makes the same code roughly 2-3x slower.
`asmlab_prefer_p_core()` asks for a performance core but cannot guarantee one.

## Further reading

Full list with ISBNs: [RESOURCES.md](../RESOURCES.md).

- CS:APP 3e, 5.1–5.3. ISBN 978-0-13-409266-9.
- Denis Bakhvalov, *Performance Analysis and Tuning on Modern CPUs*, 2e. ISBN 979-8-86958-422-9. [Free PDF](https://github.com/dendibakh/perf-book).
- Mytkowicz et al., [Producing Wrong Data Without Doing Anything Obviously Wrong!](https://dl.acm.org/doi/10.1145/1508284.1508275) ([PDF](https://dijkstra.eecs.umich.edu/kleach/eecs481/w21/readings/producing-wrong-data.pdf))
- Bakhvalov, [How to get consistent results when benchmarking on Linux?](https://easyperf.net/blog/2019/08/02/Perf-measurement-environment-on-Linux)
- Chandler Carruth, [Going Nowhere Faster](https://www.youtube.com/watch?v=2EWejmkKlxs)
- [Dougall Johnson, Apple CPU](https://dougallj.github.io/applecpu/)
