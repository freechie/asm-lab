# 08 — Same code on x86-64

```
make run LESSON=08
make asm LESSON=08
make x86 LESSON=08
```

## Calling conventions

ARM64 passes the first eight integer arguments in `x0` through `x7`. The `sum_args` dump in `abi.c` shows all eight registers in use.

x86-64 passes the first six integer arguments in `%rdi`, `%rsi`, `%rdx`, `%rcx`, `%r8`, and `%r9`. Arguments seven and eight live on the stack.

## AT&T syntax and operand order

x86 assembly in this lab uses AT&T syntax. The source operand comes first and the destination second. `add %rsi, %rdi` means `%rdi = %rdi + %rsi`.

A compact add on x86 often looks like `leal (%rdi,%rsi), %eax`. ARM64 expresses the same operation as `add w0, w1, w0` with three register operands.

## RISC vs CISC

ARM64 uses fixed-width instructions and mostly three-register operations. x86 uses variable-length instructions and many two-operand forms that overwrite a source register. Instruction count and per-instruction complexity differ even when the C source is identical.

## compare.py

`compare.py` runs `tools/asmdump.sh` on `01-a-plus-b/add.c` for both architectures and prints the `add_called` function side by side.

## Commands

```
make asm LESSON=01
make x86 LESSON=01
make run LESSON=01 ARCH=x86_64
make asm LESSON=08
make x86 LESSON=08
```

## Rosetta timings

Running with `ARCH=x86_64` on Apple Silicon goes through Rosetta translation. Those timings measure translated code, not native x86-64 silicon.

## Further reading

Full list with ISBNs: [README.md](../README.md#resources).

- CS:APP 3e, ch. 3. ISBN 978-0-13-409266-9. Now in the book's native x86-64.
- Agner Fog, [calling-conventions manual](https://www.agner.org/optimize/)
- [System V AMD64 ABI](https://gitlab.com/x86-psABIs/x86-64-ABI)
- Apple, [About the Rosetta translation environment](https://developer.apple.com/documentation/apple-silicon/about-the-rosetta-translation-environment)
- [AAPCS64](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst)
- Matt Godbolt, [What Has My Compiler Done for Me Lately?](https://www.youtube.com/watch?v=bSkpMdDe4g4) with both ARM64 and x86-64 in Compiler Explorer
