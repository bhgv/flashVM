# Jointly guarded arithmetic

The fully commented Limbo example exercises ADD, SUB, AND, OR and XOR for byte,
int and big. Thirty-two changing input pairs keep every result below byte wrap;
the native runtime fixture separately checks full unsigned boundaries and partial
aliasing. Guarding must cover both input captures and result publication together.

Compile the native-cell image once, then compare startup profiles:

```sh
LIMBO=build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap
RUNTIME=build/workspace1735/disvm-editor/disvm
"$LIMBO" -F -I tools/limbo-bootstrap/upstream/module \
  -o build/guarded-arithmetic.dis disVM/examples/guarded-arithmetic/main.b
"$RUNTIME" --gc-worker --no-jit build/guarded-arithmetic.dis
"$RUNTIME" --gc-worker --guarded-copies --no-jit build/guarded-arithmetic.dis
"$RUNTIME" --gc-worker --guarded-copies --jit build/guarded-arithmetic.dis
```

From the installed runtime/editor package root:

```sh
./limbo-native -F -I include -o guarded-arithmetic.dis examples/guarded-arithmetic/main.b
./disvm --gc-worker --guarded-copies --no-jit guarded-arithmetic.dis
./disvm --gc-worker --guarded-copies --jit guarded-arithmetic.dis
```

Expected: `PASS guarded arithmetic: 15 operations, 32 input pairs`.
Paths illustrate one local build; guarded JIT evidence is hosted Linux/SysV x64.
These images are not ordinary legacy Inferno files or a new Dis64 numeric profile.

Internally shared_arithmetic validates the current owner and all spans, claims
the same ordered native stripe domain as existing cell/range operations, captures
both inputs, computes modular integer results and publishes before unlock. The
new dvm_range_access_three_begin API keeps the existing access structure layout;
source order never determines lock order. Native callers must independently keep
ranges/domain alive. A guard is not a root, mapping proof or ownership transfer.
No allocation/VM callback occurs under these guards; BUSY unwinds before effects.

See [the contract](../../runtime/SHARED-ARITHMETIC.md), installed at package root.
NativeWorld still serializes general managed computation. The compiler's future
local/shared selection and remaining GC/ownership protocols are separate work.
