/* Lesson 07 - memory hierarchy on Apple Silicon.
 *
 *     make run LESSON=07
 */
#include <stdatomic.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "timer.h"

#define CACHE_LINE 128
#define NODE_BYTES 128

typedef struct ChaseNode {
    struct ChaseNode *next;
    char pad[NODE_BYTES - sizeof(struct ChaseNode *)];
} ChaseNode;

static uint32_t rng_state = 2463534242u;

static uint32_t xorshift32(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static ChaseNode *build_chase(void *buf, size_t bytes) {
    size_t count = bytes / NODE_BYTES;
    ChaseNode *nodes = (ChaseNode *)buf;
    if (count == 0) {
        return NULL;
    }

    /* Random order so the prefetcher cannot hide the cache boundary. A
     * sequential ring would look almost flat across L1 and DRAM. */
    size_t *idx = malloc(count * sizeof *idx);
    if (idx == NULL) {
        exit(1);
    }
    for (size_t i = 0; i < count; i++) {
        idx[i] = i;
    }
    for (size_t i = count - 1; i > 0; i--) {
        size_t j = (size_t)xorshift32() % (i + 1);
        size_t tmp = idx[i];
        idx[i] = idx[j];
        idx[j] = tmp;
    }
    for (size_t i = 0; i < count; i++) {
        nodes[idx[i]].next = &nodes[idx[(i + 1) % count]];
    }
    ChaseNode *start = &nodes[idx[0]];
    free(idx);
    return start;
}

static uintptr_t pointer_chase(ChaseNode *start, uint64_t steps) {
    ChaseNode *p = start;
    for (uint64_t i = 0; i < steps; i++) {
        p = p->next;
        KEEP(p);
    }
    return (uintptr_t)p;
}

static double chase_ns_per_access(ChaseNode *start, uint64_t steps, int warmup) {
    if (warmup) {
        for (int w = 0; w < ASMLAB_WARMUP; w++) {
            KEEP(pointer_chase(start, steps));
        }
    }

    double best = 1.0e300;
    for (int t = 0; t < ASMLAB_TRIALS; t++) {
        uint64_t t0 = now_ns();
        uintptr_t tail = pointer_chase(start, steps);
        uint64_t t1 = now_ns();
        KEEP(tail);
        double ns = (double)(t1 - t0);
        if (ns < best) {
            best = ns;
        }
    }
    return best / (double)steps;
}

static void working_set_sweep(void) {
    const size_t min_bytes = 4 * 1024;
    const size_t max_bytes = 32 * 1024 * 1024;

    printf("  working-set latency (one random lap per size)\n");
    printf("    %10s  %12s\n", "size", "ns/access");

    for (size_t bytes = min_bytes; bytes <= max_bytes; bytes *= 2) {
        void *buf = malloc(bytes);
        if (buf == NULL) {
            fprintf(stderr, "malloc(%zu) failed\n", bytes);
            exit(1);
        }
        memset(buf, 0, bytes);

        size_t count = bytes / NODE_BYTES;
        ChaseNode *start = build_chase(buf, bytes);
        uint64_t steps = count;
        if (steps < 500000) {
            steps = ((500000 + count - 1) / count) * count;
        }
        double ns = chase_ns_per_access(start, steps, 0);

        const char *unit = " B";
        double shown = (double)bytes;
        if (bytes >= 1024 * 1024) {
            shown /= 1024.0 * 1024.0;
            unit = " MB";
        } else if (bytes >= 1024) {
            shown /= 1024.0;
            unit = " KB";
        }
        printf("    %8.0f%s  %12.2f\n", shown, unit, ns);

        free(buf);
    }
    printf("\n");
}

static void chase_benches(void) {
    const uint64_t steps = 500000;

    {
        const size_t bytes = 16 * 1024;
        void *buf = malloc(bytes);
        memset(buf, 0, bytes);
        ChaseNode *start = build_chase(buf, bytes);

        BENCH("chase, 16 KB (L1)", steps, {
            uintptr_t tail = pointer_chase(start, steps);
            KEEP(tail);
        });

        free(buf);
    }

    {
        const size_t bytes = 16 * 1024 * 1024;
        void *buf = malloc(bytes);
        memset(buf, 0, bytes);
        ChaseNode *start = build_chase(buf, bytes);

        BENCH("chase, 16 MB (DRAM)", steps, {
            uintptr_t tail = pointer_chase(start, steps);
            KEEP(tail);
        });

        free(buf);
    }
}

static uint64_t stride_walk(volatile char *buf, size_t buf_bytes, size_t stride,
                            uint64_t reps) {
    uint64_t acc = 0;
    for (uint64_t r = 0; r < reps; r++) {
        for (size_t off = 0; off < buf_bytes; off += stride) {
            buf[off] ^= (char)(r + 1);
            TOUCH((void *)&buf[off]);
            acc += (unsigned char)buf[off];
        }
    }
    KEEP(acc);
    return acc;
}

static void cache_line_benches(void) {
    const size_t buf_bytes = 8 * 1024 * 1024;
    volatile char *buf = (volatile char *)malloc(buf_bytes);
    if (buf == NULL) {
        exit(1);
    }
    memset((void *)buf, 0, buf_bytes);

    const uint64_t reps64 = buf_bytes / 64;
    const uint64_t reps128 = buf_bytes / 128;

    BENCH("stride 64 B", reps64 * 4, {
        KEEP(stride_walk(buf, buf_bytes, 64, 4));
    });

    BENCH("stride 128 B", reps128 * 4, {
        KEEP(stride_walk(buf, buf_bytes, 128, 4));
    });

    free((void *)buf);
}

typedef struct {
    atomic_uint_fast64_t a;
    atomic_uint_fast64_t b;
} counters_near;

typedef struct {
    atomic_uint_fast64_t a;
    char pad[CACHE_LINE - sizeof(atomic_uint_fast64_t)];
    atomic_uint_fast64_t b;
} counters_far;

typedef struct {
    atomic_uint_fast64_t *slot;
    uint64_t iters;
} inc_arg;

static void *increment_slot(void *arg) {
    inc_arg *work = (inc_arg *)arg;
    for (uint64_t i = 0; i < work->iters; i++) {
        atomic_fetch_add_explicit(work->slot, 1, memory_order_relaxed);
    }
    return NULL;
}

static void *cache_line_alloc(size_t size) {
    void *p = NULL;
    size_t bytes = (size + CACHE_LINE - 1) & ~(size_t)(CACHE_LINE - 1);
    if (posix_memalign(&p, CACHE_LINE, bytes) != 0) {
        return NULL;
    }
    return p;
}

static void false_sharing_benches(void) {
    counters_near *near = cache_line_alloc(sizeof(*near));
    counters_far *far = cache_line_alloc(sizeof(*far));
    if (near == NULL || far == NULL) {
        exit(1);
    }
    atomic_store(&near->a, 0);
    atomic_store(&near->b, 0);
    atomic_store(&far->a, 0);
    atomic_store(&far->b, 0);

    const uint64_t near_iters = 2000000;
    const uint64_t far_iters = 2000000;

    BENCH("false sharing, 8 B apart", near_iters * 2, {
        inc_arg args[2] = {
            {&near->a, near_iters},
            {&near->b, near_iters},
        };
        pthread_t t0, t1;
        pthread_create(&t0, NULL, increment_slot, &args[0]);
        pthread_create(&t1, NULL, increment_slot, &args[1]);
        pthread_join(t0, NULL);
        pthread_join(t1, NULL);
        KEEP(atomic_load(&near->a));
        KEEP(atomic_load(&near->b));
    });

    BENCH("no sharing, 128 B apart", far_iters * 2, {
        inc_arg args[2] = {
            {&far->a, far_iters},
            {&far->b, far_iters},
        };
        pthread_t t0, t1;
        pthread_create(&t0, NULL, increment_slot, &args[0]);
        pthread_create(&t1, NULL, increment_slot, &args[1]);
        pthread_join(t0, NULL);
        pthread_join(t1, NULL);
        KEEP(atomic_load(&far->a));
        KEEP(atomic_load(&far->b));
    });

    free(near);
    free(far);
}

static double matrix_sum_row_major(double *m, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            sum += m[i * n + j];
            TOUCH(&m[i * n + j]);
        }
    }
    KEEP(sum);
    return sum;
}

