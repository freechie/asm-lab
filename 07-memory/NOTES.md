# 07 — Memory hierarchy

```
make run LESSON=07
```

## What this lesson measures

Four experiments map the memory system empirically on this chip.

The pointer-chase table sweeps working-set size from 4 KB to 32 MB. The list
is a random ring so hardware prefetch cannot hide the cache. Latency jumps
near 64 KB when the list stops fitting in L1. A second knee near 4 MB marks
L2. Past that, accesses pay DRAM cost.

## Surprises from lesson 00

The cache line here is 128 bytes, not the 64 bytes most articles assume. Two 64-bit variables 64 bytes apart still share one line and can still false-share.

The page size is 16 KB, four times the 4 KB pages common on x86.

There is no L3 size in sysctl. The shelf in the latency curve past 4 MB is how you find the next level.

## How to read the knees

The first jump on this M1 Pro is often near 256 KB rather than the 64 KB L1
size sysctl reports. Trust the table in front of you. A second shelf around
4 MB is L2. Then DRAM.

## Benchmark names

| Name | What it shows |
|---|---|
| `chase, 16 KB (L1)` | Pointer chase with the list in L1 |
| `chase, 16 MB (DRAM)` | Same chase with the list in DRAM |
| `stride 64 B` | Touch every 64 bytes. Two touches per 128-byte line |
| `stride 128 B` | Touch every 128 bytes. One touch per line |
| `false sharing, 8 B apart` | Two threads `atomic_fetch_add` on the same line |
| `no sharing, 128 B apart` | Same increment on separate lines |
| `matrix, row-major` | Scan a 1024×1024 array in storage order |
| `matrix, column-major` | Scan the same array against the grain |

DRAM must be slower than L1. Row-major must beat column-major for C row-major arrays.

## Further reading

Full list with ISBNs: [RESOURCES.md](../RESOURCES.md).

- CS:APP 3e, ch. 6. ISBN 978-0-13-409266-9.
- Ulrich Drepper, [What Every Programmer Should Know About Memory](https://www.akkadia.org/drepper/cpumemory.pdf) (2007)
- Igor Ostrovsky, [Gallery of Processor Cache Effects](https://igoro.com/archive/gallery-of-processor-cache-effects/)
- Herb Sutter, [Eliminate False Sharing](https://herbsutter.com/2009/05/15/effective-concurrency-eliminate-false-sharing/)
- Patterson and Hennessy, *Computer Organization and Design* (ARM Edition), ch. 5. ISBN 978-0-12-801733-3.
- Casey Muratori, Performance-Aware Programming, Part 3 (Moving Data)
- [Algorithms for Modern Hardware](https://en.algorithmica.org/hpc/), memory chapters
