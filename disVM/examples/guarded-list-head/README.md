# Rooted mapped list-head projection

The example prepends 32 String-bearing records, copies each head, removes its
source list node and checks the copied record afterwards. The String is a managed
reference, so list-head extraction requires typed ownership handling. Default and
guarded startup use the same compiled native-cell image.

From a checkout, substitute your matching build-product paths:

```sh
LIMBO=build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap
RUNTIME=build/workspace1735/disvm-editor/disvm
"$LIMBO" -F -I tools/limbo-bootstrap/upstream/module \
  -o build/guarded-list-head.dis disVM/examples/guarded-list-head/main.b
"$RUNTIME" --gc-worker --no-jit build/guarded-list-head.dis
"$RUNTIME" --gc-worker --guarded-copies --no-jit build/guarded-list-head.dis
"$RUNTIME" --gc-worker --guarded-copies --jit build/guarded-list-head.dis
```

The build-directory name is illustrative. Installed SDK, from package root:

```sh
./limbo-native -F -I include -o guarded-list-head.dis examples/guarded-list-head/main.b
./disvm --gc-worker --guarded-copies --no-jit guarded-list-head.dis
./disvm --gc-worker --guarded-copies --jit guarded-list-head.dis
```

Expected output: `PASS guarded list head: 32 independent mapped records`.
The guarded JIT profile currently requires hosted SysV x64; explicit C execution
on another target requires independent acceptance. These native-cell images are
not ordinary legacy Inferno/Dis files.

The guarded HEADMP path prepares native root metadata, captures/retains the list
under its source-slot guard, validates the actual allocated payload extent and
map compatibility, then calls the mapped replacement transaction. Its temporary
list owner/root survives descriptor errors, aliased output and consuming callbacks.
Cleanup releases it on success/error without retrying a consumed owner.

The full [SHARED-HEAD.md contract](../../runtime/SHARED-HEAD.md), installed at SDK
root, distinguishes strong ownership, GC roots and access permission. This example
checks language behavior, not concurrent heap execution. The actual diagnostic
separately tests allocation/refusal, source alias, real callback Cycles and errors;
general Dis execution still uses serialized NativeWorld admission.
