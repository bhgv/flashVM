# Guarded multiplication, division and remainder

The commented Limbo program checks MUL/DIV/MOD for byte, int and big. Byte
multiplication wraps to eight bits; signed quotient truncates toward zero and
remainder follows the dividend's sign. The example uses nonzero divisors so it
can check every result. The native fixture separately checks MIN/-1 and zero
error precedence with invalid dividend/output cells.

Compile one native-cell image, then compare startup selections:

```sh
LIMBO=build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap
RUNTIME=build/workspace1735/disvm-editor/disvm
"$LIMBO" -F -I tools/limbo-bootstrap/upstream/module \
  -o build/guarded-product-division.dis disVM/examples/guarded-product-division/main.b
"$RUNTIME" --gc-worker --no-jit build/guarded-product-division.dis
"$RUNTIME" --gc-worker --guarded-copies --no-jit build/guarded-product-division.dis
"$RUNTIME" --gc-worker --guarded-copies --jit build/guarded-product-division.dis
```

Installed SDK root:

```sh
./limbo-native -F -I include -o guarded-product-division.dis examples/guarded-product-division/main.b
./disvm --gc-worker --guarded-copies --no-jit guarded-product-division.dis
./disvm --gc-worker --guarded-copies --jit guarded-product-division.dis
```

Expected: `PASS guarded products/division: nine typed operations`.
The guarded profile requires --gc-worker; actual JIT acceptance here is Linux/SysV
x64. This is native WORD storage, not ordinary legacy Dis or a new Dis64 profile.

A valid operation guards both sources/result together. DIV/MOD read a valid guarded
divisor before other payloads. If dividend/output geometry is invalid, only the
valid divisor slot is claimed through three aliases: zero returns exZdiv, nonzero
returns exBounds. All error propagation happens after unlock. No managed owner
transfer, allocation or VM callback occurs under guards. Caller still supplies live
mapped storage; guards do not prove provenance, roots or general parallel safety.

For actual guest failure checks, run the checkout developer script:
`python3 disVM/runtime/test_arithmetic_errors.py RUNTIME LIMBO INCLUDE_DIR`.
It compiles all six zero DIV/MOD cases and requires exit1/`zero divide`, absence of
after-result output and identical default/guarded C/JIT behavior. Native/error/cache
checks are separate from production admission. See
[PRODUCT-DIVISION.md](../../runtime/PRODUCT-DIVISION.md), installed at SDK root.
