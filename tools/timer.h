/* timer.h - the measurement harness shared by every lesson.
 *
 * Lesson 05 explains why each piece here exists. The short version:
 *
 *   now_ns()      monotonic nanosecond clock, the primary timer
 *   cntvct()      raw ARM64 virtual counter, 24 MHz, for comparison
 *   asmlab_ghz()  clock speed, measured by timing a known instruction chain
 *   KEEP/TOUCH    stop the optimizer from deleting the code being measured
 *   BENCH_LOOP    run a statement N times, report ns/op and cycles/op
 *
 * PMCCNTR_EL0, the real cycle counter, is not readable from user space on
 * Apple Silicon, so cycles here are always ns * measured_GHz.
 */
#ifndef ASMLAB_TIMER_H
#define ASMLAB_TIMER_H

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/qos.h>
#include <time.h>

#ifndef ASMLAB_WARMUP
#define ASMLAB_WARMUP 2
#endif

#ifndef ASMLAB_TRIALS
#define ASMLAB_TRIALS 7
#endif

/* CLOCK_UPTIME_RAW is monotonic and excludes system sleep. On Apple Silicon
 * it is backed by the same 24 MHz counter as cntvct, so its true granularity
 * is about 41 ns no matter how many digits it prints. */
static inline uint64_t now_ns(void) {
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
}

#if defined(__aarch64__)
/* The virtual counter is one of the few system registers readable at EL0. */
static inline uint64_t cntvct(void) {
    uint64_t v;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
}
static inline uint64_t cntfrq(void) {
    uint64_t v;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(v));
    return v;
}
#endif

/* KEEP forces a value to exist in a register at this point in the program.
 * Without it the optimizer deletes benchmark loops whose results are unused,
 * and you measure an empty loop. TOUCH does the same for memory. */
#define KEEP(x)     __asm__ volatile("" : : "r"(x) : "memory")
#define TOUCH(p)    __asm__ volatile("" : : "r"((void *)(p)) : "memory")
#define BARRIER()   __asm__ volatile("" : : : "memory")

/* Ask the scheduler for a performance core. macOS has no CPU affinity API,
 * but QoS class strongly influences P-core vs E-core placement, and an E-core
 * runs the same code roughly 2-3x slower. */
static inline void asmlab_prefer_p_core(void) {
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
}

/* Measure clock speed by timing something whose cycle cost we already know.
 *
 * A chain of `add x, x, #1` where each add depends on the previous one runs at
 * exactly one per cycle: the CPU is superscalar but cannot start add N+1 until
 * add N produces its result. The loop counter and branch are independent, so
 * they issue in parallel and cost nothing extra. 16 adds per iteration means
 * 16 cycles per iteration, so cycles / elapsed_ns gives GHz.
 *
 * Trial 0 is discarded: the core boosts its clock only once it has been busy
 * for a few milliseconds, so an un-warmed measurement reads several percent
 * low. This is the same DVFS ramp that makes cold benchmarks lie. */
static inline double asmlab_measure_ghz(void) {
#if defined(__aarch64__)
    const uint64_t iters = 3000000;
    const uint64_t adds_per_iter = 16;
    /* Keep the highest reading. Contention and interrupts only ever slow the
     * chain down, so the fastest trial is the least polluted one. */
    double best = 0.0;

    for (int trial = 0; trial < 9; trial++) {
        uint64_t acc = 0, n = iters;
        uint64_t t0 = now_ns();
        __asm__ volatile(
            "1:\n\t"
#define A16 "add %0, %0, #1\n\t"
            A16 A16 A16 A16 A16 A16 A16 A16
            A16 A16 A16 A16 A16 A16 A16 A16
#undef A16
            "subs %1, %1, #1\n\t"
            "b.ne 1b\n\t"
            : "+r"(acc), "+r"(n)
            :
            : "cc");
        uint64_t t1 = now_ns();
        double ghz = (double)(iters * adds_per_iter) / (double)(t1 - t0);
        if (trial > 0 && ghz > best) best = ghz;
        KEEP(acc);
    }
    return best;
#else
    /* x86-64 under Rosetta: the same trick with a dependent add chain. */
    const uint64_t iters = 3000000;
    const uint64_t adds_per_iter = 16;
    double best = 0.0;

    for (int trial = 0; trial < 5; trial++) {
        uint64_t acc = 0, n = iters;
        uint64_t t0 = now_ns();
        __asm__ volatile(
            "1:\n\t"
#define A16 "addq $1, %0\n\t"
            A16 A16 A16 A16 A16 A16 A16 A16
            A16 A16 A16 A16 A16 A16 A16 A16
#undef A16
            "subq $1, %1\n\t"
            "jne 1b\n\t"
            : "+r"(acc), "+r"(n)
            :
            : "cc");
        uint64_t t1 = now_ns();
        double ghz = (double)(iters * adds_per_iter) / (double)(t1 - t0);
        if (ghz > best) best = ghz;
        KEEP(acc);
    }
    return best;
#endif
}

static inline double asmlab_ghz(void) {
    static double cached = 0.0;
    if (cached == 0.0) cached = asmlab_measure_ghz();
    return cached;
}

static inline void asmlab_report(const char *name, uint64_t ops, double ns) {
    double per_op = ns / (double)ops;
    printf("  %-38s %10.2f ns/op  %8.2f cycles/op\n", name, per_op,
           per_op * asmlab_ghz());
    fflush(stdout);
}

/* BENCH: the body is responsible for performing ops_ operations.
 * Reports the fastest of ASMLAB_TRIALS runs, because a benchmark can be
 * slowed down by interrupts and cache eviction but never sped up, so the
 * minimum is the closest thing to the code's true cost.
 *
 * Variadic so that a body containing commas (declaration lists, multi-argument
 * calls) is not chopped into separate macro arguments. */
#define BENCH(name_, ops_, ...)                                                \
    do {                                                                       \
        const uint64_t _ops = (uint64_t)(ops_);                                \
        for (int _w = 0; _w < ASMLAB_WARMUP; _w++) { __VA_ARGS__; }            \
        double _best = 1.0e300;                                                \
        for (int _t = 0; _t < ASMLAB_TRIALS; _t++) {                           \
            uint64_t _t0 = now_ns();                                           \
            __VA_ARGS__;                                                       \
            uint64_t _t1 = now_ns();                                           \
            double _ns = (double)(_t1 - _t0);                                  \
            if (_ns < _best) _best = _ns;                                      \
        }                                                                      \
        asmlab_report((name_), _ops, _best);                                   \
    } while (0)

/* BENCH_LOOP writes the loop for you. `i` is available inside the body. */
#define BENCH_LOOP(name_, n_, ...)                                             \
    BENCH(name_, n_, {                                                         \
        for (uint64_t i = 0; i < (uint64_t)(n_); i++) { __VA_ARGS__; }         \
    })

static inline void asmlab_header(const char *title) {
    asmlab_prefer_p_core();
    printf("\n=== %s ===\n", title);
    printf("  (measured clock: %.3f GHz)\n\n", asmlab_ghz());
    fflush(stdout);
}

#endif /* ASMLAB_TIMER_H */
