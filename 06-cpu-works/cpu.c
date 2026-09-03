/* Lesson 06 - the CPU hides latency until it cannot.
 *
 *     make run LESSON=06
 *     make asm LESSON=06
 *     make asm LESSON=06 FUNC=vector_add_autovec
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "timer.h"

static uint32_t rng_state = 2463534242u;

static uint32_t xorshift32(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static int compare_int(const void *pa, const void *pb) {
    int a = *(const int *)pa;
    int b = *(const int *)pb;
    return (a > b) - (a < b);
}

static void fill_random_u8(int *a, size_t n) {
    for (size_t i = 0; i < n; i++) {
        a[i] = (int)(xorshift32() & 0xffu);
    }
}

static volatile int branch_sink;

__attribute__((noinline)) uint64_t branch_scan(const int *data, size_t n) {
    uint64_t sum = 0;
#pragma clang loop vectorize(disable) interleave(disable)
    for (size_t i = 0; i < n; i++) {
        int v = data[i];
        if (v >= 128) {
            /* A store the optimizer cannot remove. Without it, clang turns
             * this if into csel or NEON and there is no branch to mispredict. */
            branch_sink = v;
            sum += (uint64_t)v;
        }
    }
    return sum;
}

__attribute__((noinline)) void vector_add_autovec(float *c, const float *a,
                                                    const float *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        c[i] = a[i] + b[i];
    }
}

__attribute__((noinline)) void vector_add_blocked(float *c, const float *a,
                                                  const float *b, size_t n) {
    float acc = 0.0f;
    for (size_t i = 0; i < n; i++) {
        acc += a[i];
        c[i] = acc + b[i];
    }
}

int main(void) {
    asmlab_header("06 - how the CPU executes code");

    const uint64_t N = 100000000;
    const size_t branch_n = 1u << 22;
    const size_t vec_n = 1u << 18;
    const size_t vec_passes = 64;

    int *raw = malloc(branch_n * sizeof(int));
    int *sorted = malloc(branch_n * sizeof(int));
    float *fa = malloc(vec_n * sizeof(float));
    float *fb = malloc(vec_n * sizeof(float));
    float *fc = malloc(vec_n * sizeof(float));

    if (!raw || !sorted || !fa || !fb || !fc) {
        fprintf(stderr, "malloc failed\n");
        free(raw);
        free(sorted);
        free(fa);
        free(fb);
        free(fc);
        return 1;
    }

    fill_random_u8(raw, branch_n);
    for (size_t i = 0; i < branch_n; i++) {
        sorted[i] = raw[i];
    }
    qsort(sorted, branch_n, sizeof(int), compare_int);

    for (size_t i = 0; i < vec_n; i++) {
        fa[i] = (float)i;
        fb[i] = (float)(vec_n - i);
    }

    printf("  correctness\n");
    vector_add_autovec(fc, fa, fb, 4);
    printf("    fc[0] after autovec    = %.1f  (expect %.1f)\n", fc[0],
           fa[0] + fb[0]);

    printf("\n  latency vs throughput\n");

    {
        int acc = 0;
        BENCH("add latency, dependent", N, {
            int a = acc;
            for (uint64_t k = 0; k < N; k++) {
                a = a + 1;
                KEEP(a);
            }
            acc = a;
        });
    }

    {
        int a0 = 0, a1 = 0, a2 = 0, a3 = 0;
        int a4 = 0, a5 = 0, a6 = 0, a7 = 0;
        BENCH("add throughput, 8 independent", N, {
            int w0 = a0, w1 = a1, w2 = a2, w3 = a3;
            int w4 = a4, w5 = a5, w6 = a6, w7 = a7;
            for (uint64_t k = 0; k < N / 8; k++) {
                w0 = w0 + 1;
                w1 = w1 + 1;
                w2 = w2 + 1;
                w3 = w3 + 1;
                w4 = w4 + 1;
                w5 = w5 + 1;
                w6 = w6 + 1;
                w7 = w7 + 1;
                KEEP(w0);
                KEEP(w1);
                KEEP(w2);
                KEEP(w3);
                KEEP(w4);
                KEEP(w5);
                KEEP(w6);
                KEEP(w7);
            }
            a0 = w0;
            a1 = w1;
            a2 = w2;
            a3 = w3;
            a4 = w4;
            a5 = w5;
            a6 = w6;
            a7 = w7;
        });
    }

    printf("\n  branch prediction\n");

    {
        uint64_t sum = 0;
        BENCH("branch, data sorted", branch_n, {
            uint64_t s = sum;
            s += branch_scan(sorted, branch_n);
            TOUCH(sorted);
            KEEP((int)s);
            sum = s;
        });
    }

    {
        uint64_t sum = 0;
        BENCH("branch, data unsorted", branch_n, {
            uint64_t s = sum;
            s += branch_scan(raw, branch_n);
            TOUCH(raw);
            KEEP((int)s);
            sum = s;
        });
    }

    printf("\n  pipeline latency\n");

    {
        uint32_t acc = 1000000u;
        BENCH("add latency (control)", N, {
            uint32_t a = acc;
            for (uint64_t k = 0; k < N; k++) {
                a = a + 1u;
                KEEP(a);
            }
            acc = a;
        });
    }

    {
        uint32_t acc = 1000000u;
        const uint32_t div = 3u;
        BENCH("udiv latency, dependent", N, {
            uint32_t a = acc;
            for (uint64_t k = 0; k < N; k++) {
#if defined(__aarch64__)
                __asm__ volatile("udiv %w0, %w0, %w1" : "+r"(a) : "r"(div));
#else
                a = a / div;
                if (a == 0u) {
                    a = 1000000u;
                }
#endif
                KEEP(a);
            }
            acc = a;
        });
    }

    printf("\n  SIMD autovectorization\n");

    {
        BENCH("vector add, autovec", vec_n * vec_passes, {
            for (size_t p = 0; p < vec_passes; p++) {
                vector_add_autovec(fc, fa, fb, vec_n);
                TOUCH(fc);
            }
        });
    }

    {
        BENCH("vector add, blocked", vec_n * vec_passes, {
            for (size_t p = 0; p < vec_passes; p++) {
                vector_add_blocked(fc, fa, fb, vec_n);
                TOUCH(fc);
            }
        });
    }

    printf("\n  Read these numbers as:\n");
    printf("    dependent add ~1 cycle     each result waits on the last\n");
    printf("    8 independent <1 cycle     multiple integer units overlap\n");
    printf("    sorted branch faster       the predictor learns the pattern\n");
    printf("    udiv >> add                divide has a long pipeline\n");
    printf("    autovec cheaper than blocked  SIMD when dependencies allow\n");
    printf("\n  Dump the assembly with:  make asm LESSON=06\n\n");

    free(raw);
    free(sorted);
    free(fa);
    free(fb);
    free(fc);
    return 0;
}
