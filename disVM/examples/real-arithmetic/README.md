# Four REAL operations in the guarded hosted runtime

The commented `main.b` reads two array elements, computes sum/difference/product/
quotient and checks exact binary-representable results before printing. It uses
existing Limbo syntax and existing Dis opcodes. It creates no OS threads itself.

From the installed runtime SDK directory:

```sh
./limbo-native -I module -o real-arithmetic.dis examples/real-arithmetic/main.b
./disvm --no-jit --gc-worker --guarded-copies real-arithmetic.dis
./disvm --jit --gc-worker --guarded-copies real-arithmetic.dis
```

Expected numbers:6,-2,8,0.5. Compiler/runtime must be from the same SDK. The
explicit guarded profile is selected at startup before executable cache creation.
Omit `--guarded-copies` for the retained default operation path. x64 guarded JIT
entries tail-enter the same checked C operations; that is not inline FP code.

This demonstrates scalar results, not simultaneous managed mutators. NativeWorld
still serializes general managed execution. Independent mapped cell lifetime,
root/graph/GC epochs and ownership remain required. Each operation captures its
input pair, releases all guards before native FP evaluation, then guards output.
The two intervals do not implement atomic read-modify-write on a shared result.
Native FP environment/traps are unchanged; unmasked signal handling is not tested
by this example. See [REAL-ARITHMETIC.md](../../runtime/REAL-ARITHMETIC.md), installed
at SDK root, for diagnostics, masked boundary checks and limits.
