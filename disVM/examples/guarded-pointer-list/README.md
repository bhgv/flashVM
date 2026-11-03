# Guarded pointer-list head and tail

The example projects String owners, overwrites a list with its own tail and keeps
projected Strings after the source nodes are retired. The final tail is a valid
nil value. The same native-cell image runs with ordinary or guarded startup.

From the checkout, substitute the paths of your configured matching products:

```sh
LIMBO=build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap
RUNTIME=build/workspace1735/disvm-editor/disvm
"$LIMBO" -F -I tools/limbo-bootstrap/upstream/module \
  -o build/guarded-pointer-list.dis disVM/examples/guarded-pointer-list/main.b
"$RUNTIME" --gc-worker --no-jit build/guarded-pointer-list.dis
"$RUNTIME" --gc-worker --guarded-copies --no-jit build/guarded-pointer-list.dis
"$RUNTIME" --gc-worker --guarded-copies --jit build/guarded-pointer-list.dis
```

Installed SDK, from package root:

```sh
./limbo-native -F -I include -o guarded-pointer-list.dis examples/guarded-pointer-list/main.b
./disvm --gc-worker --guarded-copies --no-jit guarded-pointer-list.dis
./disvm --gc-worker --guarded-copies --jit guarded-pointer-list.dis
```

Expected output: `PASS guarded pointer list: HEADP, aliased TAIL, terminal nil`.
The guarded JIT profile currently requires hosted SysV x64; explicit C execution
on other targets needs independent acceptance. Native-cell images are not ordinary
legacy Inferno/Dis files.

Guarded HEADP/TAIL first hold/root the source list under its source-slot guard,
check its actual allocated field extent and, for HEADP, its mapped first slot.
The existing MOVP core then guards the interior field and destination through
retain/mark/publication. Old output is consumed after unlock; source-list cleanup
runs on normal/error return and preserves the first error. No source List address
is borrowed beyond the lifetime of that hold.

[SHARED-LIST-POINTER.md](../../runtime/SHARED-LIST-POINTER.md), installed at SDK
root, defines exact limits and diagnostics. This example demonstrates language
ownership behavior, not general parallel mutation or arbitrary callback recovery.
NativeWorld still serializes managed execution. The diagnostic separately runs
real callback-driven Cycles, refusal/rollback, aliases, cache retirement and errors.
Measured extra costs are recorded in [PERFORMANCE-COSTS.md](../../runtime/PERFORMANCE-COSTS.md).
