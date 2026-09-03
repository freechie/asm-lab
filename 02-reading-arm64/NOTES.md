# 02 — Reading ARM64

```
make run LESSON=02
make asm LESSON=02
make asm LESSON=02 DIFF=1
make asm LESSON=02 FUNC=add_ints
```

## Register file

ARM64 has 31 general-purpose registers named `x0` through `x30`. Each `x` register is 64 bits wide. The low 32 bits of `xN` are also named `wN`. Integer C locals and `int` parameters use the `w` view at `-O2`.

`sp` is the stack pointer. `lr` (`x30`) holds the return address after `bl`. `ret` jumps to `lr`.

## Calling convention

The first eight integer or pointer arguments arrive in `x0` through `x7`. The return value of an `int` function lives in `w0` (the low half of `x0`).

Look at `add_ints`. At the function entry both operands are already in registers because the caller obeyed the convention. The body is one `add` and one `ret`.

`call_add` shows a real call. The caller moves arguments into `x0` and `x1`, executes `bl _add_ints`, and reads the result from `w0` when control returns.

## Load and store

`load_store` writes through a pointer then reads it back. Stores become `str`. Loads become `ldr`. The address register is often `x0` when the pointer was the first argument.

At `-O0` the compiler spills almost everything to the stack between statements. At `-O2` the same function keeps values in registers and you see far fewer memory operations. Run `make asm LESSON=02 DIFF=1` on `load_store` and count `str`/`ldr` pairs in each version.

## Arithmetic and compare

`mul_ints` lowers to `mul`. `min_of_two` lowers to `cmp` followed by a conditional branch (`b.lt`) or a conditional select (`csel`) at higher optimization.

Flags set by `cmp` drive conditional branches. `b.eq`, `b.ne`, `b.lt`, and friends test those flags without re-comparing.

## Branches

Unconditional `b` jumps. `bl` jumps and links, storing the return address in `lr`. Every `__attribute__((noinline))` function call from `main` is a `bl` at the call site and a `ret` at the callee.

## Exercises

1. Change `int` to `long` in `add_ints` and dump again. Does the assembly switch from `w0` to `x0`?
2. Change `int` to `double` in `mul_ints`. Which register file does clang use now?
3. Pick `min_of_two` at `-O0` and `-O2`. Does the compiler prefer a branch or `csel` at `-O2`?
4. In `call_add`, identify every instruction between the `bl` and the moment `w0` holds the sum.

## Further reading

Full list with ISBNs: [README.md](../README.md#resources).

- CS:APP 3e, 3.4–3.7. ISBN 978-0-13-409266-9. x86-64 in the book. Map `%rdi` to `x0`.
- Pyeatt and Ughetta, *ARM 64-Bit Assembly Language*, ch. 1–4. ISBN 978-0-12-819221-4.
- ARM, [Learn the architecture: A64](https://developer.arm.com/documentation/102374/0103/Overview)
- Josh Lospinoso, [ARM64 Is Not Just x64 With More Registers](https://lospino.so/blog/abi-series/arm64-is-not-just-x64/)
- [pkivolowitz/asm_book](https://github.com/pkivolowitz/asm_book)
- [AAPCS64](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst)
- Casey Muratori, Performance-Aware Programming, Part 1 (Reading ASM)
