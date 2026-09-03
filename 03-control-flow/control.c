/* Lesson 03 - control flow, memory, and structs in ARM64.
 *
 *     make asm LESSON=03
 *     tools/asmdump.sh -r -f jump_table 03-control-flow/control.c
 */
#include <stdio.h>

#include "timer.h"

struct Point {
    int x;
    int y;
};

__attribute__((noinline)) int abs_if(int x) {
    if (x < 0) {
        return -x;
    }
    return x;
}

__attribute__((noinline)) int sum_for(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += i;
        KEEP(sum);
    }
    return sum;
}

__attribute__((noinline)) void scale_array(int *a, int n) {
    for (int i = 0; i < n; i++) {
        a[i] *= 2;
    }
}

__attribute__((noinline)) int struct_field(struct Point p) {
    return p.y;
}

__attribute__((noinline)) int jump_table(int k) {
    switch (k) {
    case 0:
        return 100;
    case 1:
        return 250;
    case 2:
        return 175;
    case 3:
        return 400;
    case 4:
        return 320;
    case 5:
        return 512;
    case 6:
        return 640;
    case 7:
        return 128;
    default:
        return -1;
    }
}

int main(int argc, char **argv) {
    int stepping = argc > 1;
    (void)argv;
    asmlab_header("03 - control flow and data structures");

    printf("  correctness\n");
    printf("    abs_if(-5)           = %d\n", abs_if(-5));
    printf("    sum_for(10)          = %d\n", sum_for(10));

    int arr[4] = {1, 2, 3, 4};
    scale_array(arr, 4);
    printf("    scale_array          = {%d, %d, %d, %d}\n", arr[0], arr[1],
           arr[2], arr[3]);

    struct Point p = {.x = 10, .y = 20};
    printf("    struct_field(p)      = %d\n", struct_field(p));
    printf("    jump_table(3)        = %d\n", jump_table(3));
    printf("    jump_table(99)       = %d\n", jump_table(99));

    if (stepping) {
        return 0;
    }

    printf("\n  cost\n");

    const uint64_t N = 200000;

    {
        int acc = 0;
        BENCH("sum_for, n=1000", N, {
            int s = acc;
            for (uint64_t k = 0; k < N; k++) {
                s = sum_for(1000);
                KEEP(s);
            }
            acc = s;
        });
    }

    printf("\n  Single-step with:  python3 03-control-flow/step.py\n");
    printf("  Jump table data:   tools/asmdump.sh -r -f jump_table ");
    printf("03-control-flow/control.c\n\n");
    return 0;
}
