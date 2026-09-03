/* Lesson 04 - the C side of the three-way Python cost measurement.
 *
 * Pair with count.py (bytecode and wall time) and count_insns.py (lldb
 * instruction count of one PyNumber_Add). KEEP sits inside the loops so
 * -O2 cannot replace N additions of 1 with a single add.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "timer.h"

__attribute__((noinline)) int add_called(int a, int b) {
    return a + b;
}

static int sum_i32(const int *a, size_t n) {
    int s = 0;
    for (size_t i = 0; i < n; i++) {
        s += a[i];
        KEEP(s);
    }
    return s;
}

int main(void) {
    asmlab_header("04 - C costs, for the Python comparison");

    const uint64_t N = 50000000;
    {
        int acc = 0;
        BENCH("add, through a function call", N, {
            int a = acc;
            for (uint64_t k = 0; k < N; k++) {
                a = add_called(a, 1);
                KEEP(a);
            }
            acc = a;
        });
    }

    const size_t LEN = 1 << 20;
    int *buf = malloc(LEN * sizeof *buf);
    if (!buf) {
        perror("malloc");
        return 1;
    }
    for (size_t i = 0; i < LEN; i++) buf[i] = (int)(i & 0xff);

    {
        int sink = 0;
        BENCH("sum 1M ints, C loop", LEN, {
            sink = sum_i32(buf, LEN);
            KEEP(sink);
        });
        printf("    checksum %d\n", sink);
    }

    free(buf);
    printf("\n  A C add through a real call is a few cycles. Python's a+b is\n");
    printf("  the same add under a much larger interpreter. Run count.py for\n");
    printf("  bytecode, wall time, and the lldb instruction count.\n\n");
    return 0;
}
