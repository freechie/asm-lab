/* Lesson 05 - why measuring code is harder than it looks.
 *
 * Every later lesson trusts tools/timer.h. This one shows the ways a
 * benchmark can lie, and why each piece of the harness exists.
 */
#include <stdint.h>
#include <stdio.h>

#include "timer.h"

__attribute__((noinline)) static void empty_call(void) {
    __asm__ volatile("" ::: "memory");
}

static inline int add_one(int x) {
    return x + 1;
}

static void busy_warmup(void) {
    volatile uint64_t sink = 0;
    for (uint64_t i = 0; i < 50000000; i++) {
        sink += i;
    }
    KEEP(sink);
}

static double time_add_chain(uint64_t iters) {
    uint64_t acc = 0, n = iters;
    uint64_t t0 = now_ns();
#if defined(__aarch64__)
    __asm__ volatile(
        "1:\n\t"
        "add %0, %0, #1\n\t"
        "subs %1, %1, #1\n\t"
        "b.ne 1b\n\t"
        : "+r"(acc), "+r"(n)
        :
        : "cc");
#else
    __asm__ volatile(
        "1:\n\t"
        "addq $1, %0\n\t"
        "subq $1, %1\n\t"
        "jne 1b\n\t"
        : "+r"(acc), "+r"(n)
        :
        : "cc");
#endif
    uint64_t t1 = now_ns();
    KEEP(acc);
    return (double)(t1 - t0);
}

int main(void) {
    const uint64_t warm_iters = 1000000;
    double cold_ns;

    asmlab_prefer_p_core();
    cold_ns = time_add_chain(warm_iters);

    asmlab_header("05 - timing pitfalls");

    const uint64_t N = 100000000;

    printf("  optimizer\n");

    /* KEEP after the loop lets the compiler fold N additions of 1 into one. */
    {
        int a = 0;
        uint64_t t0 = now_ns();
        for (uint64_t k = 0; k < N; k++) {
            a = a + 1;
        }
        KEEP(a);
        uint64_t t1 = now_ns();
        printf("  deleted loop (no KEEP)              %10.2f ns/op  (loop may fold)\n",
               (double)(t1 - t0) / (double)N);
    }

    {
        int acc = 0;
        BENCH("kept loop (KEEP inside)", N, {
            int w = acc;
            for (uint64_t k = 0; k < N; k++) {
                w = w + 1;
                KEEP(w);
            }
            acc = w;
        });
    }

    printf("\n  clock grain\n");
#if defined(__aarch64__)
    printf("  cntfrq_el0                          %10llu Hz\n",
           (unsigned long long)cntfrq());
    printf("  measured core clock                 %10.3f GHz\n", asmlab_ghz());
    printf("  one core cycle                      %10.3f ns\n", 1.0 / asmlab_ghz());
#endif
    {
        uint64_t min_delta = UINT64_MAX;
        for (int i = 0; i < 1000000; i++) {
            uint64_t t0 = now_ns();
            uint64_t t1 = now_ns();
            uint64_t d = t1 - t0;
            if (d > 0 && d < min_delta) {
                min_delta = d;
            }
        }
        printf("  smallest nonzero now_ns() delta     %10llu ns\n",
               (unsigned long long)min_delta);
    }

    printf("\n  DVFS ramp\n");
    {
        busy_warmup();
        double warm = time_add_chain(warm_iters);
        printf("  cold dependent add chain            %10.2f ns/op  %8.2f cycles/op\n",
               cold_ns / (double)warm_iters,
               (cold_ns / (double)warm_iters) * asmlab_ghz());
        printf("  warm dependent add chain            %10.2f ns/op  %8.2f cycles/op\n",
               warm / (double)warm_iters,
               (warm / (double)warm_iters) * asmlab_ghz());
    }

    printf("\n  min vs mean\n");
    {
        const uint64_t iters = 5000000;
        const int trials = 7;
        double sum = 0.0;
        double best = 1.0e300;

        for (int t = 0; t < trials; t++) {
            double ns = time_add_chain(iters);
            sum += ns;
            if (ns < best) {
                best = ns;
            }
        }
        double mean = sum / (double)trials;
        printf("  dependent add chain (min)           %10.2f ns/op  %8.2f cycles/op\n",
               best / (double)iters, (best / (double)iters) * asmlab_ghz());
        printf("  dependent add chain (mean)          %10.2f ns/op  %8.2f cycles/op\n",
               mean / (double)iters, (mean / (double)iters) * asmlab_ghz());
    }

    printf("\n  calibration\n");
    {
        const uint64_t iters = 10000000;
        BENCH("calibration, dependent add chain", iters, {
            uint64_t acc = 0, n = iters;
#if defined(__aarch64__)
            __asm__ volatile(
                "1:\n\t"
                "add %0, %0, #1\n\t"
                "subs %1, %1, #1\n\t"
                "b.ne 1b\n\t"
                : "+r"(acc), "+r"(n)
                :
                : "cc");
#else
            __asm__ volatile(
                "1:\n\t"
                "addq $1, %0\n\t"
                "subq $1, %1\n\t"
                "jne 1b\n\t"
                : "+r"(acc), "+r"(n)
                :
                : "cc");
#endif
            KEEP(acc);
        });
    }

    printf("\n  call overhead\n");
    {
        int acc = 0;
        BENCH("inlined add", N, {
            int w = acc;
            for (uint64_t k = 0; k < N; k++) {
                w = add_one(w);
                KEEP(w);
            }
            acc = w;
        });
    }
    {
        const uint64_t M = N / 100;
        BENCH("noinline empty call", M, {
            for (uint64_t k = 0; k < M; k++) {
                empty_call();
                KEEP(k);
            }
        });
    }

    return 0;
}
