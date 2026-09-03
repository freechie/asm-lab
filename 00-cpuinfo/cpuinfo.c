/* Lesson 00 - the numbers every later lesson refers back to.
 *
 * Reads the CPU's own description of itself through sysctl. When lesson 07
 * finds a cliff in memory latency at some size, this is where you check
 * whether that size is a cache boundary.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysctl.h>

#include "timer.h"

static uint64_t num(const char *name) {
    uint64_t v = 0;
    size_t len = sizeof v;
    if (sysctlbyname(name, &v, &len, NULL, 0) != 0) return 0;
    /* Some keys are 32-bit; sysctl writes only the low half. */
    if (len == sizeof(uint32_t)) v &= 0xffffffffu;
    return v;
}

static void str(const char *name, char *out, size_t cap) {
    size_t len = cap;
    if (sysctlbyname(name, out, &len, NULL, 0) != 0) snprintf(out, cap, "?");
}

static void human(uint64_t bytes, char *out, size_t cap) {
    if (bytes == 0)                 snprintf(out, cap, "n/a");
    else if (bytes >= 1u << 30)     snprintf(out, cap, "%.2f GB", bytes / 1073741824.0);
    else if (bytes >= 1u << 20)     snprintf(out, cap, "%.0f MB", bytes / 1048576.0);
    else if (bytes >= 1024)         snprintf(out, cap, "%.0f KB", bytes / 1024.0);
    else                            snprintf(out, cap, "%llu B", (unsigned long long)bytes);
}

static void line(const char *label, uint64_t bytes) {
    char h[32];
    human(bytes, h, sizeof h);
    printf("  %-26s %12s  (%llu bytes)\n", label, h, (unsigned long long)bytes);
}

int main(void) {
    char brand[128], model[128];
    str("machdep.cpu.brand_string", brand, sizeof brand);
    str("hw.model", model, sizeof model);

    printf("\n=== CPU ===\n");
    printf("  %-26s %s\n", "chip", brand);
    printf("  %-26s %s\n", "machine", model);
    printf("  %-26s %llu\n", "logical cores",
           (unsigned long long)num("hw.logicalcpu"));
    printf("  %-26s %llu\n", "performance cores",
           (unsigned long long)num("hw.perflevel0.physicalcpu"));
    printf("  %-26s %llu\n", "efficiency cores",
           (unsigned long long)num("hw.perflevel1.physicalcpu"));

    printf("\n=== MEMORY HIERARCHY ===\n");
    line("L1 data cache",   num("hw.l1dcachesize"));
    line("L1 instr cache",  num("hw.l1icachesize"));
    line("L2 cache",        num("hw.l2cachesize"));
    line("L3 cache",        num("hw.l3cachesize"));
    line("cache line",      num("hw.cachelinesize"));
    line("page size",       num("hw.pagesize"));
    line("RAM",             num("hw.memsize"));

    printf("\n=== CLOCKS ===\n");
    asmlab_prefer_p_core();
#if defined(__aarch64__)
    printf("  %-26s %llu Hz  (%.1f MHz)\n", "cntvct_el0 tick rate",
           (unsigned long long)cntfrq(), cntfrq() / 1e6);
    printf("  %-26s %.1f ns\n", "  -> its resolution", 1e9 / (double)cntfrq());
#endif
    printf("  %-26s %.3f GHz\n", "measured core clock", asmlab_ghz());
    printf("  %-26s %.3f ns\n", "  -> one cycle is", 1.0 / asmlab_ghz());

    printf("\nNotes\n");
    printf("  * The cache line is 128 bytes here. On x86-64 it is 64. Any code\n");
    printf("    tuned around a 64-byte line is mistuned on this machine.\n");
    printf("  * L3 reports as n/a: the M-series shared last-level cache is not\n");
    printf("    exposed through sysctl, though lesson 07 will still see it.\n");
    printf("  * Cache sizes are per performance core (L1) and per cluster (L2).\n");
    printf("\n");
    return 0;
}
