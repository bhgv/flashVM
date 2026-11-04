# Guarded real/integer conversion

The commented `main.b` reads dynamic array values and executes CVTWF, CVTLF,
CVTFW and CVTFL through existing Limbo syntax. It checks integer-to-real results
and the existing half-away adjustment for2.5. It does not demonstrate a new
language keyword or start concurrent managed mutators.

From an installed runtime SDK directory:

```sh
./limbo-native -I module -o real-conversion.dis examples/real-conversion/main.b
./disvm --no-jit --gc-worker --guarded-copies real-conversion.dis
./disvm --jit --gc-worker --guarded-copies real-conversion.dis
```

Expected:`converted: 3 3 3 3`. Use compiler/runtime from the same SDK. The guarded
profile is chosen at startup; default execution remains available without the
flag. x64 selected tails must complete actual dispatch exactly once, rather than
silently returning to interpreter fallback.

CVTRF/CVTFR convert native binary32/64 representations and are covered by actual
C opcodes/dispatch diagnostics. This Limbo example does not emit those two; no
binary32 machine-code guest execution is claimed from it. Native WORD width is
retained: no Dis64 Int32 wire field is widened by this hosted example.

Guarded conversion captures source bytes, releases exclusion for native FP, then
claims output and publishes saved result bytes. A nonfinite/out-of-range rounded
integer conversion raises invalid argument without writing output. FP environment
and trap policy remain native. This is not atomic read-modify-write; NativeWorld
and frame/referent/GC lifetime requirements remain. See
[REAL-CONVERSION.md](../../runtime/REAL-CONVERSION.md), installed at SDK root.
