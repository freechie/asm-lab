/* Lesson 02 - small C functions whose ARM64 assembly you can read.
 *
 *     make asm LESSON=02
 *     make asm LESSON=02 DIFF=1
 *     make asm LESSON=02 FUNC=add_ints
 */
#include <stdio.h>

#include "timer.h"

__attribute__((noinline)) int add_ints(int a, int b) {
    return a + b;
}

__attribute__((noinline)) int load_store(int *p, int v) {
    *p = v;
    return *p;
}

__attribute__((noinline)) int mul_ints(int a, int b) {
    return a * b;
}

__attribute__((noinline)) int min_of_two(int a, int b) {
    if (a < b) {
        return a;
    }
    return b;
}

__attribute__((noinline)) int call_add(int a, int b) {
    return add_ints(a, b);
}

int main(void) {
    asmlab_header("02 - reading ARM64");

    printf("  correctness\n");
    printf("    add_ints(2, 3)       = %d\n", add_ints(2, 3));
    int slot = 0;
    printf("    load_store(&slot, 7) = %d\n", load_store(&slot, 7));
    printf("    mul_ints(6, 7)       = %d\n", mul_ints(6, 7));
    printf("    min_of_two(3, 9)     = %d\n", min_of_two(3, 9));
    printf("    call_add(4, 5)       = %d\n", call_add(4, 5));

    printf("\n  cost\n");

    const uint64_t N = 100000000;

    {
        int acc = 0;
        BENCH("add_ints, dependent chain", N, {
            int a = acc;
            for (uint64_t k = 0; k < N; k++) {
                a = add_ints(a, 1);
                KEEP(a);
            }
            acc = a;
        });
    }

    printf("\n  Dump the assembly with:\n");
    printf("    make asm LESSON=02\n");
    printf("    make asm LESSON=02 DIFF=1\n");
    printf("    make asm LESSON=02 FUNC=add_ints\n\n");
    return 0;
}
