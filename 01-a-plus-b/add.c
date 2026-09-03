/* Lesson 01 - what `a + b` costs in C, and proof that hand-written assembly
 * is doing the same thing.
 *
 * Read the generated assembly with:
 *     make asm LESSON=01
 *     make asm LESSON=01 DIFF=1     (-O0 beside -O2)
 */
#include <stdio.h>

#include "timer.h"

/* Implemented in add.S (preprocessor picks ARM64 or x86-64). */
int add_arm64(int a, int b);

/* The version the compiler writes. Small enough that it will be inlined at
 * -O2, which is exactly the point of the two measurements below. */
static inline int add_inline(int a, int b) {
    return a + b;
}

/* Same body, but the optimizer is forbidden from inlining it, so a real
 * branch-and-link happens. This is the honest comparison against Python,
 * where calling a function is unavoidable. */
__attribute__((noinline)) int add_called(int a, int b) {
    return a + b;
}

int main(void) {
    asmlab_header("01 - a + b in C and in assembly");

    printf("  correctness\n");
    printf("    add_inline(2, 3)  = %d\n", add_inline(2, 3));
    printf("    add_called(2, 3)  = %d\n", add_called(2, 3));
#if defined(__aarch64__)
    printf("    add_arm64(2, 3)   = %d   <- hand-written assembly\n",
           add_arm64(2, 3));
#else
    printf("    add_arm64(2, 3)   = %d   <- hand-written assembly (x86-64)\n",
           add_arm64(2, 3));
#endif

    const uint64_t N = 100000000;

    printf("\n  cost\n");

    /* A dependent chain: each add needs the previous result, so the CPU
     * cannot overlap them and we see the true latency of one add.
     *
     * KEEP goes inside the loop, not after it. Without it the optimizer
     * recognises that adding 1 N times equals adding N, replaces the whole
     * loop with a single instruction, and the benchmark reports 0.00 ns/op.
     * Lesson 05 shows that failure mode on purpose. */
    {
        int acc = 0;
        BENCH("add, dependent chain", N, {
            int a = acc;
            for (uint64_t k = 0; k < N; k++) {
                a = add_inline(a, 1);
                KEEP(a);
            }
            acc = a;
        });
    }

    /* Four independent chains. The M1 has several integer units, so it starts
     * several of these in the same cycle. Same instruction, less time each. */
    {
        int a = 0, b = 0, c = 0, d = 0;
        BENCH("add, 4 independent chains", N, {
            int w = a, x = b, y = c, z = d;
            for (uint64_t k = 0; k < N / 4; k++) {
                w = add_inline(w, 1);
                x = add_inline(x, 1);
                y = add_inline(y, 1);
                z = add_inline(z, 1);
                KEEP(w); KEEP(x); KEEP(y); KEEP(z);
            }
            a = w; b = x; c = y; d = z;
        });
    }

    /* With a real call: branch-and-link, the add, then return. */
    {
        int acc = 0;
        BENCH("add, through a function call", N / 10, {
            int a = acc;
            for (uint64_t k = 0; k < N / 10; k++) a = add_called(a, 1);
            acc = a;
            KEEP(acc);
        });
    }

    /* Through the hand-written assembly, to confirm it costs the same. */
    {
        int acc = 0;
        BENCH("add, hand-written assembly", N / 10, {
            int a = acc;
            for (uint64_t k = 0; k < N / 10; k++) a = add_arm64(a, 1);
            acc = a;
            KEEP(acc);
        });
    }

    printf("\n  Read these numbers as:\n");
    printf("    ~1.0 cycles  one add, waiting on the previous add. This is the\n");
    printf("                 instruction's latency, and it is the floor.\n");
    printf("    <1.0 cycles  independent adds overlapping. The chip is wider\n");
    printf("                 than one instruction per cycle, so identical code\n");
    printf("                 runs faster purely because it has no dependency.\n");
    printf("                 Lesson 06 pushes this further; the KEEP barriers\n");
    printf("                 here hold it back from the true throughput.\n");
    printf("    ~3.0 cycles  branch-and-link, the add, and the return.\n");
    printf("    The hand-written assembly matches the compiler exactly,\n");
    printf("    because it is the same two instructions.\n");
    printf("\n  Now the Python side:  make run LESSON=01   (or python3 01-a-plus-b/add.py)\n\n");
    return 0;
}
