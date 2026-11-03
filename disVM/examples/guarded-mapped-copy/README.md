# Guarded mapped record assignment

`main.b` copies a record containing a managed String and two integers. The same
compiled native-cell image runs with ordinary startup or the experimental guarded
profile. It checks the last complete record, releases the destination name and
checks that the source's independently held name is intact.

With matching hosted compiler/runtime products, substitute their actual paths:

```sh
LIMBO=build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap
RUNTIME=build/workspace1735/disvm-editor/disvm
"$LIMBO" -F -I tools/limbo-bootstrap/upstream/module \
  -o build/guarded-mapped-example.dis disVM/examples/guarded-mapped-copy/main.b
"$RUNTIME" --gc-worker --no-jit build/guarded-mapped-example.dis
"$RUNTIME" --gc-worker --guarded-copies --no-jit build/guarded-mapped-example.dis
"$RUNTIME" --gc-worker --guarded-copies --jit build/guarded-mapped-example.dis
```

The build-directory name above is an example; use your configured build's products.
For an installed SDK, compile with `./limbo-native -F -I include`, use the source
at `examples/guarded-mapped-copy/main.b`, and run `./disvm` with the same flags.
The guarded JIT profile currently requires hosted SysV x64. Explicit C execution
on other targets requires independent validation. Do not run this native-cell
image as an ordinary Inferno/legacy Dis file.

Expected output: `PASS guarded mapped example: 1000 copies, source intact`.

MOVMP's selected transaction snapshots bytes, retains new references under range
protection, roots displaced owners, publishes once, unlocks and retires old owners.
Root occurrence withdrawal before each destructor keeps other old owners visible
across callbacks. The detailed contract, allocation/error tests and benchmark
are in [SHARED-MAPPED.md](../../runtime/SHARED-MAPPED.md) (installed at package root).
This example is a language-level ownership check, not a callback/error-race or
parallel-execution proof. The dedicated actual-runtime tests cover those bounded
callback cases; general managed execution is still serialized by NativeWorld.
