/* Lesson 08 - same C on ARM64 and x86-64.
 *
 *     make run LESSON=08
 *     make asm LESSON=08
 *     make x86 LESSON=08
 */
#include <stdio.h>

#include "timer.h"

__attribute__((noinline)) int sum_args(int a, int b, int c, int d, int e, int f,
                                       int g, int h) {
    return a + b + c + d + e + f + g + h;
}

int main(void) {
    asmlab_header("08 - x86-64 ABI and assembly");

    int result = sum_args(1, 2, 3, 4, 5, 6, 7, 8);
    printf("  sum_args(1..8) = %d\n\n", result);

    const uint64_t N = 50000000;
    BENCH("sum_args, through a call", N / 10, {
        int acc = 0;
        for (uint64_t k = 0; k < N / 10; k++) {
            acc = sum_args(acc, 1, 1, 1, 1, 1, 1, 1);
            KEEP(acc);
        }
        KEEP(acc);
    });

    printf("\n  Compare ARM64 and x86-64 dumps with:\n");
    printf("    make asm LESSON=08\n");
    printf("    make x86 LESSON=08\n");
    printf("    make run LESSON=08   (runs compare.py too)\n\n");
    return 0;
}
