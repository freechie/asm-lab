/* Lesson 01 - a + b, written directly in assembly for both architectures.
 *
 * Nothing here is generated. This is the whole function, and it is what clang
 * produces from `int add(int a, int b) { return a + b; }`.
 *
 * The file is `.S` rather than `.s` so the C preprocessor runs first and can
 * pick an architecture. Mach-O gives exported C symbols a leading underscore,
 * so the C name add_arm64 is the assembly label _add_arm64.
 */

.text
.global _add_arm64
.p2align 2

_add_arm64:
#if defined(__arm64__) || defined(__aarch64__)

    /* The AArch64 calling convention placed the first two integer arguments
     * in x0 and x1 before this function was entered. `w` names the low 32
     * bits of the same register, which is what a C `int` occupies. */
    add     w0, w0, w1      /* w0 = w0 + w1 */
    ret                     /* jump to the address in x30, the link register */

    /* Two instructions. No prologue, no stack frame, no spill, because this
     * function needs no memory: the inputs and the result all live in
     * registers the caller already set up. */

#elif defined(__x86_64__)

    /* System V on x86-64 passes the first two integer arguments in rdi and
     * rsi, and returns in rax. `edi`/`esi`/`eax` are their low 32 bits.
     *
     * x86 arithmetic is two-operand and destructive: `add %esi, %edi` would
     * overwrite edi. Since the result has to end up in eax anyway, the usual
     * trick is `lea`, the address-calculation instruction, abused as a
     * three-operand add. It computes edi+esi into eax without touching
     * either input, and clang emits exactly this. */
    leal    (%rdi, %rsi), %eax
    ret

#else
#error "unsupported architecture"
#endif