static double matrix_sum_col_major(double *m, int n) {
    double sum = 0.0;
    for (int j = 0; j < n; j++) {
        for (int i = 0; i < n; i++) {
            sum += m[i * n + j];
            TOUCH(&m[i * n + j]);
        }
    }
    KEEP(sum);
    return sum;
}

static void matrix_benches(void) {
    const int n = 1024;
    const uint64_t elems = (uint64_t)n * (uint64_t)n;
    double *m = malloc((size_t)elems * sizeof(double));
    if (m == NULL) {
        exit(1);
    }
    for (uint64_t i = 0; i < elems; i++) {
        m[i] = (double)i * 0.001;
    }

    BENCH("matrix, row-major", elems, {
        KEEP(matrix_sum_row_major(m, n));
    });

    BENCH("matrix, column-major", elems, {
        KEEP(matrix_sum_col_major(m, n));
    });

    free(m);
}

int main(void) {
    asmlab_header("07 - memory hierarchy");

    printf("  latency vs working-set size\n\n");
    working_set_sweep();

    printf("  pointer chase (L1 vs DRAM)\n\n");
    chase_benches();

    printf("  cache line size (128 bytes on this chip)\n\n");
    cache_line_benches();

    printf("  false sharing\n\n");
    false_sharing_benches();

    printf("  matrix traversal order\n\n");
    matrix_benches();

    printf("  Read the latency table knees near 64 KB (L1) and 4 MB (L2).\n");
    printf("  DRAM must be slower than L1. Row-major must beat column-major.\n\n");
    return 0;
}
