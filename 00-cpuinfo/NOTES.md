# 00 — Know your machine

```
make run LESSON=00
```

## Why start here

Every performance claim in this lab is relative to a specific chip. "This
array doesn't fit in cache" is meaningless until you know the cache is 64 KB.
Run this once and keep the output nearby.

## What you should see

| Property | Value on this M1 Pro | Why it matters |
|---|---|---|
| Performance cores | 8 | Where your benchmark wants to run |
| Efficiency cores | 2 | Same code, roughly 2-3x slower |
| L1 data cache | 64 KB | Lesson 07's first latency cliff |
| L2 cache | 4 MB | Lesson 07's second cliff |
| Cache line | **128 bytes** | Twice x86's 64 |
| Page size | **16 KB** | Four times x86's 4 KB |
| Core clock | ~3.2 GHz | One cycle ≈ 0.31 ns |

## The two surprises

**The cache line is 128 bytes.** Nearly every article about false sharing and
struct padding assumes 64. On Apple Silicon, two variables 64 bytes apart still
share a line and still contend. Lesson 07 measures this.

**There is no L3 in sysctl.** The M1 Pro has a large system-level cache shared
with the GPU, but the kernel does not report it. Lesson 07 finds it empirically
instead, as a shelf in the latency curve past 4 MB.

## Where the clock number comes from

macOS will not tell you the core frequency, so `timer.h` measures it: it runs a
chain of `add` instructions where each one depends on the previous result. That
dependency forces exactly one add per cycle, no matter how wide the CPU is. Time
16 million of them, divide, and you have the clock. Lesson 05 unpacks this.

Two clocks get printed and they are not the same thing:

- `cntvct_el0` ticks at 24 MHz, so it can only resolve about 41 ns. It is a
  timer, not a cycle counter.
- The measured ~3.2 GHz core clock is what converts nanoseconds to cycles.

ARM64 does have a real cycle counter, `PMCCNTR_EL0`, but reading it from user
space on Apple Silicon raises `SIGILL`. Lesson 05 covers the workarounds.

## Further reading

Full list with ISBNs: [README.md](../README.md#resources).

- CS:APP 3e, ch. 6.1–6.2. ISBN 978-0-13-409266-9. North American hardcover.
- [Dougall Johnson, Apple CPU](https://dougallj.github.io/applecpu/)
- Andrei Frumusanu, [Putting Apple Silicon M1 To The Test](https://www.anandtech.com/show/16252/mac-mini-apple-m1-tested)
- [Colin Scott, latency numbers](https://colin-scott.github.io/personal_website/research/interactive_latency.html)
- Ben Eater, [8-bit breadboard computer](https://www.youtube.com/playlist?list=PLowKtXNTBypGqImE405J2565dvjafglHU) (first few videos)
- Casey Muratori, [Welcome to Performance-Aware Programming](https://www.computerenhance.com/p/welcome-to-the-performance-aware)
